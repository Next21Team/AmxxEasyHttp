if (TARGET ZLIB::ZLIB)
    return()
endif()

include(FindPackageHandleStandardArgs)

###
### Find includes and libraries
###

find_path(ZLIB_INCLUDE_DIR
        NAMES zlib.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/zlib/include
        NO_DEFAULT_PATH
        NO_CACHE
)

find_library(ZLIB_LIBRARY
        NAMES zlib_a.lib libz.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/zlib/lib
        NO_DEFAULT_PATH
        NO_CACHE
)

find_package_handle_standard_args(ZLIB REQUIRED_VARS ZLIB_LIBRARY ZLIB_INCLUDE_DIR)

###
### Setup library
###

add_library(ZLIB::ZLIB STATIC IMPORTED GLOBAL)

set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${ZLIB_LIBRARY}"
)
