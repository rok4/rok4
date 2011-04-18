
# CMake module to search for TIFF library
#
# If it's found it sets TIFF_FOUND to TRUE
# and following variables are set:
#    TIFF_INCLUDE_DIR
#    TIFF_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(TIFF_INCLUDE_DIR tiff.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libtiff/src
    )
  FIND_LIBRARY(TIFF_LIBRARY NAMES libtiff.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libtiff/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(TIFF_INCLUDE_DIR NAMES tiff.h PATHS  
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(TIFF_LIBRARY NAMES libtiff.a PATHS  
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ) 
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Tiff" DEFAULT_MSG TIFF_INCLUDE_DIR TIFF_LIBRARY )
