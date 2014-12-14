
# CMake module to search for Jpeg library
#
# If it's found it sets JPEG_FOUND to TRUE
# and following variables are set:
#    JPEG_INCLUDE_DIR
#    JPEG_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(JPEG_INCLUDE_DIR jconfig.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libjpeg/src
    )
  FIND_LIBRARY(JPEG_LIBRARY NAMES libjpeg.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libjpeg/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(JPEG_INCLUDE_DIR NAMES jconfig.h PATHS  
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(JPEG_LIBRARY NAMES libjpeg.a PATHS  
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ) 
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Jpeg" DEFAULT_MSG JPEG_INCLUDE_DIR JPEG_LIBRARY )
