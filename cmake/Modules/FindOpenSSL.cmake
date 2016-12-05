
# CMake module to search for Curl library
#
# If it's found it sets OPENSSL_FOUND to TRUE
# and following variables are set:
#    OPENSSL_INCLUDE_DIR
#    OPENSSL_LIBRARY

FIND_PATH(OPENSSL_INCLUDE_DIR hmac.h 
    /usr/local/include 
    /usr/include/openssl
    /usr/local/include/openssl
    c:/msys/local/include
    C:/dev/cpp/libopenssl/src
    )

FIND_LIBRARY(OPENSSL_LIBRARY NAMES libssl.so PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    c:/msys/local/lib
    C:/dev/cpp/libopenssl/src
    )

FIND_LIBRARY(CRYPTO_LIBRARY NAMES libcrypto.so PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    c:/msys/local/lib
    C:/dev/cpp/libcrypto/src
    )    


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "OpenSSL" DEFAULT_MSG OPENSSL_INCLUDE_DIR OPENSSL_LIBRARY CRYPTO_LIBRARY)