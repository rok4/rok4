
# CMake module to search for Png library
#
# If it's found it sets PNG_FOUND to TRUE
# and following variables are set:
#    PNG_INCLUDE_DIR
#    PNG_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(PNG_INCLUDE_DIR png.h
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libpng/src
    )
  FIND_LIBRARY(PNG_LIBRARY NAMES libpng.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libpng/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(PNG_INCLUDE_DIR NAMES png.h PATHS
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(PNG_LIBRARY NAMES libpng.a PATHS
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )  
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Png" DEFAULT_MSG PNG_INCLUDE_DIR PNG_LIBRARY )
