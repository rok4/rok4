
# CMake module to search for Curl library
#
# If it's found it sets CURL_FOUND to TRUE
# and following variables are set:
#    CURL_INCLUDE_DIR
#    CURL_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(CURL_INCLUDE_DIR curl/curl.h 
    /usr/local/include 
    /usr/include 
    /usr/local/include/curl
    /usr/include/curl
    c:/msys/local/include
    C:/dev/cpp/libcurl/src
    )
  FIND_LIBRARY(CURL_LIBRARY NAMES libcurl.a PATHS 
    /usr/local/lib 
    /usr/lib 
    /usr/local/lib/curl 
    /usr/lib/curl 
    c:/msys/local/lib
    C:/dev/cpp/libcurl/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(CURL_INCLUDE_DIR NAMES curl.h
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(CURL_LIBRARY NAMES libcurl.a
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )  
ENDIF (USE_SYSTEM_LIB)

#IF(CURL_LIBRARY)
# SET(CURL_LIBRARY ${CURL_LIBRARY} ${CMAKE_DL_LIBS})
#ENDIF(CURL_LIBRARY)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Curl" DEFAULT_MSG CURL_INCLUDE_DIR CURL_LIBRARY )
