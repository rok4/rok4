
# CMake module to search for Image library
#
# If it's found it sets IMAGE_FOUND to TRUE
# and following variables are set:
#    IMAGE_INCLUDE_DIR
#    IMAGE_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(IMAGE_INCLUDE_DIR ExtendedCompoundImage.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libimage/src
    )
  FIND_LIBRARY(IMAGE_LIBRARY NAMES libimage.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libimage/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(IMAGE_INCLUDE_DIR NAMES ExtendedCompoundImage.h PATHS  
    ${ROK4_DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(IMAGE_LIBRARY NAMES libimage.a PATHS  
    ${ROK4_DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ) 
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Image" DEFAULT_MSG IMAGE_INCLUDE_DIR IMAGE_LIBRARY )
