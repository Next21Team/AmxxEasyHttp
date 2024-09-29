#include "EasyHttp.h"

#include <utility>
#include <filesystem>

#include "session_factory/CprSessionFactory.h"
#include "datetime_service/DateTimeService.h"

using namespace ezhttp;
using namespace std::chrono_literals;

EasyHttp::EasyHttp(std::string ca_cert_path, int threads) :
    ca_cert_path_(std::move(ca_cert_path)),
    session_cache_(std::make_shared<CprSessionFactory>(), std::make_shared<DateTimeService>(), std::chrono::seconds(kMaxAgeConnSeconds), kMaxSessionsPerHost)
{
    update_scheduler_ = std::make_unique<async::fifo_scheduler>();
    request_scheduler_ = std::make_shared<async::threadpool_scheduler>(threads);
}

std::shared_ptr<RequestControlInterface> EasyHttp::SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const ResponseCallback& on_complete)
{
    auto request_control = std::make_shared<RequestControl>();

    auto task = async::spawn(*request_scheduler_, [this, request_control, method, url, options]()
    {
        Response ezhttp_response;

        // Used scope to ensure that cpr::Response is destroyed before exiting the function.
        // For some reason, if this is not done, double free (?) for std::shared_ptr<CurlHolder> occurs occasionally.
        {
            cpr::Response cpr_response = SendRequest(request_control, method, url, options);
            ezhttp_response = Response(cpr_response);
        }

        return ezhttp_response;
    })
    .then(*update_scheduler_, [request_control, on_complete](Response response)
    {
        request_control->set_completed_unsafe();

        if (!request_control->is_forgotten_unsafe())
            on_complete(std::move(response));
    });

    tasks_.emplace_back(std::move(task), request_control);

    return request_control;
}

EasyHttp::~EasyHttp()
{
    while (!tasks_.empty())
        EasyHttp::RunFrame();

    // !! make shure that request_scheduler_ destroys always before update_scheduler_
    // to avoid continuation in destroyed update_scheduler_
    request_scheduler_.reset();
    update_scheduler_.reset();
}

void EasyHttp::RunFrame()
{
    for (int i = 0; i < kMaxTasksExecPerFrame; i++)
    {
        if (!update_scheduler_->try_run_one_task())
            break;
    }

    // erase completed tasks
    for (auto it = tasks_.begin(); it != tasks_.end(); )
    {
        auto& task = it->get_task();

        if (task.ready())
            it = tasks_.erase(it);
        else
            ++it;
    }
}

int EasyHttp::GetActiveRequestCount()
{
    return tasks_.size();
}

void EasyHttp::ForgetAllRequests()
{
    for (auto& task_data : tasks_)
        task_data.get_request_control()->set_forgotten_unsafe();
}

void EasyHttp::CancelAllRequests()
{
    for (auto& task_data : tasks_)
    {
        task_data.get_request_control()->CancelRequest();
    }
}

void EasyHttp::SetSessionCommonOptions(cpr::Session& session, const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options)
{
#ifdef LINUX
    cpr::SslOptions ssl_opt;
    ssl_opt.ca_info = ca_cert_path_;
    session.SetSslOptions(ssl_opt);
#endif

    session.SetProgressCallback(cpr::ProgressCallback(
        [request_control](cpr::cpr_off_t downloadTotal, cpr::cpr_off_t downloadNow, cpr::cpr_off_t uploadTotal, cpr::cpr_off_t uploadNow, intptr_t userdata) {
            std::lock_guard lock_guard(request_control->get_mutex());

            request_control->set_progress_unsafe(
                    RequestProgress{(int32_t) (downloadTotal), (int32_t) (downloadNow), (int32_t) (uploadTotal),
                                    (int32_t) (uploadNow)});

            return !request_control->is_cancelled_unsafe();
        }));

    if (options.timeout)
        session.SetTimeout(*options.timeout);

    if (options.connect_timeout)
        session.SetConnectTimeout(*options.connect_timeout);
}

