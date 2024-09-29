#pragma once
#include <utility>
#include <optional>

#include <cpr/cpr.h>

namespace ezhttp
{
    struct RequestOptions
    {
        std::optional<cpr::UserAgent> user_agent{};
        std::optional<cpr::Parameters> url_parameters{};
        std::optional<cpr::Payload> form_payload{};
        std::optional<cpr::Body> body{};
        std::optional<cpr::Header> header{};
        std::optional<cpr::Cookies> cookies{};
        std::optional<cpr::Timeout> timeout{};
        std::optional<cpr::ConnectTimeout> connect_timeout{};
        std::optional<std::string> proxy_url{};
        std::optional<std::pair<std::string, std::string>> proxy_auth{};
        std::optional<cpr::Authentication> auth{};
        bool require_secure{};
        std::optional<std::string> file_path{}; // for ftp and multipart/form-data in future
    };
}
