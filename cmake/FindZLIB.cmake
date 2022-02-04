if (TARGET ZLIB::zlib)
    return()
endif()

find_library(ZLIB_LIBRARY
        NAMES libz.a zlib_a.lib
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/zlib/lib NO_DEFAULT_PATH)

message(${ZLIB_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZLIB REQUIRED_VARS ZLIB_LIBRARY)

add_library(ZLIB::ZLIB STATIC IMPORTED GLOBAL)
set_target_properties(ZLIB::ZLIB PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${ZLIB_LIBRARY}"
)

unset(ZLIB_LIBRARY CACHE)