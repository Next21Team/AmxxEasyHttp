#include "EasyHttp.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

#include <curl/curl.h>

#include "datetime_service/DateTimeService.h"
#include "session_factory/CprSessionFactory.h"
#include "utils/ftp_utils.h"

using namespace ezhttp;

namespace
{
    struct FtpWildcardDownloadContext
    {
        std::filesystem::path destination_directory;
        std::filesystem::path current_file_path;
        std::ofstream current_file;
        std::vector<std::filesystem::path> created_files;
        bool file_open_failed = false;
        bool write_failed = false;
    };

    bool EnsureDirectoryExists(const std::filesystem::path &directory_path)
    {
        if (directory_path.empty())
            return true;

        std::error_code ec;
        std::filesystem::create_directories(directory_path, ec);
        return !ec;
    }

    void RemoveFileIfExists(const std::filesystem::path &file_path)
    {
        if (file_path.empty())
            return;

        std::error_code ec;
        std::filesystem::remove(file_path, ec);
    }

    void RemoveFilesIfExist(const std::vector<std::filesystem::path> &file_paths)
    {
        for (const auto &file_path : file_paths)
            RemoveFileIfExists(file_path);
    }

    void MarkCancelledResponse(Response &response, std::string message)
    {
        response.error.code = cpr::ErrorCode::REQUEST_CANCELLED;
        response.error.message = std::move(message);
    }

    int OnFtpWildcardChunkBegin(const curl_fileinfo *file_info, void *user_data, int /*remaining_files*/)
    {
        auto *context = static_cast<FtpWildcardDownloadContext *>(user_data);
        if (context == nullptr || file_info == nullptr || file_info->filename == nullptr)
            return CURL_CHUNK_BGN_FUNC_FAIL;

        if (file_info->filetype != CURLFILETYPE_FILE)
            return CURL_CHUNK_BGN_FUNC_SKIP;

        context->current_file.close();
        context->current_file.clear();
        context->current_file_path = context->destination_directory / file_info->filename;

        if (!EnsureDirectoryExists(context->current_file_path.parent_path()))
        {
            context->file_open_failed = true;
            return CURL_CHUNK_BGN_FUNC_FAIL;
        }

        context->current_file.open(context->current_file_path, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
        if (!context->current_file.is_open())
        {
            context->file_open_failed = true;
            return CURL_CHUNK_BGN_FUNC_FAIL;
        }

        context->created_files.emplace_back(context->current_file_path);
        return CURL_CHUNK_BGN_FUNC_OK;
    }

    long OnFtpWildcardChunkEnd(void *user_data)
    {
        auto *context = static_cast<FtpWildcardDownloadContext *>(user_data);
        if (context == nullptr)
            return CURL_CHUNK_END_FUNC_FAIL;

        context->current_file.close();
        context->current_file_path.clear();
        return CURL_CHUNK_END_FUNC_OK;
    }
}

EasyHttp::EasyHttp(std::string ca_cert_path, int threads) : ca_cert_path_(std::move(ca_cert_path)),
                                                            session_cache_(std::make_shared<CprSessionFactory>(), std::make_shared<DateTimeService>(), std::chrono::seconds(kMaxAgeConnSeconds), kMaxSessionsPerHost)
{
    const int worker_count = std::max(1, threads);
    worker_threads_.reserve(worker_count);

    for (int i = 0; i < worker_count; ++i)
        worker_threads_.emplace_back(&EasyHttp::WorkerLoop, this);
}

std::shared_ptr<RequestControl> EasyHttp::SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const ResponseCallback &on_complete)
{
    auto request_control = std::make_shared<RequestControl>();
    cpr::Url normalized_url = url;

    if (method == RequestMethod::FtpUpload || method == RequestMethod::FtpDownload)
        normalized_url = cpr::Url{utils::NormalizeFtpUrl(url.str())};

    {
        std::lock_guard lock_guard(pending_requests_mutex_);
        pending_requests_.push_back(PendingRequest{request_control, method, normalized_url, options, on_complete});
    }

    requests_.emplace_back(request_control);
    pending_requests_cv_.notify_one();

    return request_control;
}

