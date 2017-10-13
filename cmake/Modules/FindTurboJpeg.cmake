
# CMake module to search for Jpeg library
#
# If it's found it sets TURBOJPEG_FOUND to TRUE
# and following variables are set:
#    TURBOJPEG_INCLUDE_DIR
#    TURBOJPEG_LIBRARY

FIND_PATH(TURBOJPEG_INCLUDE_DIR turbojpeg.h
  /usr/local/include 
  /usr/include 
  c:/msys/local/include
  )
FIND_LIBRARY(TURBOJPEG_LIBRARY NAMES libturbojpeg.so PATHS 
  /usr/local/lib 
  /usr/lib 
  /usr/lib64
  c:/msys/local/lib
  C:/dev/cpp/libjpeg/src
  /usr/lib/x86_64-linux-gnu/
  )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "TurboJpeg" DEFAULT_MSG TURBOJPEG_INCLUDE_DIR TURBOJPEG_LIBRARY )
