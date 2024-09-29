include(FetchContent)

FetchContent_Declare(
        GoogleTest
        GIT_REPOSITORY          https://github.com/google/googletest.git
        GIT_TAG                 v1.15.2
        USES_TERMINAL_DOWNLOAD  TRUE
)

FetchContent_MakeAvailable(GoogleTest)

add_library(gtest_int INTERFACE)
target_link_libraries(gtest_int INTERFACE gtest)
target_include_directories(gtest_int INTERFACE ${GoogleTest_SOURCE_DIR}/include)

add_library(GTest::GTest ALIAS gtest_int)

# Group under the "tests/gtest" project folder in IDEs such as Visual Studio.
set_property(TARGET gtest PROPERTY FOLDER "tests/gtest")
set_property(TARGET gtest_main PROPERTY FOLDER "tests/gtest")