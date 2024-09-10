#pragma once
#include <cpr/session.h>

#include "CprSessionFactoryInterface.h"

namespace ezhttp
{
    class CprSessionFactory : public CprSessionFactoryInterface
    {
    public:
        std::shared_ptr<cpr::Session> CreateSession(std::shared_ptr<cpr::CurlHolder> curl_holder = nullptr);
    };
}
