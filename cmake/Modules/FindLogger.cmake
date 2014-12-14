
# CMake module to search for Logger library
#
# If it's found it sets LOGGER_FOUND to TRUE
# and following variables are set:
#    LOGGER_INCLUDE_DIR
#    LOGGER_LIBRARY

IF (USE_SYSTEM_LIB)
  FIND_PATH(LOGGER_INCLUDE_DIR Logger.h 
    /usr/local/include 
    /usr/include 
    c:/msys/local/include
    C:/dev/cpp/liblogger/src
    )
  FIND_LIBRARY(LOGGER_LIBRARY NAMES liblogger.a PATHS 
    /usr/local/lib 
    /usr/lib 
    c:/msys/local/lib
    C:/dev/cpp/liblogger/src
    )
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(LOGGER_INCLUDE_DIR NAMES Logger.h PATHS  
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    )
  FIND_LIBRARY(LOGGER_LIBRARY NAMES liblogger.a PATHS
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ) 
ENDIF (USE_SYSTEM_LIB)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Logger" DEFAULT_MSG LOGGER_INCLUDE_DIR LOGGER_LIBRARY )
