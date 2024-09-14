#include "HostCacheItem.h"

namespace ezhttp
{
    HostCacheItem::HostCacheItem(std::chrono::system_clock::time_point last_access) :
        last_access_(last_access)
    {
    }

    std::vector<CurlHolderCacheItem>& HostCacheItem::get_cache()
    {
        return cache_;
    }

    std::chrono::system_clock::time_point HostCacheItem::get_last_access() const
    {
        return last_access_;
    }

    void HostCacheItem::set_last_access(std::chrono::system_clock::time_point last_access)
    {
        last_access_ = last_access;
    }
}
