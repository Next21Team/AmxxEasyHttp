set(TARGET_NAME easy_http)
set(OUTPUT_NAME easy_http_amxx)

find_package(Threads REQUIRED)
find_package(HLSDK REQUIRED)
find_package(metamod REQUIRED)
find_package(ReHLDS REQUIRED)

# Both the module and its ReHLDS adapter use these compiler settings.
add_compile_definitions(NOMINMAX HAVE_STDINT_H NO_MSVC8_AUTO_COMPAT)
if (UNIX)
    add_compile_definitions(LINUX)
else ()
    add_compile_definitions(_WIN32 WIN32 _WIN32_WINNT=0x0600 WINVER=0x0600)
endif ()

if (GCC)
    add_compile_options(-fno-rtti -Werror)
elseif (MSVC)
    add_compile_options(/GR- /WX /EHsc $<$<CONFIG:Release>:/Zi>)
endif ()

add_library(rehlds_frame OBJECT RehldsPausedFrame.cpp)
target_link_libraries(rehlds_frame PRIVATE ReHLDS::ReHLDS_includes)
set_target_properties(rehlds_frame PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (MSVC)
    target_compile_options(rehlds_frame PRIVATE /source-charset:.1252)
endif ()

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
        $<TARGET_OBJECTS:rehlds_frame>
        RehldsPausedFrame.h
        EasyHttpModule.cpp
        EasyHttpModule.h
        easy_http/EasyHttpInterface.h
        easy_http/EasyHttp.cpp
        easy_http/EasyHttp.h
        easy_http/EasyHttpOptionsBuilder.h
        easy_http/Response.h
        easy_http/RequestOptions.h
        easy_http/RequestMethod.h
        easy_http/RequestControl.h
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
        utils/TraceLog.cpp
        utils/TraceLog.h
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
        ${CMAKE_DL_LIBS}
        cpr::cpr
        parson::parson
        HLSDK::HLSDK_includes
        metamod::metamod
)
add_library(easy_http::easy_http ALIAS ${TARGET_NAME})

target_include_directories(${TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_link_options(${TARGET_NAME} PRIVATE
            $<$<CONFIG:Release>:/LTCG>            # enables whole program optimization on link-time
            $<$<CONFIG:Release>:/INCREMENTAL:NO>  # /INCREMENTAL is not compatible with /LTCG
            $<$<CONFIG:Release>:/DEBUG>           # enables PDB generation
            $<$<CONFIG:Release>:/OPT:REF>         # eliminates functions and data that are never referenced
            $<$<CONFIG:Release>:/OPT:ICF>         # enables COMDAT folding
    )
endif ()

set_target_properties(${TARGET_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${OUTPUT_NAME}$<$<BOOL:${UNIX}>:_i386>"
        PDB_NAME ${OUTPUT_NAME}
)

if (GCC)
    set_target_properties(${TARGET_NAME} PROPERTIES
            LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/AmxxEasyHttp.version
            LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/AmxxEasyHttp.version"
    )
endif ()
