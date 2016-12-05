
# CMake module to search for Jpeg library
#
# If it's found it sets JPEG_FOUND to TRUE
# and following variables are set:
#    JPEG_INCLUDE_DIR
#    JPEG_LIBRARY

FIND_PATH(JPEG_INCLUDE_DIR turbojpeg.h
  /usr/local/include 
  /usr/include 
  c:/msys/local/include
  )
FIND_LIBRARY(JPEG_LIBRARY NAMES libturbojpeg.a PATHS 
  /usr/local/lib 
  /usr/lib 
  c:/msys/local/lib
  C:/dev/cpp/libjpeg/src
  /usr/lib/x86_64-linux-gnu/
  )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Jpeg" DEFAULT_MSG JPEG_INCLUDE_DIR JPEG_LIBRARY )
