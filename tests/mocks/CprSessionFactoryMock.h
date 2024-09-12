#pragma once
#include <utility>

#include <gmock/gmock.h>
#include <easy_http/session_factory/CprSessionFactory.h>

class CprSessionFactoryMock : public ezhttp::CprSessionFactoryInterface
{
    ezhttp::CprSessionFactory cpr_session_factory_;

public:
    CprSessionFactoryMock()
    {
        ON_CALL(*this, CreateSession).WillByDefault([this](std::shared_ptr<cpr::CurlHolder> curl_holder)
        {
            return cpr_session_factory_.CreateSession(std::move(curl_holder));
        });
    }

    MOCK_METHOD(std::unique_ptr<cpr::Session>, CreateSession, (std::shared_ptr<cpr::CurlHolder> curl_holder), (override));
};
