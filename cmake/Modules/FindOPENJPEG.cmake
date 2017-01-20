
# CMake module to search for OpenJpeg library
#
# If it's found it sets OPENJPEG_FOUND to TRUE
# and following variables are set:
#    JPEG2000_INCLUDE_DIR
#    OPENJPEG_LIBRARY

FIND_PATH(JPEG2000_INCLUDE_DIR openjpeg.h
    /usr/local/include 
    /usr/include 
    /usr/include/openjpeg-2.1/      
    c:/msys/local/include
    C:/dev/cpp/libopenjpeg/src
    )

FIND_LIBRARY(OPENJPEG_LIBRARY NAMES libopenjp2.so PATHS 
    /usr/local/lib 
    /usr/lib 
    /usr/lib64
    /usr/lib/x86_64-linux-gnu/
    c:/msys/local/lib
    C:/dev/cpp/libopenjpeg/src
    )


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Openjpeg" DEFAULT_MSG JPEG2000_INCLUDE_DIR OPENJPEG_LIBRARY )