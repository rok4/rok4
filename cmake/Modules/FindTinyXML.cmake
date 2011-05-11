
# CMake module to search for TinyXML library
#
# If it's found it sets TINYXML_FOUND to TRUE
# and following variables are set:
#    TINYXML_INCLUDE_DIR
#    TINYXML_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(TINYXML_INCLUDE_DIR tinystr.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libtinyxml/src
    )
  FIND_LIBRARY(TINYXML_LIBRARY NAMES libtinyxml.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/libtinyxml/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(TINYXML_INCLUDE_DIR NAMES tinystr.h PATHS  
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(TINYXML_LIBRARY NAMES libtinyxml.a PATHS  
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ) 
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "TinyXML" DEFAULT_MSG TINYXML_INCLUDE_DIR TINYXML_LIBRARY )
