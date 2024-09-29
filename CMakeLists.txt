cmake_minimum_required(VERSION 3.20)
project(AmxxEasyHttp
        VERSION        2.4.0
        DESCRIPTION    "HTTP, HTTPS and FTP for AMXX plugins"
        HOMEPAGE_URL   "https://github.com/Next21Team/AmxxEasyHttp"
        LANGUAGES      CXX
)

option(AMXX_EASY_HTTP_BUILD_TESTS "Set ON to build a tests (dll/so will not be built in this case)" OFF)
option(AMXX_EASY_HTTP_USE_SYSTEM_GTEST "Set ON to use GTest installed on the system (otherwise GTest will be downloaded via FetchContent)" OFF)

###
### Cmake setup
###

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # Prevent option() from removing normal variables with the same name
set(CMAKE_POLICY_DEFAULT_CMP0117 NEW) # Do not add /GR to standard compilation flags (prevents D9025 warning)

set(AMXX_EASY_HTTP_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_MODULE_PATH
        ${AMXX_EASY_HTTP_ROOT}/cmake
        ${CMAKE_MODULE_PATH}
)

if (AMXX_EASY_HTTP_BUILD_TESTS)
    set(AMXX_EASY_HTTP_BUILD_STATIC ON)
else ()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out/bin/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out/pdb/${CMAKE_BUILD_TYPE})
endif ()

set(BUILD_SHARED_LIBS OFF) # BUILD_SHARED_LIBS only affect external dependencies

###
### Compilers setup
###

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Only GCC and MSVC are supported")
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(GCC ON)
endif ()

set(CMAKE_CXX_STANDARD 17)

if (GCC)
    add_compile_options(
            -m32
    )

    add_link_options(
            -m32
            -static-libstdc++    # Static linking of the GCC C++ runtime
            -static-libgcc       # Static linking of the GCC library
    )
elseif (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # Static linking of the MSVC's C++ runtime
endif ()

###
### cpr setup
###

set(CPR_USE_SYSTEM_CURL ON)
set(CPR_ENABLE_SSL ON)
if (UNIX)
    set(CPR_FORCE_OPENSSL_BACKEND ON)
elseif (WIN32)
    set(CPR_FORCE_WINSSL_BACKEND ON)
endif ()

###
### Add dependencies and subprojects
###

include(cmake/external_dep/matchitcpp_external.cmake)
add_subdirectory(dep/cpr)
add_subdirectory(dep/asyncplusplus)
add_subdirectory(dep/parson)
add_subdirectory(src)

if (GCC)
    target_compile_options(cpr PRIVATE
            -Wno-error=attribute-warning
    )
endif ()

###
### GTest setup
###

if (AMXX_EASY_HTTP_BUILD_TESTS)
    if (AMXX_EASY_HTTP_USE_SYSTEM_GTEST)
        find_package(GTest)
    endif ()
    if (NOT AMXX_EASY_HTTP_USE_SYSTEM_GTEST OR NOT GTEST_FOUND)
        message(STATUS "Not using system gtest, using built-in googletest project instead.")
        include(cmake/external_dep/GTest_external.cmake)
    endif ()

    enable_testing()
    add_subdirectory(tests)
endif ()

###
### Add custom build targets
###

set(MAIN_TARGET_NAME easy_http)

add_custom_target(BUILD_ALL ALL)
add_dependencies(BUILD_ALL ${MAIN_TARGET_NAME})

add_custom_target(INSTALL_ALL
        COMMENT "Copying '${MAIN_TARGET_NAME}' library to '$ENV{${MAIN_TARGET_NAME}_COPY_TO_PATH}'"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${MAIN_TARGET_NAME}> $ENV{${MAIN_TARGET_NAME}_COPY_TO_PATH})

add_custom_target(BUILD_AND_INSTALL_ALL ALL)
add_dependencies(BUILD_AND_INSTALL_ALL BUILD_ALL)
add_dependencies(BUILD_AND_INSTALL_ALL INSTALL_ALL)
