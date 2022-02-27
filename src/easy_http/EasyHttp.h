#pragma once

#include "EasyHttpInterface.h"
#include "EasyHttpOptionsBuilder.h"
#include <async++.h>
#include <vector>
#include <optional>

namespace ezhttp
{
    class EasyHttp : public EasyHttpInterface
    {
        static const int kMaxTasksExecPerFrame = 6;
        static const int kMaxThreads = 10;

        std::string ca_cert_path_;

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
        cpr::Session CreateSessionWithCommonOptions(const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options);
        cpr::Response SendRequest(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options);
        cpr::Response SendHttpRequest(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options);
        cpr::Response FtpUpload(const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options);
        cpr::Response FtpDownload(const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options);
    };
}
