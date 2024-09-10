#include "CprSessionCache.h"

#include <utility>

#include "../UrlUtils.h"

using namespace std::chrono_literals;

namespace ezhttp
{
    CprSessionCache::CprSessionCache(std::shared_ptr<CprSessionFactoryInterface> session_factory, std::shared_ptr<DateTimeServiceInterface> date_time_service, std::chrono::seconds maxage_conn, uint32_t max_sessions_per_host) :
        session_factory_(std::move(session_factory)),
        date_time_service_(std::move(date_time_service)),
        maxage_conn_(maxage_conn),
        max_sessions_per_host_(max_sessions_per_host)
    {
    }

    std::shared_ptr<cpr::Session> CprSessionCache::GetSession(const std::string& url)
    {
        if (max_sessions_per_host_ == 0)
            return CreateSession(url);

        std::string host = UrlUtils::GetHostByUrl(url);
        auto current_time = date_time_service_->GetNow();

        if (host.empty())
            return nullptr;

        auto curl_cache_it = cache_.find(host);
        if (curl_cache_it == cache_.end())
            return CreateSession(url);

        auto& cache_items = curl_cache_it->second;

        if (cache_items.empty())
            return CreateSession(url);

        // The newest session is stored at the end of the vector, if it is expired, all previous sessions are also expired
        if (IsSessionCacheExpired(cache_items.back(), current_time))
        {
            cache_items.clear();
            return CreateSession(url);
        }

        auto curl_holder = cache_items.back().get_curl_holder();
        cache_items.pop_back();

        return CreateSession(url, curl_holder);
    }

    void CprSessionCache::ReturnSession(cpr::Session& session)
    {
        ReturnSession(session.GetFullRequestUrl(), session.GetCurlHolder());
    }

    void CprSessionCache::ReturnSession(const std::string& url, std::shared_ptr<cpr::CurlHolder> curl_holder)
    {
        if (max_sessions_per_host_ == 0)
            return;

        std::string host = UrlUtils::GetHostByUrl(url);
        auto current_time = date_time_service_->GetNow();

        if (host.empty())
            return;

        auto curl_cache_it = cache_.find(host);
        if (curl_cache_it == cache_.end())
        {
            cache_.emplace(host, std::vector{ CurlHolderCacheItem(curl_holder, current_time) });
            return;
        }

        auto& cache_items = curl_cache_it->second;

        FreeSpaceForSession(cache_items, current_time);

        cache_items.emplace_back(curl_holder, current_time);
    }

    std::shared_ptr<cpr::Session> CprSessionCache::CreateSession(const std::string& url, std::shared_ptr<cpr::CurlHolder> curl_holder)
    {
        auto session = session_factory_->CreateSession(curl_holder);
        session->SetUrl(url);

        return session;
    }

    void CprSessionCache::FreeSpaceForSession(std::vector<CurlHolderCacheItem>& cache_items, std::chrono::system_clock::time_point current_time)
    {
        if (cache_items.size() < max_sessions_per_host_)
            return;

        auto first_non_expired_it = std::lower_bound(cache_items.begin(), cache_items.end(), current_time,
                                                     [this](const CurlHolderCacheItem& cache_item, std::chrono::system_clock::time_point time)
                                                     {
                                                         return IsSessionCacheExpired(cache_item, time);
                                                     });

        if (first_non_expired_it != cache_items.begin())
        {
            cache_items.erase(cache_items.begin(), first_non_expired_it);
        }
        else if (cache_items.size() >= max_sessions_per_host_)
        {
            auto newest_of_the_olds_it = cache_items.begin() + (cache_items.size() - max_sessions_per_host_) + 1;
            cache_items.erase(cache_items.begin(), newest_of_the_olds_it);
        }
    }

    bool CprSessionCache::IsSessionCacheExpired(const CurlHolderCacheItem& cache_item, std::chrono::system_clock::time_point current_time)
    {
        return cache_item.get_last_use() + maxage_conn_ <= current_time;
    }
}
