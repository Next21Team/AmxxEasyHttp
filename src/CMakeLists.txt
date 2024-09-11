set(TARGET_NAME easy_http)
set(OUTPUT_NAME easy_http_amxx)

find_package(Threads REQUIRED)
find_package(HLSDK REQUIRED)
find_package(metamod REQUIRED)

if (AMXX_EASY_HTTP_BUILD_STATIC)
    set(LIBRARY_BUILD_TYPE STATIC)
    set(TARGET_LIBRARIES_SCOPE PUBLIC)
else ()
    set(LIBRARY_BUILD_TYPE SHARED)
    set(TARGET_LIBRARIES_SCOPE PRIVATE)
endif ()

add_library(${TARGET_NAME} ${LIBRARY_BUILD_TYPE}
        sdk/amxxmodule.cpp
        sdk/amxxmodule.h
        sdk/moduleconfig.h
        module.cpp
        EasyHttpModule.cpp
        EasyHttpModule.h
        easy_http/EasyHttpInterface.h
        easy_http/EasyHttp.cpp
        easy_http/EasyHttp.h
        easy_http/EasyHttpOptionsBuilder.h
        easy_http/Response.h
        easy_http/RequestOptions.h
        easy_http/RequestMethod.h
        easy_http/UrlUtils.cpp
        easy_http/UrlUtils.h
        easy_http/session_cache/CprSessionCache.cpp
        easy_http/session_cache/CprSessionCache.h
        easy_http/session_cache/CurlHolderCacheItem.cpp
        easy_http/session_cache/CurlHolderCacheItem.h
        easy_http/session_factory/CprSessionFactory.cpp
        easy_http/session_factory/CprSessionFactory.h
        easy_http/session_cache/HostCacheItem.cpp
        easy_http/session_cache/HostCacheItem.h
        easy_http/session_factory/CprSessionFactoryInterface.h
        easy_http/datetime_service/DateTimeServiceInterface.h
        easy_http/datetime_service/DateTimeService.cpp
        easy_http/datetime_service/DateTimeService.h
        utils/ContainerWithHandles.h
        utils/ftp_utils.h
        utils/ftp_utils.cpp
        utils/string_utils.h
        utils/string_utils.cpp
        utils/amxx_utils.cpp
        utils/amxx_utils.h
        json/IJsonMngr.h
        json/JsonMngr.cpp
        json/JsonMngr.h
        json/JsonNatives.cpp
        json/JsonNatives.h
)

target_link_libraries(${TARGET_NAME} ${TARGET_LIBRARIES_SCOPE}
        Threads::Threads
        Async++
        cpr::cpr
        parson::parson
        HLSDK::HLSDK_includes
        metamod::metamod
)
add_library(easy_http::easy_http ALIAS ${TARGET_NAME})

target_include_directories(${TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${TARGET_NAME} PRIVATE
        NOMINMAX
        HAVE_STDINT_H
        NO_MSVC8_AUTO_COMPAT
)

if (UNIX)
    target_compile_definitions(${TARGET_NAME} PRIVATE
            LINUX
    )
else ()
    target_compile_definitions(${TARGET_NAME} PRIVATE
            _WIN32
            _WIN32_WINNT=0x0600
            WIN32
            WINVER=0x0600
    )
endif ()

if (GCC)
    target_compile_options(${TARGET_NAME} PRIVATE
            -fno-rtti  # disable rtti to reduce the library size
            -Werror    # treat all warnings as error
            -Wno-error=attribute-warning
    )
elseif (MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE
            /GR-       # disable rtti to reduce the library size
            /WX        # treat all warnings as error
            /EHsc      # NEVER catch SEH exceptions by catch(...); c functions cannot throw exceptions
    )

    if (CMAKE_BUILD_TYPE MATCHES Release)
        target_compile_options(${TARGET_NAME} PRIVATE
                /Zi              # produce a PDB file
        )

        target_link_options(${TARGET_NAME} PRIVATE
                /LTCG            # enables whole program optimization on link-time
                /INCREMENTAL:NO  # /INCREMENTAL is not compatible with /LTCG
                /DEBUG           # enables PDB generation
                /OPT:REF         # eliminates functions and data that are never referenced
                /OPT:ICF         # enables COMDAT folding
        )
    endif ()
endif ()

set_target_properties(${TARGET_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME ${OUTPUT_NAME}
        PDB_NAME ${OUTPUT_NAME}
)

if (GCC)
    set_target_properties(${TARGET_NAME} PROPERTIES
            LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/AmxxEasyHttp.version
            LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/AmxxEasyHttp.version"
    )
endif ()
