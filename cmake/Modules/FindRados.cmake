
# CMake module to search for Curl library
#
# If it's found it sets RADOS_FOUND to TRUE
# and following variables are set:
#    RADOS_INCLUDE_DIR
#    RADOS_LIBRARY

FIND_PATH(RADOS_INCLUDE_DIR librados.h 
    /usr/local/include 
    /usr/include/rados
    /usr/local/include/rados
    c:/msys/local/include
    C:/dev/cpp/librados/src
    )

FIND_LIBRARY(RADOS_LIBRARY NAMES librados.so PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    c:/msys/local/lib
    C:/dev/cpp/librados/src
    )


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Rados" DEFAULT_MSG RADOS_INCLUDE_DIR RADOS_LIBRARY )