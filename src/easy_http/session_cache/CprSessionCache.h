#pragma once
#include <memory>
#include <string>
#include <chrono>

#include <cpr/session.h>

#include "../session_factory/CprSessionFactoryInterface.h"
#include "../datetime_service/DateTimeServiceInterface.h"

#include "CurlHolderCacheItem.h"

namespace ezhttp
{
    class CprSessionCache
    {
        std::shared_ptr<CprSessionFactoryInterface> session_factory_;
        std::shared_ptr<DateTimeServiceInterface> date_time_service_;
        std::chrono::seconds maxage_conn_;
        uint32_t max_sessions_per_host_;

        std::unordered_map<std::string, std::vector<CurlHolderCacheItem>> cache_{};

        std::mutex mutex_;

    public:
        CprSessionCache(std::shared_ptr<CprSessionFactoryInterface> session_factory, std::shared_ptr<DateTimeServiceInterface> date_time_service, std::chrono::seconds maxage_conn, uint32_t max_sessions_per_host);

        std::unique_ptr<cpr::Session> GetSession(const std::string& url);
        void ReturnSession(cpr::Session& session);
        void ReturnSession(const std::string& url, std::shared_ptr<cpr::CurlHolder> curl_holder);

    private:
        std::unique_ptr<cpr::Session> CreateSession(const std::string& url, std::shared_ptr<cpr::CurlHolder> curl_holder = nullptr);
        void FreeSpaceForSession(std::vector<CurlHolderCacheItem>& cache_items, std::chrono::system_clock::time_point current_time);
        bool IsSessionCacheExpired(const CurlHolderCacheItem& cache_item, std::chrono::system_clock::time_point current_time);
    };
}
