
# CMake module to search for Jpeg library
#
# If it's found it sets JPEG_FOUND to TRUE
# and following variables are set:
#    JPEG_LIBRARY

FIND_LIBRARY(JPEG_LIBRARY NAMES libjpeg.so PATHS 
  /usr/local/lib 
  /usr/lib 
  /usr/lib64
  c:/msys/local/lib
  C:/dev/cpp/libjpeg/src
  /usr/lib/x86_64-linux-gnu/
  )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Jpeg" DEFAULT_MSG JPEG_LIBRARY )
