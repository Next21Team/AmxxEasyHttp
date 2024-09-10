#pragma once
#include "CurlHolderCacheItem.h"

namespace ezhttp
{
    class HostCacheItem
    {
        std::vector<CurlHolderCacheItem> cache_;
        std::chrono::system_clock::time_point last_access_;

    public:
        HostCacheItem(std::chrono::system_clock::time_point last_access);

        std::vector<CurlHolderCacheItem>& get_cache();
        std::chrono::system_clock::time_point get_last_access() const;
        void set_last_access(std::chrono::system_clock::time_point last_access);
    };
}