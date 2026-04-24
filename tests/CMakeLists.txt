set(TARGET_NAME "AmxxEasyHttp-tests")

include(GoogleTest)

add_executable(${TARGET_NAME}
        easy_http_module_tests.cpp
        ftp_utils_tests.cpp
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
        easy_http::easy_http
)

gtest_discover_tests(${TARGET_NAME})
