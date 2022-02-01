#pragma once

#include "EasyHttpInterface.h"
#include "EasyHttpOptionsBuilder.h"
#include <async++.h>
#include <unordered_map>

namespace ezhttp
{
    class EasyHttp : public EasyHttpInterface
    {
        async::threadpool_scheduler request_scheduler_;
        async::fifo_scheduler update_scheduler_;

    public:
        explicit EasyHttp();

        std::shared_ptr<RequestControl> SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const RequestCallback& on_complete) override;
        void RunFrame() override;

    private:
        static cpr::Response SendRequestCpr(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, cpr::Url url, RequestOptions options);
    };
}
