
# CMake module to search for Jpeg library
#
# If it's found it sets HIREDIS_FOUND to TRUE
# and following variables are set:
#    HIREDIS_INCLUDE_DIR
#    HIREDIS_LIBRARY

FIND_PATH(HIREDIS_INCLUDE_DIR hiredis.h
  /usr/local/include 
  /usr/include/hiredis
  c:/msys/local/include
  )
FIND_LIBRARY(HIREDIS_LIBRARY NAMES libhiredis.so PATHS 
  /usr/local/lib 
  /usr/lib 
  /usr/lib64
  c:/msys/local/lib
  C:/dev/cpp/libjpeg/src
  /usr/lib/x86_64-linux-gnu/
  )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Hiredis" DEFAULT_MSG HIREDIS_INCLUDE_DIR HIREDIS_LIBRARY )