EasyHttp::~EasyHttp()
{
    {
        std::lock_guard lock_guard(pending_requests_mutex_);
        stop_requested_ = true;
    }

    pending_requests_cv_.notify_all();

    for (auto &worker_thread : worker_threads_)
    {
        if (worker_thread.joinable())
            worker_thread.join();
    }

    while (true)
    {
        RunFrame();

        std::lock_guard lock_guard(completed_requests_mutex_);
        if (completed_requests_.empty() && requests_.empty())
            break;
    }
}

void EasyHttp::WorkerLoop()
{
    while (true)
    {
        PendingRequest pending_request;

        {
            std::unique_lock lock_guard(pending_requests_mutex_);
            pending_requests_cv_.wait(lock_guard, [this]()
                                      { return stop_requested_ || !pending_requests_.empty(); });

            if (pending_requests_.empty())
                return;

            pending_request = std::move(pending_requests_.front());
            pending_requests_.pop_front();
        }

        Response response = pending_request.request_control->canceled.load()
                                ? CreateErrorResponse(pending_request.url, cpr::ErrorCode::REQUEST_CANCELLED, "Request canceled before dispatch")
                                : SendRequest(pending_request.request_control, pending_request.method, pending_request.url, pending_request.options);

        std::lock_guard lock_guard(completed_requests_mutex_);
        completed_requests_.push_back(CompletedRequest{
            pending_request.request_control,
            std::move(response),
            std::move(pending_request.on_complete)});
    }
}

bool EasyHttp::TryPopCompletedRequest(CompletedRequest &completed_request)
{
    std::lock_guard lock_guard(completed_requests_mutex_);
    if (completed_requests_.empty())
        return false;

    completed_request = std::move(completed_requests_.front());
    completed_requests_.pop_front();
    return true;
}

void EasyHttp::RunFrame()
{
    for (int i = 0; i < kMaxTasksExecPerFrame; ++i)
    {
        CompletedRequest completed_request;
        if (!TryPopCompletedRequest(completed_request))
            break;

        completed_request.request_control->completed.store(true);

        if (!completed_request.request_control->forgotten.load())
            completed_request.on_complete(std::move(completed_request.response));
    }

    for (auto it = requests_.begin(); it != requests_.end();)
    {
        if ((*it)->completed.load())
            it = requests_.erase(it);
        else
            ++it;
    }
}

void EasyHttp::ForgetAllRequests()
{
    for (auto &request : requests_)
        request->forgotten.store(true);
}

void EasyHttp::CancelAllRequests()
{
    for (auto &request : requests_)
        request->canceled.store(true);

    pending_requests_cv_.notify_all();
}

Response EasyHttp::CreateErrorResponse(const cpr::Url &url, cpr::ErrorCode code, std::string message) const
{
    Response response;
    response.url = url;
    response.error.code = code;
    response.error.message = std::move(message);
    return response;
}

void EasyHttp::SetSessionCommonOptions(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url & /*url*/, const RequestOptions &options)
{
#ifdef LINUX
    cpr::SslOptions ssl_opt;
    ssl_opt.ca_info = ca_cert_path_;
    session.SetSslOptions(ssl_opt);
#endif

    session.SetProgressCallback(cpr::ProgressCallback(
        [request_control](cpr::cpr_off_t download_total, cpr::cpr_off_t download_now, cpr::cpr_off_t upload_total, cpr::cpr_off_t upload_now, intptr_t /*userdata*/)
        {
            request_control->SetProgress(
                static_cast<int32_t>(download_total),
                static_cast<int32_t>(download_now),
                static_cast<int32_t>(upload_total),
                static_cast<int32_t>(upload_now));

            return !request_control->canceled.load();
        }));

    if (options.timeout)
        session.SetTimeout(*options.timeout);

    if (options.connect_timeout)
        session.SetConnectTimeout(*options.connect_timeout);
}

