
# CMake module to search for Png library
#
# If it's found it sets PNG_FOUND to TRUE
# and following variables are set:
#    OPENJPEG_INCLUDE_DIR
#    OPENJPEG_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(OPENJPEG_INCLUDE_DIR openjpeg.h
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libopenjpeg/src
    )
  FIND_LIBRARY(OPENJPEG_LIBRARY NAMES libopenjp2.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libopenjpeg/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(OPENJPEG_INCLUDE_DIR NAMES openjpeg.h PATHS
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(OPENJPEG_LIBRARY NAMES libopenjp2.a PATHS
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )  
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Openjpeg" DEFAULT_MSG OPENJPEG_INCLUDE_DIR OPENJPEG_LIBRARY )