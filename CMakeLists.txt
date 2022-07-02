cmake_minimum_required(VERSION 3.18)
project(AmxxEasyHttp)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)

set(AMXX_EASY_HTTP_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Add "cmake" to module path
set(CMAKE_MODULE_PATH
    ${AMXX_EASY_HTTP_ROOT}/cmake
    ${CMAKE_MODULE_PATH}
)

if (UNIX)
    set(CMAKE_C_FLAGS -m32)
    set(CMAKE_CXX_FLAGS -m32)
endif()

# common options
set(BUILD_SHARED_LIBS OFF)
set(USE_SYSTEM_GTEST OFF)
if (UNIX)
    set(CMAKE_THREAD_PREFER_PTHREAD ON)
endif()

# cpr options
set(CPR_FORCE_USE_SYSTEM_CURL ON) # system curl will be overridden by our embedded curl module
set(CPR_USE_SYSTEM_GTEST OFF)
set(BUILD_CPR_TESTS OFF)
set(CPR_ENABLE_SSL ON)

find_package(Threads REQUIRED)
add_subdirectory(dep/cpr)
add_subdirectory(dep/asyncplusplus)

add_library(${PROJECT_NAME} SHARED
    src/sdk/amxxmodule.cpp
    src/sdk/amxxmodule.h
    src/sdk/moduleconfig.h
    src/easy_http/EasyHttpInterface.h
    src/easy_http/EasyHttp.cpp
    src/easy_http/EasyHttp.h
    src/easy_http/EasyHttpOptionsBuilder.h
    src/utils/ContainerWithHandles.h
    src/utils/ftp_utils.h
    src/utils/ftp_utils.cpp
    src/utils/string_utils.h
    src/utils/string_utils.cpp
    src/module.cpp
    src/undef_metamod.h
    src/EasyHttpModule.cpp
    src/EasyHttpModule.h
)

target_include_directories(${PROJECT_NAME} PUBLIC
    dep/cpr/include
    dep/asyncplusplus/include
    dep/halflife/common
    dep/halflife/dlls
    dep/halflife/engine
    dep/halflife/public
    dep/metamod
    src/sdk
    src
)

#-----------
# properties
#-----------

if (UNIX)
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti)

    if (CMAKE_BUILD_TYPE MATCHES Release)
        target_compile_options(${PROJECT_NAME} PRIVATE -O2)
    endif()
endif()

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /GR-)

    if (CMAKE_BUILD_TYPE MATCHES Release)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zi /O2)

        # Tell linker to include symbol data & generate map file
        set_target_properties(${PROJECT_NAME} PROPERTIES
                LINK_FLAGS "/LTCG  /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF /MAP:${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map"
        )

        # Set file name & location
        set_target_properties(${PROJECT_NAME} PROPERTIES
                COMPILE_PDB_NAME ${PROJECT_NAME}
                COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
        )
    endif()
endif()

#-----------
# definitions
#-----------

target_compile_definitions(${PROJECT_NAME} PUBLIC
    LIBASYNC_STATIC
    NOMINMAX
    HAVE_STDINT_H
    NO_MSVC8_AUTO_COMPAT
)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        _WINDOWS
        _WIN32
        _WIN32_WINNT=0x0600
        WIN32
        WINVER=0x0600
    )
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        LINUX
    )
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _DEBUG SOURCE_DEBUG)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC NDEBUG)
endif()

#-----------
# libraries
#-----------

target_link_libraries(${PROJECT_NAME}
    Threads::Threads
    Async++
    cpr::cpr
    CURL::libcurl
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME}
        Ws2_32.lib
        Crypt32.lib
        Wldap32.lib
        Normaliz.lib
        ZLIB::ZLIB
    )
else()
    target_link_libraries(${PROJECT_NAME}
        OPENSSL::libssl    #todo all curl dependencies list move to variable
        OPENSSL::libcrypto
        ZLIB::ZLIB
        CARES::libcares
        -static-libstdc++
        -static-libgcc
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/AmxxEasyHttp.version
        LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/AmxxEasyHttp.version")
endif()

#-----------
# post process
#-----------

if (DEFINED ENV{${PROJECT_NAME}_COPY_TO_PATH})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> $ENV{${PROJECT_NAME}_COPY_TO_PATH}
            COMMENT "Copying '${PROJECT_NAME}' library to '$ENV{${PROJECT_NAME}_COPY_TO_PATH}'"
    )
endif()
