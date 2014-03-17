
# CMake module to search for Kakadu library
#
# If it's found it sets KAKADU_FOUND to TRUE
# and following variables are set:
#    JPEG2000_INCLUDE_DIR
#    KAKADU_LIBRARY_1
#    KAKADU_LIBRARY_2

FIND_PATH(JPEG2000_INCLUDE_DIR kdu_utils.h
# On cherche kdu_utils.h et pas jp2.h pour ne pas tomber sur le jp2.h de la lib openjpeg
# Tous les headers (en open source) sont dans le projet
    ${ROK4LIBSDIR}/libkakadu/kakadu-6.4
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
)

FIND_LIBRARY(KAKADU_LIBRARY_1 NAME libkdu_aux.a PATHS 
    ${KAKADU_LIBRARY_PATH}
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libkakadu/src
)

FIND_LIBRARY(KAKADU_LIBRARY_2 NAME libkdu.a PATHS 
    ${KAKADU_LIBRARY_PATH}
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libkakadu/src
)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Kakadu" DEFAULT_MSG JPEG2000_INCLUDE_DIR KAKADU_LIBRARY_1 KAKADU_LIBRARY_2 )
