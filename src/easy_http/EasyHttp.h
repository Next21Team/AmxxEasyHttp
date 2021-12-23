#pragma once

#include "EasyHttpInterface.h"
#include "EasyHttpOptionsBuilder.h"
#include "concurrencpp/concurrencpp.h"
#include <unordered_map>

namespace ezhttp
{
    class EasyHttp : public EasyHttpInterface
    {
        concurrencpp::runtime runtime_;
        std::shared_ptr<concurrencpp::manual_executor> update_executor_;
        std::shared_ptr<concurrencpp::thread_pool_executor> request_executor_;

    public:
        explicit EasyHttp();

        std::shared_ptr<RequestControl> SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, std::shared_ptr<RequestCallback> on_complete) override;
        void RunFrame() override;

    private:
        concurrencpp::result<void> SendRequestCoroutine(std::shared_ptr<RequestControl> request_control, RequestMethod method, cpr::Url url, std::shared_ptr<RequestCallback> on_complete,
                                                        RequestOptions options);

        static cpr::Response SendRequestCpr(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, cpr::Url url, RequestOptions options);
    };
}
