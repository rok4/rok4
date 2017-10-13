
# CMake module to search for Png library
#
# If it's found it sets PNG_FOUND to TRUE
# and following variables are set:
#    PNG_INCLUDE_DIR
#    PNG_LIBRARY

FIND_PATH(PNG_INCLUDE_DIR png.h
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libpng/src
    )

FIND_LIBRARY(PNG_LIBRARY NAMES libpng.so PATHS 
    /usr/local/lib 
    /usr/lib
    /usr/lib64 
    c:/msys/local/lib
    C:/dev/cpp/libpng/src
    )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Png" DEFAULT_MSG PNG_INCLUDE_DIR PNG_LIBRARY )
