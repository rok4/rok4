# CMake module to search for FastCGI headers
#
# If it's found it sets FCGI_FOUND to TRUE
# and following variables are set:
#    FCGI_INCLUDE_DIR
#    FCGI_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(FCGI_INCLUDE_DIR fcgios.h PATHS
    /usr/include
    /usr/local/include
    /usr/include/fastcgi
    #MSVC
    "$ENV{LIB_DIR}/include"
    $ENV{INCLUDE}
    #mingw
    c:/msys/local/include
    )
  FIND_LIBRARY(FCGI_LIBRARY NAMES libfcgi.a PATHS  
    /usr/lib
    /usr/local/lib
    /usr/lib/fastcgi
    #MSVC
    "$ENV{LIB_DIR}/lib"
    $ENV{LIB}
    #mingw
    c:/msys/local/lib
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(FCGI_INCLUDE_DIR NAMES fcgios.h PATHS  
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(FCGI_LIBRARY NAMES libfcgi.a PATHS  
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Fcgi" DEFAULT_MSG FCGI_INCLUDE_DIR FCGI_LIBRARY )
