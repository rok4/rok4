
# CMake module to search for Kakadu library
#
# If it's found it sets KAKADU_FOUND to TRUE
# and following variables are set:
#    KAKADU_INCLUDE_DIR
#    KAKADU_LIBRARY

FIND_PATH(KAKADU_INCLUDE_DIR jp2.h
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libopenjpeg/src
    /home/theo/TEST/v6_4_1-00841C/managed/all_includes
    )
FIND_LIBRARY(KAKADU_LIBRARY NAMES libkdu_v64R.so PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libkakadu/src
    /home/theo/TEST/v6_4_1-00841C/lib/Linux-x86-64-gcc
    )
#FIND_LIBRARY(KAKADU_LIBRARY kakadu)

MESSAGE("KAKADU_INCLUDE_DIR : ${KAKADU_INCLUDE_DIR}")
MESSAGE("KAKADU_LIBRARY : ${KAKADU_LIBRARY}")
  
IF(CPPUNIT_LIBRARY)
    SET(CPPUNIT_LIBRARY ${KAKADU_LIBRARY} ${CMAKE_DL_LIBS})
ENDIF(CPPUNIT_LIBRARY)


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Kakadu" DEFAULT_MSG KAKADU_INCLUDE_DIR KAKADU_LIBRARY )