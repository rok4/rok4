
# CMake module to search for the Packbits library
#
# If it's found it sets PKB_FOUND to TRUE
# and following variables are set:
#    PKB_INCLUDE_DIR
#    PKB_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(PKB_INCLUDE_DIR pkbEncoder.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libpkb/src
    )
  FIND_LIBRARY(PKB_LIBRARY NAMES libpkb.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libpkb/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(PKB_INCLUDE_DIR NAMES pkbEncoder.h PATHS  
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(PKB_LIBRARY NAMES libpkb.a PATHS  
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ) 
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "PKB" DEFAULT_MSG PKB_INCLUDE_DIR PKB_LIBRARY )
