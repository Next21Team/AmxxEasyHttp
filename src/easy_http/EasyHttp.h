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
        const int kMaxTasksExecPerFrame = 10;
        const int kMaxThreads = 10;

        std::string ca_cert_path_;

        std::unique_ptr<async::fifo_scheduler> update_scheduler_;
        std::unique_ptr<async::threadpool_scheduler> request_scheduler_;

        std::vector<async::task<void>> tasks_;

    public:
        explicit EasyHttp(std::string ca_cert_path);
        ~EasyHttp() override;

        std::shared_ptr<RequestControl> SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const RequestCallback& on_complete) override;
        void RunFrame() override;

    private:
        cpr::Session CreateSession(const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url);
        cpr::Response SendRequest(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options);
        cpr::Response SendHttpRequest(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, const cpr::Url& url, const RequestOptions& options);
        cpr::Response FtpUpload(const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options);
        cpr::Response FtpDownload(const std::shared_ptr<RequestControl>& request_control, const cpr::Url& url, const RequestOptions& options);
    };
}
