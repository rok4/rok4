
# CMake module to search for Proj library
#
# If it's found it sets PROJ_FOUND to TRUE
# and following variables are set:
#    PROJ_INCLUDE_DIR
#    PROJ_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(PROJ_INCLUDE_DIR proj_api.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libproj/src
    )
  FIND_LIBRARY(PROJ_LIBRARY NAMES libproj.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libproj/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(PROJ_INCLUDE_DIR NAMES proj_api.h PATHS  
    ${ROK4_DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(PROJ_LIBRARY NAMES libproj.a PATHS  
    ${ROK4_DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ) 
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Proj" DEFAULT_MSG PROJ_INCLUDE_DIR PROJ_LIBRARY )
