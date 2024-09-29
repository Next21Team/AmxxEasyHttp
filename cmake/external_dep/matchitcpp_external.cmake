include(FetchContent)

FetchContent_Declare(
        matchit
        GIT_REPOSITORY          https://github.com/BowenFu/matchit.cpp.git
        GIT_TAG                 62c85a91413c61880ededee1a265ecfc87eb8ecd
        USES_TERMINAL_DOWNLOAD  TRUE
)

FetchContent_MakeAvailable(matchit)

add_library(matchit_int INTERFACE)
target_link_libraries(matchit_int INTERFACE matchit)
target_include_directories(matchit_int INTERFACE ${matchit_SOURCE_DIR} ${matchit_BINARY_DIR})

add_library(matchit::matchit ALIAS matchit_int)
