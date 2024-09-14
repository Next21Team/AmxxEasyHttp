#include "CurlHolderCacheItem.h"

#include <utility>

namespace ezhttp
{
    CurlHolderCacheItem::CurlHolderCacheItem(std::shared_ptr<cpr::CurlHolder> curl_holder, std::chrono::system_clock::time_point last_use) :
        curl_holder_(std::move(curl_holder)),
        last_use_(last_use)
    {
    }

    std::shared_ptr<cpr::CurlHolder> CurlHolderCacheItem::get_curl_holder() const
    {
        return curl_holder_;
    }

    std::chrono::system_clock::time_point CurlHolderCacheItem::get_last_use() const
    {
        return last_use_;
    }
}