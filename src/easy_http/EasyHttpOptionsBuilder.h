#pragma once

#include "EasyHttpInterface.h"
#include "cpr/cpr.h"
#include <optional>
#include <utility>
#include <string>

namespace ezhttp
{
    class EasyHttpOptionsBuilder
    {
        RequestOptions options_;

    public:
        void SetUserAgent(const std::string& user_agent);
        void AddUrlParameter(const std::string& key, const std::string& value);
        void AddFormPayload(const std::string& key, const std::string& value);
        void SetBody(const std::string& body);
        void SetHeader(const std::string& key, const std::string& value);
        void SetCookie(const std::string& key, const std::string& value);
        void SetTimeout(int32_t timeout_ms);
        void SetProxy(const std::string& proxy_url);
        void SetProxyAuth(const std::string& user, const std::string& password);
        void SetAuth(const std::string& user, const std::string& password);

        [[nodiscard]] RequestOptions& GetOptions() { return options_; }
    };
}