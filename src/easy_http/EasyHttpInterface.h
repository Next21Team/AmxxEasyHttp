#pragma once

#include "cpr/cpr.h"
#include <amxxmodule.h>
#include <functional>
#include <utility>
#include <optional>

namespace ezhttp
{
    using RequestCallback = std::function<void(cpr::Response)>;

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
        std::optional<std::vector<cell>> user_data;
        bool require_secure = false;
        std::optional<std::string> file_path; // for ftp and multipart/form-data in future
    };

    struct RequestControl
    {
        struct Progress
        {
            int32_t download_total;
            int32_t download_now;
            int32_t upload_total;
            int32_t upload_now;
        };

        std::atomic_bool canceled{};
        std::atomic<Progress> progress{};
    };

    class EasyHttpInterface
    {
    public:
        virtual ~EasyHttpInterface() = default;

        virtual std::shared_ptr<RequestControl> SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const RequestCallback& on_complete) = 0;
        virtual void RunFrame() = 0;
    };
}
