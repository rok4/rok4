
# CMake module to search for Kakadu library
#
# If it's found it sets KAKADU_FOUND to TRUE
# and following variables are set:
#    JPEG2000_INCLUDE_DIR
#    KAKADU_LIBRARY_1
#    KAKADU_LIBRARY_2

FIND_PATH(JPEG2000_INCLUDE_DIR jpx.h
#     /usr/local/include 
#     /usr/include 
#     c:/msys/local/include
#     C:/dev/cpp/libkakadu/src
#     /usr/local/kakadu6.4/managed/all_includes
    /usr/local/kakadu6.4/
    )

FIND_LIBRARY(KAKADU_LIBRARY_1 NAME libkdu_aux.a PATHS 
#     /usr/local/lib 
#     /usr/lib 
#     c:/msys/local/lib
#     C:/dev/cpp/libkakadu/src
#     /usr/local/kakadu6.4/lib/Linux-x86-64-gcc
#/usr/local/kakadu6.4/
    )

FIND_LIBRARY(KAKADU_LIBRARY_2 NAME libkdu.a PATHS 
#     /usr/local/lib 
#     /usr/lib 
#     c:/msys/local/lib
#     C:/dev/cpp/libkakadu/src
#     /usr/local/kakadu6.4/lib/Linux-x86-64-gcc
#/usr/local/kakadu6.4/lib/Linux-x86-64-gcc
    )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Kakadu" DEFAULT_MSG JPEG2000_INCLUDE_DIR KAKADU_LIBRARY_1 KAKADU_LIBRARY_2 )