Response EasyHttp::SendRequest(const std::shared_ptr<RequestControl> &request_control, RequestMethod method, const cpr::Url &url, const RequestOptions &options)
{
    std::unique_ptr<cpr::Session> session = session_cache_.GetSession(url.str());
    if (!session)
        return CreateErrorResponse(url, cpr::ErrorCode::INVALID_URL_FORMAT, "Invalid URL");

    if (request_control->canceled.load())
    {
        session_cache_.ReturnSession(*session);
        return CreateErrorResponse(url, cpr::ErrorCode::REQUEST_CANCELLED, "Request canceled before transfer");
    }

    SetSessionCommonOptions(*session, request_control, url, options);

    Response response;
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

Response EasyHttp::SendHttpRequest(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, RequestMethod method, const cpr::Url &url, const RequestOptions &options)
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

    if (request_control->canceled.load())
        return CreateErrorResponse(url, cpr::ErrorCode::REQUEST_CANCELLED, "Request canceled before transfer");

    Response response;
    switch (method)
    {
    case RequestMethod::HttpPost:
        response = Response(session.Post());
        break;

    case RequestMethod::HttpPut:
        response = Response(session.Put());
        break;

    case RequestMethod::HttpPatch:
        response = Response(session.Patch());
        break;

    case RequestMethod::HttpDelete:
        response = Response(session.Delete());
        break;

    default:
        response = Response(session.Get());
        break;
    }

    if (request_control->canceled.load())
        MarkCancelledResponse(response, "Request canceled");

    return response;
}

Response EasyHttp::FtpUpload(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url &url, const RequestOptions &options)
{
    if (!options.file_path)
        return CreateErrorResponse(url, cpr::ErrorCode::INTERNAL_ERROR, "Local file path is required for FTP upload");

    std::ifstream file(*options.file_path, std::fstream::in | std::fstream::binary);
    if (!file.is_open())
        return CreateErrorResponse(url, cpr::ErrorCode::INTERNAL_ERROR, "Failed to open local file for FTP upload");

    session.SetReadCallback(cpr::ReadCallback([&file](char *buffer, size_t &size, intptr_t /*userdata*/)
                                              {
        if (file.rdstate() & std::fstream::failbit ||
            file.rdstate() & std::fstream::eofbit)
        {
            size = 0;
            return true;
        }

        file.read(buffer, static_cast<std::streamsize>(size));

        if (file.rdstate() & std::fstream::failbit ||
            file.rdstate() & std::fstream::eofbit)
        {
            size = static_cast<size_t>(file.gcount());
            return true;
        }

        return true; }));

    if (request_control->canceled.load())
        return CreateErrorResponse(url, cpr::ErrorCode::REQUEST_CANCELLED, "Ftp uploading canceled before transfer");

    CURL *curl = session.GetCurlHolder()->handle;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 0L);
    curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
    if (options.require_secure)
    {
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    CURLcode curl_result = curl_easy_perform(curl);
    file.close();

    Response response(session.Complete(curl_result));
    if (request_control->canceled.load())
        MarkCancelledResponse(response, "Ftp uploading canceled");

    return response;
}

Response EasyHttp::FtpDownload(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url &url, const RequestOptions &options)
{
    if (!options.file_path)
        return CreateErrorResponse(url, cpr::ErrorCode::INTERNAL_ERROR, "Local file path is required for FTP download");

    if (utils::HasWildcardPath(url.str()))
        return FtpDownloadWildcard(session, request_control, url, options);

    return FtpDownloadSingle(session, request_control, url, options);
}

