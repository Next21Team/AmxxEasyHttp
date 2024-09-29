#pragma once
#include <optional>
#include <utility>
#include <string>

#include <cpr/cpr.h>

#include "EasyHttpInterface.h"

namespace ezhttp
{
    class EasyHttpOptionsBuilder
    {
        RequestOptions options_;

    public:
        void SetUserAgent(const std::string& user_agent)
        {
            options_.user_agent = cpr::UserAgent(user_agent);
        }

        void AddUrlParameter(const std::string& key, const std::string& value)
        {
            if (!options_.url_parameters)
                options_.url_parameters = cpr::Parameters{};

            options_.url_parameters->Add({key, value});
        }

        void AddFormPayload(const std::string& key, const std::string& value)
        {
            if (!options_.form_payload)
                options_.form_payload = cpr::Payload{};

            options_.form_payload->Add({key, value});
        }

        void SetBody(const std::string& body)
        {
            options_.body = cpr::Body(body);
        }

        void AppendBody(const std::string& body)
        {
            if (!options_.body)
                SetBody(body);
            else
                *options_.body += body;
        }

        void SetHeader(const std::string& key, const std::string& value)
        {
            if (!options_.header)
                options_.header = cpr::Header{};

            (*options_.header)[key] = value;
        }

        void SetCookie(const std::string& key, const std::string& value)
        {
            if (!options_.cookies)
                options_.cookies = cpr::Cookies{};

            auto it = std::find_if(options_.cookies->begin(), options_.cookies->end(), [&key](const auto& item)
            {
                return item.GetName() == key;
            });
            if (it == options_.cookies->end())
            {
                options_.cookies->push_back(cpr::Cookie(key, value));
            }
            else
            {
                size_t index = it - options_.cookies->begin();
                (*options_.cookies)[index] = cpr::Cookie(key, value);
            }
        }

        void SetTimeout(int32_t timeout_ms)
        {
            options_.timeout = cpr::Timeout{timeout_ms};
        }

        void SetConnectTimeout(int32_t timeout_ms)
        {
            options_.connect_timeout = cpr::ConnectTimeout{timeout_ms};
        }

        void SetProxy(const std::string& proxy_url)
        {
            options_.proxy_url = proxy_url;
        }

        void SetProxyAuth(const std::string& user, const std::string& password)
        {
            options_.proxy_auth = {user, password};
        }

        void SetAuth(const std::string& user, const std::string& password)
        {
            options_.auth = cpr::Authentication(user, password, cpr::AuthMode::BASIC);
        }

        void SetSecure(bool secure)
        {
            options_.require_secure = secure;
        }

        void SetFilePath(const std::string& file_path)
        {
            options_.file_path = file_path;
        }

        [[nodiscard]] RequestOptions& BuildOptions() { return options_; }
    };
}
