
# CMake module to search for Zlib library
#
# If it's found it sets ZLIB_FOUND to TRUE
# and following variables are set:
#    ZLIB_INCLUDE_DIR
#    ZLIB_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(ZLIB_INCLUDE_DIR zlib.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libzlib/src
    )
  FIND_LIBRARY(ZLIB_LIBRARY NAMES libz.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libzlib/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(ZLIB_INCLUDE_DIR NAMES zlib.h PATHS  
    ${ROK4_DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(ZLIB_LIBRARY NAMES libz.a PATHS  
    ${ROK4_DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ) 
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Zlib" DEFAULT_MSG ZLIB_INCLUDE_DIR ZLIB_LIBRARY )
