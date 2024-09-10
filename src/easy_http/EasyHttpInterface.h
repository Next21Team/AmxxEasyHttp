#pragma once
#include <functional>

#include "Response.h"
#include "RequestOptions.h"
#include "RequestMethod.h"

namespace ezhttp
{
    class EasyHttpInterface
    {
    public:
        using ResponseCallback = std::function<void(Response)>;

    public:
        virtual ~EasyHttpInterface() = default;

        virtual std::shared_ptr<RequestControl> SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const ResponseCallback& on_complete) = 0;
        virtual void RunFrame() = 0;
        virtual int GetActiveRequestCount() = 0;

        // No callback functions will be called for all current requests
        virtual void ForgetAllRequests() = 0;

        // All requests will be interrupted as soon as possible
        virtual void CancelAllRequests() = 0;
    };
}
