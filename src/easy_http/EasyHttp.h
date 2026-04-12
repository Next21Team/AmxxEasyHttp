#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

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

        struct PendingRequest
        {
            std::shared_ptr<RequestControl> request_control;
            RequestMethod method;
            cpr::Url url;
            RequestOptions options;
            ResponseCallback on_complete;
        };

        struct CompletedRequest
        {
            std::shared_ptr<RequestControl> request_control;
            Response response;
            ResponseCallback on_complete;
        };

        std::mutex pending_requests_mutex_;
        std::condition_variable pending_requests_cv_;
        std::deque<PendingRequest> pending_requests_;

        std::mutex completed_requests_mutex_;
        std::deque<CompletedRequest> completed_requests_;

        std::vector<std::thread> worker_threads_;
        std::vector<std::shared_ptr<RequestControl>> requests_;
        bool stop_requested_{false};

    public:
        explicit EasyHttp(std::string ca_cert_path, int threads = kMaxThreads);
        ~EasyHttp() override;

        std::shared_ptr<RequestControl> SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const ResponseCallback &on_complete) override;
        void RunFrame() override;
        int GetActiveRequestCount() override { return static_cast<int>(requests_.size()); }
        void ForgetAllRequests() override;
        void CancelAllRequests() override;

    private:
        void WorkerLoop();
        bool TryPopCompletedRequest(CompletedRequest &completed_request);
        Response CreateErrorResponse(const cpr::Url &url, cpr::ErrorCode code, std::string message) const;
        Response SendRequest(const std::shared_ptr<RequestControl> &request_control, RequestMethod method, const cpr::Url &url, const RequestOptions &options);
        void SetSessionCommonOptions(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url &url, const RequestOptions &options);
        Response SendHttpRequest(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, RequestMethod method, const cpr::Url &url, const RequestOptions &options);
        Response FtpUpload(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url &url, const RequestOptions &options);
        Response FtpDownload(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url &url, const RequestOptions &options);
        Response FtpDownloadSingle(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url &url, const RequestOptions &options);
        Response FtpDownloadWildcard(cpr::Session &session, const std::shared_ptr<RequestControl> &request_control, const cpr::Url &url, const RequestOptions &options);
    };
}