cpr::Response EasyHttp::SendRequest(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options)
{
    std::unique_ptr<cpr::Session> session = session_cache_.GetSession(url.str());
    SetSessionCommonOptions(*session, request_control, url, options);

    cpr::Response response;
    switch (method)
    {
        case RequestMethod::FtpUpload:
            response = FtpUpload(*session, request_control, url, options);
            break;

        case RequestMethod::FtpDownload:
            response = FtpDownload(*session, request_control, url, options);
            break;

        default:
            response = SendHttpRequest(*session, request_control, method, url, options);
            break;
    }

    session_cache_.ReturnSession(*session);
    return response;
}

cpr::Response EasyHttp::SendHttpRequest(cpr::Session& session, const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options)
{
    if (options.user_agent)
        session.SetUserAgent(*options.user_agent);

    if (options.url_parameters)
        session.SetParameters(*options.url_parameters);

    if (options.form_payload)
        session.SetPayload(*options.form_payload);

    if (options.body)
        session.SetBody(*options.body);

    if (options.header)
        session.SetHeader(*options.header);

    if (options.cookies)
        session.SetCookies(*options.cookies);

    if (options.proxy_url)
    {
        std::string protocol = url.str().substr(0, url.str().find(':'));
        session.SetProxies({{protocol, *options.proxy_url}});
    }

    if (options.proxy_auth)
    {
        std::string user = options.proxy_auth->first;
        std::string password = options.proxy_auth->second;

        std::string protocol = url.str().substr(0, url.str().find(':'));
        session.SetProxyAuth({{protocol, cpr::EncodedAuthentication{user, password}}});
    }

    if (options.auth)
        session.SetAuth(*options.auth);

    switch (method)
    {
        case RequestMethod::HttpPost:
            return session.Post();

        case RequestMethod::HttpPut:
            return session.Put();

        case RequestMethod::HttpPatch:
            return session.Patch();

        case RequestMethod::HttpDelete:
            return session.Delete();
    }

    return session.Get();
}

cpr::Response EasyHttp::FtpUpload(cpr::Session& session, const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options)
{
    if (!options.file_path)
        return session.Complete(CURLE_READ_ERROR);

    std::ifstream file(*options.file_path, std::fstream::in | std::fstream::binary);
    if (!file.is_open())
        return session.Complete(CURLE_READ_ERROR);

    session.SetReadCallback(cpr::ReadCallback([&file](char* buffer, size_t& size, intptr_t userdata) {
        if (file.rdstate() & std::fstream::failbit ||
            file.rdstate() & std::fstream::eofbit)
        {
            size = 0;
            return true;
        }

        file.read(buffer, size);

        if (file.rdstate() & std::fstream::failbit ||
            file.rdstate() & std::fstream::eofbit)
        {
            size = file.gcount();
            return true;
        }

        return true;
    }));

    CURL* curl = session.GetCurlHolder()->handle;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 0);
    curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
    if (options.require_secure)
    {
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    }
    CURLcode curl_result = curl_easy_perform(curl);

    file.close();

    cpr::Response resp = session.Complete(curl_result);
    if (request_control->IsCancelled())
    {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        resp.error.code = cpr::ErrorCode::REQUEST_CANCELLED;
        resp.error.message = "Ftp uploading canceled. Code " + std::to_string(response_code);
    }

    return resp;
}

cpr::Response EasyHttp::FtpDownload(cpr::Session& session, const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options)
{
    if (!options.file_path)
        return session.Complete(CURLE_READ_ERROR);

    std::ofstream file(*options.file_path, std::ofstream::out | std::ofstream::binary);
    if (!file.is_open())
        return session.Complete(CURLE_READ_ERROR);

    session.SetWriteCallback(cpr::WriteCallback([&file](std::string data, intptr_t userdata) {
        file.write(data.c_str(), data.size());
        return true;
    }));

    CURL* curl = session.GetCurlHolder()->handle;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 0);
    if (options.require_secure)
    {
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    }
    CURLcode curl_result = curl_easy_perform(curl);

    file.close();

    cpr::Response resp = session.Complete(curl_result);
    if (request_control->IsCancelled())
    {
        std::filesystem::remove(*options.file_path);

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        resp.error.code = cpr::ErrorCode::REQUEST_CANCELLED;
        resp.error.message = "Ftp downloading canceled. Code " + std::to_string(response_code);
    }

    return resp;
}