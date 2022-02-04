find_library(SSL_LIBRARY
        NAMES libssl.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/openssl/lib NO_DEFAULT_PATH)

find_library(CRYPTO_LIBRARY
        NAMES libcrypto.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/openssl/lib NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenSSL REQUIRED_VARS SSL_LIBRARY CRYPTO_LIBRARY)

if (NOT TARGET OPENSSL::libcrypto)
    add_library(OPENSSL::libcrypto STATIC IMPORTED GLOBAL)
    set_target_properties(OPENSSL::libcrypto PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${CRYPTO_LIBRARY}"
    )
endif()

if (NOT TARGET OPENSSL::libssl)
    add_library(OPENSSL::libssl STATIC IMPORTED GLOBAL)
    set_target_properties(OPENSSL::libssl PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${SSL_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "OPENSSL::libcrypto"
    )
endif()

unset(SSL_LIBRARY CACHE)
unset(CRYPTO_LIBRARY CACHE)