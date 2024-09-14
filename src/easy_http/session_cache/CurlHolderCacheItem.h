#pragma once
#include <chrono>
#include <memory>

#include <cpr/session.h>

namespace ezhttp
{
    class CurlHolderCacheItem
    {
        std::shared_ptr<cpr::CurlHolder> curl_holder_;
        std::chrono::system_clock::time_point last_use_;

    public:
        CurlHolderCacheItem(std::shared_ptr<cpr::CurlHolder> curl_holder, std::chrono::system_clock::time_point last_use);

        std::shared_ptr<cpr::CurlHolder> get_curl_holder() const;
        std::chrono::system_clock::time_point get_last_use() const;
    };
}