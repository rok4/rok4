
# CMake module to search for TinyXML library
#
# If it's found it sets TINYXML_FOUND to TRUE
# and following variables are set:
#    TINYXML_INCLUDE_DIR
#    TINYXML_LIBRARY

FIND_PATH(TINYXML_INCLUDE_DIR tinyxml.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/libtinyxml/src
    )
FIND_LIBRARY(TINYXML_LIBRARY NAMES libtinyxml.so PATHS 
    /usr/local/lib 
    /usr/lib 
    /usr/lib/x86_64-linux-gnu 
    c:/msys/local/lib
    C:/dev/cpp/libtinyxml/src
    )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "TinyXML" DEFAULT_MSG TINYXML_INCLUDE_DIR TINYXML_LIBRARY )