Response EasyHttp::FtpDownloadSingle(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url &url, const RequestOptions &options)
{
    const std::filesystem::path file_path = utils::BuildDownloadPath(
        *options.file_path,
        utils::GetRemoteFileName(url.str()));

    if (file_path.empty())
        return CreateErrorResponse(url, cpr::ErrorCode::INTERNAL_ERROR, "Failed to resolve local file path for FTP download");

    if (!EnsureDirectoryExists(file_path.parent_path()))
        return CreateErrorResponse(url, cpr::ErrorCode::INTERNAL_ERROR, "Failed to create local directory for FTP download");

    std::ofstream file(file_path, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    if (!file.is_open())
        return CreateErrorResponse(url, cpr::ErrorCode::INTERNAL_ERROR, "Failed to open local file for FTP download");

    bool write_failed = false;

    session.SetWriteCallback(cpr::WriteCallback([&file, &write_failed](std::string data, intptr_t /*userdata*/)
                                                {
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
        write_failed = !file.good();
        return !write_failed; }));

    if (request_control->canceled.load())
    {
        file.close();
        RemoveFileIfExists(file_path);
        return CreateErrorResponse(url, cpr::ErrorCode::REQUEST_CANCELLED, "Ftp downloading canceled before transfer");
    }

    CURL *curl = session.GetCurlHolder()->handle;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 0L);
    if (options.require_secure)
    {
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    CURLcode curl_result = curl_easy_perform(curl);
    file.close();

    Response response(session.Complete(curl_result));
    if (request_control->canceled.load())
    {
        RemoveFileIfExists(file_path);
        MarkCancelledResponse(response, "Ftp downloading canceled");
    }
    else if (curl_result != CURLE_OK || write_failed)
    {
        RemoveFileIfExists(file_path);
    }

    return response;
}

Response EasyHttp::FtpDownloadWildcard(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url &url, const RequestOptions &options)
{
    const std::filesystem::path destination_directory(*options.file_path);
    if (!EnsureDirectoryExists(destination_directory))
        return CreateErrorResponse(url, cpr::ErrorCode::INTERNAL_ERROR, "Failed to create local directory for FTP download");

    FtpWildcardDownloadContext context;
    context.destination_directory = destination_directory;

    session.SetWriteCallback(cpr::WriteCallback([&context](std::string data, intptr_t /*userdata*/)
                                                {
        if (!context.current_file.is_open())
        {
            context.write_failed = true;
            return false;
        }

        context.current_file.write(data.data(), static_cast<std::streamsize>(data.size()));
        context.write_failed = !context.current_file.good();
        return !context.write_failed; }));

    if (request_control->canceled.load())
        return CreateErrorResponse(url, cpr::ErrorCode::REQUEST_CANCELLED, "Ftp downloading canceled before transfer");

    CURL *curl = session.GetCurlHolder()->handle;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 0L);
    curl_easy_setopt(curl, CURLOPT_WILDCARDMATCH, 1L);
    curl_easy_setopt(curl, CURLOPT_CHUNK_BGN_FUNCTION, OnFtpWildcardChunkBegin);
    curl_easy_setopt(curl, CURLOPT_CHUNK_END_FUNCTION, OnFtpWildcardChunkEnd);
    curl_easy_setopt(curl, CURLOPT_CHUNK_DATA, &context);
    if (options.require_secure)
    {
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    CURLcode curl_result = curl_easy_perform(curl);
    context.current_file.close();
    curl_easy_setopt(curl, CURLOPT_WILDCARDMATCH, 0L);
    curl_easy_setopt(curl, CURLOPT_CHUNK_BGN_FUNCTION, nullptr);
    curl_easy_setopt(curl, CURLOPT_CHUNK_END_FUNCTION, nullptr);
    curl_easy_setopt(curl, CURLOPT_CHUNK_DATA, nullptr);

    Response response(session.Complete(curl_result));
    if (request_control->canceled.load())
    {
        RemoveFilesIfExist(context.created_files);
        MarkCancelledResponse(response, "Ftp downloading canceled");
    }
    else if (curl_result != CURLE_OK || context.file_open_failed || context.write_failed)
    {
        RemoveFilesIfExist(context.created_files);
    }

    return response;
}
