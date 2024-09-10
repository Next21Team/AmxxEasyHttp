#pragma once
#include "DateTimeServiceInterface.h"

namespace ezhttp
{
    class DateTimeService : public DateTimeServiceInterface
    {
    public:
        std::chrono::system_clock::time_point GetNow() override;
    };
}