if (TARGET CURL::libcurl)
    return()
endif()

find_path(CURL_INCLUDE_DIR
        NAMES curl/curl.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/curl/include NO_DEFAULT_PATH)

find_library(CURL_LIBRARY
        NAMES libcurl_a.lib libcurl.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/curl/lib NO_DEFAULT_PATH)

find_library(CURL_LIBRARY_DEBUG
        NAMES libcurl_a_debug.lib libcurl.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/curl/lib NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CURL REQUIRED_VARS CURL_LIBRARY CURL_LIBRARY_DEBUG CURL_INCLUDE_DIR)

include(CMakeFindDependencyMacro)
find_dependency(ZLIB)
if (UNIX)
    find_dependency(cares)
    find_dependency(OpenSSL)
endif()

set(_supported_components HTTP HTTPS SSL FTP FTPS)
foreach(_comp ${curl_FIND_COMPONENTS})
    message(_comp)
    if (NOT ";${_supported_components};" MATCHES ";${_comp};")
        set(CURL_FOUND False)
        set(CURL_NOT_FOUND_MESSAGE "Unsupported component: ${_comp}")
    endif()
endforeach()

add_library(CURL::libcurl STATIC IMPORTED GLOBAL)
set_property(TARGET CURL::libcurl APPEND PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET CURL::libcurl APPEND PROPERTY IMPORTED_CONFIGURATIONS Debug)
set_target_properties(CURL::libcurl PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS "CURL_STATICLIB"
    INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}"

    IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;RC"
    IMPORTED_LOCATION_RELEASE "${CURL_LIBRARY}"
    INTERFACE_LINK_LIBRARIES_RELEASE "$<$<PLATFORM_ID:Linux>:cares openssl> ZLIB"

    IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;RC"
    IMPORTED_LOCATION_DEBUG "${CURL_LIBRARY_DEBUG}"
    INTERFACE_LINK_LIBRARIES_DEBUG "$<$<PLATFORM_ID:Linux>:cares openssl> ZLIB"
)

unset(CURL_LIBRARY CACHE)
unset(CURL_LIBRARY_DEBUG CACHE)
