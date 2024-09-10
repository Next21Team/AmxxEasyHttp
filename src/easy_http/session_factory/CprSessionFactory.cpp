#include "CprSessionFactory.h"

namespace ezhttp
{
    std::shared_ptr<cpr::Session> CprSessionFactory::CreateSession(std::shared_ptr<cpr::CurlHolder> curl_holder)
    {
        if (curl_holder == nullptr)
            return std::make_shared<cpr::Session>();

        return std::make_shared<cpr::Session>(curl_holder);
    }
}
