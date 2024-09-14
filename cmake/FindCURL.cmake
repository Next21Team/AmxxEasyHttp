if (TARGET CURL::libcurl)
    return()
endif()

include(FindPackageHandleStandardArgs)
include(CMakeFindDependencyMacro)

###
### Find includes and libraries
###

find_path(CURL_INCLUDE_DIR
        NAMES curl/curl.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/curl/include
        NO_DEFAULT_PATH
        NO_CACHE
)

find_library(CURL_LIBRARY
        NAMES libcurl_a.lib libcurl.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/curl/lib
        NO_DEFAULT_PATH
        NO_CACHE
)

find_library(CURL_LIBRARY_DEBUG
        NAMES libcurl_a_debug.lib libcurl.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/curl/lib
        NO_DEFAULT_PATH
        NO_CACHE
)

find_package_handle_standard_args(CURL REQUIRED_VARS CURL_LIBRARY CURL_LIBRARY_DEBUG CURL_INCLUDE_DIR)

# Validate supported components
if (CURL_FIND_COMPONENTS)
    set(CURL_SUPPORTED_PROTOCOLS HTTP HTTPS SSL FTP FTPS)

    foreach(component IN LISTS CURL_SUPPORTED_PROTOCOLS)
        set(CURL_${component}_FOUND FALSE)
    endforeach()

    foreach(component ${CURL_FIND_COMPONENTS})
        if (";${CURL_SUPPORTED_PROTOCOLS};" MATCHES ";${component};")
            set(CURL_${component}_FOUND TRUE)
        else ()
            message(FATAL_ERROR "CURL: Required protocol ${component} is not found")
        endif()
    endforeach()
endif ()

###
### Find dependencies
###

find_dependency(ZLIB REQUIRED)

if (UNIX)
    find_dependency(cares REQUIRED)
    find_dependency(OpenSSL REQUIRED)
endif()

###
### Setup library
###

add_library(CURL::libcurl STATIC IMPORTED GLOBAL)
set_property(TARGET CURL::libcurl APPEND PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET CURL::libcurl APPEND PROPERTY IMPORTED_CONFIGURATIONS Debug)

target_include_directories(CURL::libcurl INTERFACE
        ${CURL_INCLUDE_DIR}
)

target_compile_definitions(CURL::libcurl INTERFACE
        "CURL_STATICLIB"
)

if (UNIX)
    target_link_libraries(CURL::libcurl INTERFACE
            ZLIB::ZLIB
            CARES::libcares
            OpenSSL::Crypto
            OpenSSL::SSL
    )
elseif (WIN32)
    target_link_libraries(CURL::libcurl INTERFACE
            ZLIB::ZLIB
            Ws2_32
            Wldap32
            Crypt32
            Normaliz
    )
endif ()

set_target_properties(CURL::libcurl PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;RC"
    IMPORTED_LOCATION_RELEASE "${CURL_LIBRARY}"
    IMPORTED_LOCATION_DEBUG "${CURL_LIBRARY_DEBUG}"
)
