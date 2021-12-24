cmake_minimum_required(VERSION 3.21)
project(AmxxEasyHttp)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 23)

# common options
set(BUILD_SHARED_LIBS OFF)

# cpr options
set(ENABLE_THREADED_RESOLVER OFF)
set(BUILD_CPR_TESTS OFF)
set(USE_SYSTEM_GTEST OFF)

add_subdirectory(dep/cpr)
add_subdirectory(dep/concurrencpp)

include_directories(dep/cpr/include
        dep/concurrencpp/include
        dep/halflife/common
        dep/halflife/dlls
        dep/halflife/engine
        dep/halflife/public
        dep/metamod
        src/public_amxmodx)

add_library(AmxxEasyHttp SHARED
        src/sdk/amxxmodule.cpp
        src/sdk/amxxmodule.h
        src/sdk/moduleconfig.h
        src/easy_http/EasyHttpInterface.h
        src/easy_http/EasyHttp.cpp
        src/easy_http/EasyHttp.h
        src/easy_http/EasyHttpOptionsBuilder.cpp
        src/easy_http/EasyHttpOptionsBuilder.h
        src/module.cpp)

#-----------
# properties
#-----------

if (MSVC AND CMAKE_BUILD_TYPE MATCHES Release)
    # Tell linker to include symbol data & generate map file
    set_target_properties(${PROJECT_NAME} PROPERTIES
            LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF /MAP:${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map")

    # Set file name & location
    set_target_properties(${PROJECT_NAME} PROPERTIES
            COMPILE_PDB_NAME ${PROJECT_NAME}
            COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR})
endif()

#-----------
# definitions
#-----------

target_compile_definitions(${PROJECT_NAME} PUBLIC
        NOMINMAX
        HAVE_STDINT_H
        NO_MSVC8_AUTO_COMPAT)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _DEBUG SOURCE_DEBUG)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC NDEBUG)
endif()

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        _WINDOWS
        _WIN32
        _WIN32_WINNT=0x0600
        WIN32
        WINVER=0x0600)
endif()

#-----------
# libraries
#-----------

target_link_libraries(${PROJECT_NAME}
        cpr::cpr
        CURL::libcurl
        concurrencpp::concurrencpp)

#-----------
# post process
#-----------

if (WIN32 AND DEFINED ENV{COPY_TO_PATH})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> $ENV{COPY_TO_PATH}
            COMMENT "Copying '${PROJECT_NAME}' library to '$ENV{COPY_TO_PATH}'")
endif()