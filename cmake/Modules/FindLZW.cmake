
# CMake module to search for LZW library
#
# If it's found it sets LZW_FOUND to TRUE
# and following variables are set:
#    LZW_INCLUDE_DIR
#    LZW_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(LZW_INCLUDE_DIR lzwEncoder.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/liblzw/src
    )
  FIND_LIBRARY(LZW_LIBRARY NAMES liblzw.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/liblzw/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(LZW_INCLUDE_DIR NAMES lzwEncoder.h PATHS  
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(LZW_LIBRARY NAMES liblzw.a PATHS  
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ) 
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "LZW" DEFAULT_MSG LZW_INCLUDE_DIR LZW_LIBRARY )
