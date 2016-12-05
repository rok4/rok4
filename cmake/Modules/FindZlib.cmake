
# CMake module to search for Zlib library
#
# If it's found it sets ZLIB_FOUND to TRUE
# and following variables are set:
#    ZLIB_INCLUDE_DIR
#    ZLIB_LIBRARY

FIND_PATH(ZLIB_INCLUDE_DIR zlib.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libzlib/src
    )
FIND_LIBRARY(ZLIB_LIBRARY NAMES libz.so PATHS 
    /usr/local/lib 
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    c:/msys/local/lib
    C:/dev/cpp/libzlib/src
    )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Zlib" DEFAULT_MSG ZLIB_INCLUDE_DIR ZLIB_LIBRARY )
