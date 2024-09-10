#include "DateTimeService.h"

namespace ezhttp
{
    std::chrono::system_clock::time_point DateTimeService::GetNow()
    {
        return std::chrono::system_clock::now();
    }
}
