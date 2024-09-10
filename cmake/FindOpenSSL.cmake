find_path(OPENSSL_INCLUDE_DIR
        NAMES openssl/opensslconf.h
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/openssl/include
        NO_DEFAULT_PATH)

find_library(SSL_LIBRARY
        NAMES libssl.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/openssl/lib
        NO_DEFAULT_PATH)

find_library(CRYPTO_LIBRARY
        NAMES libcrypto.a
        PATHS ${AMXX_EASY_HTTP_ROOT}/dep/openssl/lib
        NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenSSL REQUIRED_VARS OPENSSL_INCLUDE_DIR SSL_LIBRARY CRYPTO_LIBRARY)

if (NOT TARGET OpenSSL::Crypto)
    add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
    set_target_properties(OpenSSL::Crypto PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${CRYPTO_LIBRARY}"
    )
endif()

if (NOT TARGET OpenSSL::SSL)
    add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
    set_target_properties(OpenSSL::SSL PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${SSL_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "OpenSSL::Crypto"
    )
endif()

unset(SSL_LIBRARY CACHE)
unset(CRYPTO_LIBRARY CACHE)
