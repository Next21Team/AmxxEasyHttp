if (TARGET ReHLDS::ReHLDS_includes)
    return()
endif ()

if (NOT ReHLDS_INCLUDE_DIR)
    find_path(ReHLDS_INCLUDE_DIR NAMES public/rehlds/rehlds_api.h
            PATHS "${AMXX_EASY_HTTP_ROOT}/dep/rehlds"
            NO_DEFAULT_PATH
            NO_CACHE)
    # CMake versions before 3.21 ignore NO_CACHE.
    if (CMAKE_VERSION VERSION_LESS 3.21)
        set(ReHLDS_INCLUDE_DIR "${ReHLDS_INCLUDE_DIR}")
        unset(ReHLDS_INCLUDE_DIR CACHE)
    endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ReHLDS REQUIRED_VARS ReHLDS_INCLUDE_DIR)

add_library(ReHLDS::ReHLDS_includes INTERFACE IMPORTED GLOBAL)
target_include_directories(ReHLDS::ReHLDS_includes INTERFACE
        "${ReHLDS_INCLUDE_DIR}/public/rehlds"
        "${ReHLDS_INCLUDE_DIR}/public"
        "${ReHLDS_INCLUDE_DIR}/common")
