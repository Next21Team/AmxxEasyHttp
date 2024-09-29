#pragma once
#include "RequestProgress.hpp"

namespace ezhttp
{
    class RequestControlInterface
    {
    public:
        virtual ~RequestControlInterface() = default;

        virtual void CancelRequest() = 0;

        [[nodiscard]] virtual bool IsCompleted() = 0;
        [[nodiscard]] virtual bool IsCancelled() = 0;
        [[nodiscard]] virtual RequestProgress GetProgress() = 0;
    };
}
