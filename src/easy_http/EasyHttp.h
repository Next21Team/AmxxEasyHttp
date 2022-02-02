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

        std::unique_ptr<async::fifo_scheduler> update_scheduler_;
        std::unique_ptr<async::threadpool_scheduler> request_scheduler_;

        std::vector<async::task<void>> tasks_;

    public:
        explicit EasyHttp();
        ~EasyHttp() override;

        std::shared_ptr<RequestControl> SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const RequestCallback& on_complete) override;
        void RunFrame() override;

    private:
        static cpr::Response SendRequestCpr(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, cpr::Url url, RequestOptions options);
    };
}
