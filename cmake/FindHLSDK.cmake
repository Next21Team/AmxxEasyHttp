if (TARGET HLSDK::HLSDK_includes)
    return()
endif()

include(FindPackageHandleStandardArgs)

###
### Find includes and libraries
###

find_path(HLSDK_COMMON_INCLUDE_DIR
        NAMES cl_entity.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/halflife/common
        NO_DEFAULT_PATH
        NO_CACHE
)

find_path(HLSDK_DLLS_INCLUDE_DIR
        NAMES client.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/halflife/dlls
        NO_DEFAULT_PATH
        NO_CACHE
)

find_path(HLSDK_ENGINE_INCLUDE_DIR
        NAMES APIProxy.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/halflife/engine
        NO_DEFAULT_PATH
        NO_CACHE
)

find_path(HLSDK_PUBLIC_INCLUDE_DIR
        NAMES pman_particlemem.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/halflife/public
        NO_DEFAULT_PATH
        NO_CACHE
)

find_package_handle_standard_args(HLSDK
        REQUIRED_VARS HLSDK_COMMON_INCLUDE_DIR
                      HLSDK_DLLS_INCLUDE_DIR
                      HLSDK_ENGINE_INCLUDE_DIR
                      HLSDK_PUBLIC_INCLUDE_DIR
)

###
### Setup library
###

add_library(HLSDK::HLSDK_includes INTERFACE IMPORTED GLOBAL)

target_include_directories(HLSDK::HLSDK_includes INTERFACE
        ${HLSDK_COMMON_INCLUDE_DIR}
        ${HLSDK_DLLS_INCLUDE_DIR}
        ${HLSDK_ENGINE_INCLUDE_DIR}
        ${HLSDK_PUBLIC_INCLUDE_DIR}
)
