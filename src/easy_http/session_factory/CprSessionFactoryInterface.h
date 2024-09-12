#pragma once
#include <cpr/session.h>

namespace ezhttp
{
    class CprSessionFactoryInterface
    {
    public:
        virtual ~CprSessionFactoryInterface() = default;
        virtual std::unique_ptr<cpr::Session> CreateSession(std::shared_ptr<cpr::CurlHolder> curl_holder) = 0;
    };
}
