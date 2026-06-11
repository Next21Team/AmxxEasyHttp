set(AMXX_EASY_HTTP_DEPS_DIR "${AMXX_EASY_HTTP_ROOT}/dep" CACHE PATH "Root directory of the prebuilt dependencies")

if (NOT UNIX)
    return()
endif ()

file(STRINGS "${AMXX_EASY_HTTP_ROOT}/.github/deps.lock" _lock)
foreach (_l ${_lock})
    if (_l MATCHES "^([A-Za-z0-9_]+)=(.+)$")
        set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endif ()
endforeach ()

include(FetchContent)

set(_args URL "https://github.com/${DEPS_REPO}/releases/download/${DEPS_TAG}/${DEPS_ASSET}")
if (DEPS_SHA256)
    list(APPEND _args URL_HASH SHA256=${DEPS_SHA256})
endif ()

FetchContent_Declare(amxxdeps ${_args})
FetchContent_GetProperties(amxxdeps)
if (NOT amxxdeps_POPULATED)
    FetchContent_Populate(amxxdeps)
endif ()

set(AMXX_EASY_HTTP_DEPS_DIR "${amxxdeps_SOURCE_DIR}" CACHE PATH "Root directory of the prebuilt dependencies" FORCE)
message(STATUS "Using dependencies from ${AMXX_EASY_HTTP_DEPS_DIR}")
