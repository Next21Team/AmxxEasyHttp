if (TARGET CARES::libcares)
    return()
endif()

find_library(CARES_LIBRARY
        NAMES libcares.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/cares/lib NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cares REQUIRED_VARS CARES_LIBRARY)

add_library(CARES::libcares STATIC IMPORTED GLOBAL)
set_target_properties(CARES::libcares PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${CARES_LIBRARY}"
)

unset(CARES_LIBRARY CACHE)