#include "EasyHttp.h"
#include <amxxmodule.h>
#include <undef_metamod.h>
#include <utility>
#include <filesystem>

using namespace ezhttp;
using namespace std::chrono_literals;

EasyHttp::EasyHttp(std::string ca_cert_path) :
    ca_cert_path_(std::move(ca_cert_path))
{
    update_scheduler_ = std::make_unique<async::fifo_scheduler>();
    request_scheduler_ = std::make_unique<async::threadpool_scheduler>(kMaxThreads);
}

std::shared_ptr<RequestControl> EasyHttp::SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const RequestCallback& on_complete)
{
    auto request_control = std::make_shared<RequestControl>();

    auto task = async::spawn(*request_scheduler_, [this, request_control, method, url, options]()
    {
        return SendRequest(request_control, method, url, options);
    })
    .then(*update_scheduler_, [request_control, on_complete](cpr::Response response)
    {
        if (!request_control->canceled)
            on_complete(std::move(response));
    });

    tasks_.emplace_back(std::move(task));

    return request_control;
}

EasyHttp::~EasyHttp()
{
    while (!async::when_all(tasks_).ready())
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

    for (auto it = tasks_.begin(); it != tasks_.end(); )
    {
        if (!it->valid() || it->ready())
            it = tasks_.erase(it);
        else
            ++it;
    }
}

cpr::Session EasyHttp::CreateSessionWithCommonOptions(const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options)
{
    cpr::Session session;

#ifdef LINUX
    cpr::SslOptions ssl_opt;
    ssl_opt.ca_info = ca_cert_path_;
    session.SetSslOptions(ssl_opt);
#endif

    session.SetUrl(url);
    session.SetProgressCallback(cpr::ProgressCallback(
        [request_control](cpr::cpr_off_t downloadTotal, cpr::cpr_off_t downloadNow, cpr::cpr_off_t uploadTotal, cpr::cpr_off_t uploadNow, intptr_t userdata) {
            request_control->progress = RequestControl::Progress{ (int32_t)(downloadTotal), (int32_t)(downloadNow), (int32_t)(uploadTotal), (int32_t)(uploadNow) };

            return !request_control->canceled;
        }));

    if (options.timeout)
        session.SetTimeout(*options.timeout);

    if (options.connect_timeout)
        session.SetConnectTimeout(*options.connect_timeout);

    return session;
}

cpr::Response EasyHttp::SendRequest(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options)
{
    switch (method)
    {
        case RequestMethod::FtpUpload:
            return FtpUpload(request_control, url, options);

        case RequestMethod::FtpDownload:
            return FtpDownload(request_control, url, options);

        case RequestMethod::HttpGet:
        case RequestMethod::HttpPost:
        default:
            return SendHttpRequest(request_control, method, url, options);
    }
}

cpr::Response EasyHttp::SendHttpRequest(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options)
{
    cpr::Session session = CreateSessionWithCommonOptions(request_control, url, options);

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

    if (method == RequestMethod::HttpGet)
        return session.Get();

    return session.Post();
}

cpr::Response EasyHttp::FtpUpload(const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options)
{
    cpr::Session session = CreateSessionWithCommonOptions(request_control, url, options);

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
    CURLcode curl_code = curl_easy_perform(curl);

    file.close();

    cpr::Response resp = session.Complete(curl_code);
    if (resp.error.code == cpr::ErrorCode::REQUEST_CANCELLED)
        std::filesystem::remove(*options.file_path);

    return resp;
}

cpr::Response EasyHttp::FtpDownload(const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options)
{
    cpr::Session session = CreateSessionWithCommonOptions(request_control, url, options);

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
    CURLcode curl_code = curl_easy_perform(curl);

    file.close();

    cpr::Response resp = session.Complete(curl_code);
    if (resp.error.code == cpr::ErrorCode::REQUEST_CANCELLED)
        std::filesystem::remove(*options.file_path);

    return resp;
}