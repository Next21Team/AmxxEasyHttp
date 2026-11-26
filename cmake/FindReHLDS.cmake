if (TARGET ReHLDS::headers)
    return()
endif ()

find_path(ReHLDS_INCLUDE_DIR NAMES public/rehlds/rehlds_api.h
        PATHS "${AMXX_EASY_HTTP_ROOT}/dep/rehlds"
        NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ReHLDS REQUIRED_VARS ReHLDS_INCLUDE_DIR)

add_library(ReHLDS::headers INTERFACE IMPORTED)
target_include_directories(ReHLDS::headers INTERFACE
        "${ReHLDS_INCLUDE_DIR}/public/rehlds"
        "${ReHLDS_INCLUDE_DIR}/public"
        "${ReHLDS_INCLUDE_DIR}/common")
