#pragma once
#include <string>

#include <curl/urlapi.h>

namespace ezhttp
{
    class UrlUtils
    {
        static thread_local CURLU* curl_url_;

    public:
        // Returns the host part of the url. If host cannot be obtained, returns an empty string.
        [[nodiscard]] static std::string GetHostByUrl(const std::string& url);

    private:
        static void InitializeIfNeeded();
    };
}
