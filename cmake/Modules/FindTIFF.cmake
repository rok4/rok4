
# CMake module to search for TIFF library
#
# If it's found it sets TIFF_FOUND to TRUE
# and following variables are set:
#    TIFF_INCLUDE_DIR
#    TIFF_LIBRARY

FIND_PATH(TIFF_INCLUDE_DIR tiff.h 
  /usr/local/include 
  /usr/include 
  c:/msys/local/include
  C:/dev/cpp/libtiff/src
  /usr/include/x86_64-linux-gnu/
  )


FIND_LIBRARY(TIFF_LIBRARY NAMES libtiff.so PATHS 
  /usr/local/lib 
  /usr/lib 
  c:/msys/local/lib
  C:/dev/cpp/libtiff/src
  /usr/lib/x86_64-linux-gnu/
  )


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Tiff" DEFAULT_MSG TIFF_INCLUDE_DIR TIFF_LIBRARY )
