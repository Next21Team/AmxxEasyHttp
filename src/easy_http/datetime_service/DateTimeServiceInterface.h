#pragma once
#include <chrono>

namespace ezhttp
{
    class DateTimeServiceInterface
    {
    public:
        virtual ~DateTimeServiceInterface() = default;
        virtual std::chrono::system_clock::time_point GetNow() = 0;
    };
}