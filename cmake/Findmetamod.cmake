if (TARGET metamod::metamod)
    return()
endif ()

include(FindPackageHandleStandardArgs)

###
### Find includes and libraries
###

find_path(METAMOD_INCLUDE_DIR
        NAMES metamod.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/metamod
        NO_DEFAULT_PATH
        NO_CACHE
)

find_package_handle_standard_args(metamod REQUIRED_VARS METAMOD_INCLUDE_DIR)

###
### Setup library
###

add_library(metamod::metamod INTERFACE IMPORTED GLOBAL)

target_include_directories(metamod::metamod INTERFACE
        ${METAMOD_INCLUDE_DIR}
)
