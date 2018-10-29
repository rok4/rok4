# CMake module to search for FastCGI headers
#
# If it's found it sets FCGI_FOUND to TRUE
# and following variables are set:
#    FCGI_INCLUDE_DIR
#    FCGI_LIBRARY

FIND_PATH(FCGI_INCLUDE_DIR fcgios.h
  /usr/local/include 
  /usr/include 
  c:/msys/local/include
  )
FIND_LIBRARY(FCGI_LIBRARY NAMES libfcgi.so PATHS 
  /usr/local/lib 
  /usr/lib 
  /usr/lib64
  c:/msys/local/lib
  /usr/lib/x86_64-linux-gnu/
  )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Fcgi" DEFAULT_MSG FCGI_INCLUDE_DIR FCGI_LIBRARY )