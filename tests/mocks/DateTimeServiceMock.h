#pragma once
#include <chrono>

#include <easy_http/datetime_service/DateTimeServiceInterface.h>

class DateTimeServiceMock : public ezhttp::DateTimeServiceInterface
{
    std::chrono::system_clock::time_point now_;

public:
    std::chrono::system_clock::time_point GetNow() override
    {
        return now_;
    }

    void SetNow(std::chrono::seconds now)
    {
        now_ = std::chrono::system_clock::time_point(now);
    }
};
