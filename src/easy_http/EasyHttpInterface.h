#pragma once

#include "cpr/cpr.h"
#include <functional>
#include <utility>
#include <optional>

namespace ezhttp
{
    using ResponseCallback = std::function<void(cpr::Response)>;

    enum class RequestMethod
    {
        HttpGet,
        HttpPost,
        FtpUpload,
        FtpDownload
    };

    struct RequestOptions
    {
        std::optional<cpr::UserAgent> user_agent;
        std::optional<cpr::Parameters> url_parameters;
        std::optional<cpr::Payload> form_payload;
        std::optional<cpr::Body> body;
        std::optional<cpr::Header> header;
        std::optional<cpr::Cookies> cookies;
        std::optional<cpr::Timeout> timeout;
        std::optional<cpr::ConnectTimeout> connect_timeout;
        std::optional<std::string> proxy_url;
        std::optional<std::pair<std::string, std::string>> proxy_auth;
        std::optional<cpr::Authentication> auth;
        bool require_secure = false;
        std::optional<std::string> file_path; // for ftp and multipart/form-data in future
    };

    struct RequestControl
    {
        std::mutex control_mutex;

        struct Progress
        {
            int32_t download_total;
            int32_t download_now;
            int32_t upload_total;
            int32_t upload_now;
        };

        bool completed{};
        bool forgotten{};
        bool canceled{};
        Progress progress{};
    };

    class EasyHttpInterface
    {
    public:
        virtual ~EasyHttpInterface() = default;

        virtual std::shared_ptr<RequestControl> SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const ResponseCallback& on_complete) = 0;
        virtual void RunFrame() = 0;
        virtual int GetActiveRequestCount() = 0;

        // No callback functions will be called for all current requests
        virtual void ForgetAllRequests() = 0;

        // All requests will be interrupted as soon as possible
        virtual void CancelAllRequests() = 0;
    };
}
