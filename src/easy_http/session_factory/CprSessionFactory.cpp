#include "CprSessionFactory.h"

namespace ezhttp
{
    std::unique_ptr<cpr::Session> CprSessionFactory::CreateSession(std::shared_ptr<cpr::CurlHolder> curl_holder)
    {
        if (curl_holder == nullptr)
            return std::make_unique<cpr::Session>();

        return std::make_unique<cpr::Session>(curl_holder);
    }
}
