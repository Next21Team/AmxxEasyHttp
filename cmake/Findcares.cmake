if (TARGET CARES::libcares)
    return()
endif()

find_path(CARES_INCLUDE_DIR
        NAMES ares.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/cares/include NO_DEFAULT_PATH)

find_library(CARES_LIBRARY
        NAMES libcares.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/cares/lib
        NO_DEFAULT_PATH
        NO_CACHE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cares REQUIRED_VARS CARES_LIBRARY CARES_INCLUDE_DIR)

add_library(CARES::libcares STATIC IMPORTED GLOBAL)
set_target_properties(CARES::libcares PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${CARES_LIBRARY}"
)
