set(TARGET_NAME "AmxxEasyHttp-tests")

include(GoogleTest)

add_executable(${TARGET_NAME}
        session_cache_tests.cpp
        CurlHolderComparer.h
        mocks/CprSessionFactoryMock.h
        mocks/DateTimeServiceMock.h
)

target_link_libraries(${TARGET_NAME} PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        cpr::cpr
        easy_http::easy_http
)

gtest_discover_tests(${TARGET_NAME})
