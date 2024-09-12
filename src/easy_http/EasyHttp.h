#pragma once
#include <vector>

#include <async++.h>

#include "EasyHttpInterface.h"
#include "EasyHttpOptionsBuilder.h"
#include "session_cache/CprSessionCache.h"

namespace ezhttp
{
    class EasyHttp : public EasyHttpInterface
    {
        static const int kMaxTasksExecPerFrame = 6;
        static const int kMaxThreads = 10;
        static const int kMaxSessionsPerHost = kMaxThreads;
        static const int kMaxAgeConnSeconds = 118; // curl uses this value by default (https://everything.curl.dev/transfers/conn/reuse.html)

        std::string ca_cert_path_;

        CprSessionCache session_cache_;

        std::unique_ptr<async::fifo_scheduler> update_scheduler_;
        std::shared_ptr<async::threadpool_scheduler> request_scheduler_;

        std::vector<async::task<void>> tasks_;
        std::vector<std::shared_ptr<RequestControl>> requests_;

    public:
        explicit EasyHttp(std::string ca_cert_path, int threads = kMaxThreads);
        ~EasyHttp() override;

        std::shared_ptr<RequestControl> SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const ResponseCallback& on_complete) override;
        void RunFrame() override;
        int GetActiveRequestCount() override { return tasks_.size(); }
        void ForgetAllRequests() override;
        void CancelAllRequests() override;

    private:
        cpr::Response SendRequest(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options);
        void SetSessionCommonOptions(cpr::Session& session, const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options);
        cpr::Response SendHttpRequest(cpr::Session& session, const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options);
        cpr::Response FtpUpload(cpr::Session& session, const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options);
        cpr::Response FtpDownload(cpr::Session& session, const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options);
    };
}
