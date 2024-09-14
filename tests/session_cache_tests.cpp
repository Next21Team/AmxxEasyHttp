#include <chrono>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <easy_http/session_cache/CprSessionCache.h>

#include "CurlHolderComparer.h"
#include "mocks/CprSessionFactoryMock.h"
#include "mocks/DateTimeServiceMock.h"

using namespace std::chrono_literals;

using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Truly;

TEST(CprSessionCacheTest, AllSessionsUnexpired)
{
    auto cpr_session_factory = std::make_shared<CprSessionFactoryMock>();
    auto date_time_service = std::make_shared<DateTimeServiceMock>();

    ezhttp::CprSessionCache session_cache(cpr_session_factory, date_time_service, 30s, 3);

    // Expect the cache to be empty (all calls to the factory must have an empty CurlHolder)
    EXPECT_CALL(*cpr_session_factory, CreateSession(IsNull())).Times(4);

    date_time_service->SetNow(1s);
    auto session1 = session_cache.GetSession("https://google.com/page1.html");

    date_time_service->SetNow(2s);
    auto session2 = session_cache.GetSession("https://google.com/page2.html");

    date_time_service->SetNow(3s);
    auto session3 = session_cache.GetSession("https://google.com/page3.html");

    date_time_service->SetNow(4s);
    auto session4 = session_cache.GetSession("https://google.com/page4.html");

    session_cache.ReturnSession(*session1);
    session_cache.ReturnSession(*session2);
    session_cache.ReturnSession(*session3);
    session_cache.ReturnSession(*session4);

    // At this moment we're expecting cache was filled with 3 elements, curl holders from session2, session3, session4

    EXPECT_CALL(*cpr_session_factory, CreateSession(Truly(CurlHolderComparer(session4->GetCurlHolder()))));
    date_time_service->SetNow(11s);
    auto session5 = session_cache.GetSession("https://google.com/page5.html");

    EXPECT_CALL(*cpr_session_factory, CreateSession(Truly(CurlHolderComparer(session3->GetCurlHolder()))));
    date_time_service->SetNow(12s);
    auto session6 = session_cache.GetSession("https://google.com/page6.html");

    EXPECT_CALL(*cpr_session_factory, CreateSession(Truly(CurlHolderComparer(session2->GetCurlHolder()))));
    date_time_service->SetNow(13s);
    auto session7 = session_cache.GetSession("https://google.com/page7.html");

    EXPECT_CALL(*cpr_session_factory, CreateSession(IsNull()));
    date_time_service->SetNow(14s);
    auto session8 = session_cache.GetSession("https://google.com/page8.html");
}

TEST(CprSessionCacheTest, AllSessionsExpired)
{
    auto cpr_session_factory = std::make_shared<CprSessionFactoryMock>();
    auto date_time_service = std::make_shared<DateTimeServiceMock>();

    ezhttp::CprSessionCache session_cache(cpr_session_factory, date_time_service, 30s, 3);

    // Expect the cache to be empty (all calls to the factory must have an empty CurlHolder)
    EXPECT_CALL(*cpr_session_factory, CreateSession(IsNull())).Times(4);

    date_time_service->SetNow(1s);
    auto session1 = session_cache.GetSession("https://google.com/page1.html");

    date_time_service->SetNow(2s);
    auto session2 = session_cache.GetSession("https://google.com/page2.html");

    date_time_service->SetNow(3s);
    auto session3 = session_cache.GetSession("https://google.com/page3.html");

    date_time_service->SetNow(4s);
    auto session4 = session_cache.GetSession("https://google.com/page4.html");

    session_cache.ReturnSession(*session1);
    session_cache.ReturnSession(*session2);
    session_cache.ReturnSession(*session3);
    session_cache.ReturnSession(*session4);

    // At this moment we're expecting cache was filled with 3 elements, curl holders from session2, session3, session4

    // Set the current time so that all sessions are expired
    date_time_service->SetNow(35s);

    EXPECT_CALL(*cpr_session_factory, CreateSession(IsNull()));
    auto session5 = session_cache.GetSession("https://google.com/page5.html");
}

TEST(CprSessionCacheTest, FreeSpaceForSessionOnReturn)
{
    auto cpr_session_factory = std::make_shared<CprSessionFactoryMock>();
    auto date_time_service = std::make_shared<DateTimeServiceMock>();

    ezhttp::CprSessionCache session_cache(cpr_session_factory, date_time_service, 30s, 5);

    // Expect the cache to be empty (all calls to the factory must have an empty CurlHolder)
    EXPECT_CALL(*cpr_session_factory, CreateSession(IsNull())).Times(6);

    date_time_service->SetNow(1s);
    auto session1 = session_cache.GetSession("https://google.com/page1.html");

    date_time_service->SetNow(2s);
    auto session2 = session_cache.GetSession("https://google.com/page2.html");

    date_time_service->SetNow(3s);
    auto session3 = session_cache.GetSession("https://google.com/page3.html");

    date_time_service->SetNow(4s);
    auto session4 = session_cache.GetSession("https://google.com/page4.html");

    date_time_service->SetNow(5s);
    auto session5 = session_cache.GetSession("https://google.com/page5.html");

    date_time_service->SetNow(6s);
    auto session6 = session_cache.GetSession("https://google.com/page6.html");


    date_time_service->SetNow(10s);
    session_cache.ReturnSession(*session1);

    date_time_service->SetNow(15s);
    session_cache.ReturnSession(*session2);

    date_time_service->SetNow(35s);
    session_cache.ReturnSession(*session3);

    date_time_service->SetNow(40s);
    session_cache.ReturnSession(*session4);

    date_time_service->SetNow(45s);
    session_cache.ReturnSession(*session5);

    // At this point the cache should be full, the next call to ReturnSession will initiate the process
    // of freeing up cache space for new items by deleting all expired sessions

    date_time_service->SetNow(50s);
    session_cache.ReturnSession(*session6);

    // At this moment we're expecting cache was filled with 5 elements, curl holders from session3, session4, session5, session6
    // At this point, all sessions under 20 seconds (date_time_service->SetNow) will be considered expired

    EXPECT_CALL(*cpr_session_factory, CreateSession(Truly(CurlHolderComparer(session6->GetCurlHolder()))));
    auto session7 = session_cache.GetSession("https://google.com/page7.html");

    EXPECT_CALL(*cpr_session_factory, CreateSession(Truly(CurlHolderComparer(session5->GetCurlHolder()))));
    auto session8 = session_cache.GetSession("https://google.com/page8.html");

    EXPECT_CALL(*cpr_session_factory, CreateSession(Truly(CurlHolderComparer(session4->GetCurlHolder()))));
    auto session9 = session_cache.GetSession("https://google.com/page9.html");

    EXPECT_CALL(*cpr_session_factory, CreateSession(Truly(CurlHolderComparer(session3->GetCurlHolder()))));
    auto session10 = session_cache.GetSession("https://google.com/page10.html");

    EXPECT_CALL(*cpr_session_factory, CreateSession(IsNull()));
    auto session11 = session_cache.GetSession("https://google.com/page11.html");
}
