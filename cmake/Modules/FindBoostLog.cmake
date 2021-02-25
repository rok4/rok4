
# CMake module to search for BoostLog library
#
# If it's found it sets BOOSTLOG_FOUND to TRUE
# and following variables are set:
#    BOOSTLOG_INCLUDE_DIR
#    BOOSTLOG_LIBRARY

FIND_PATH(BOOSTLOG_INCLUDE_DIR boost/log/core.hpp
    /usr/local/include 
    /usr/include 
    )

FIND_LIBRARY(BOOSTLOG_LIBRARY NAMES libboost_log.so PATHS 
    /usr/local/lib 
    /usr/lib
    /usr/lib64 
    /usr/lib/x86_64-linux-gnu
    )

FIND_LIBRARY(BOOSTLOGSETUP_LIBRARY NAMES libboost_log_setup.so PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    /usr/lib64
    )   

FIND_LIBRARY(BOOSTTHREAD_LIBRARY NAMES libboost_thread.so PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    /usr/lib64
    )    

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "BoostLog" DEFAULT_MSG BOOSTLOG_INCLUDE_DIR BOOSTLOG_LIBRARY BOOSTLOGSETUP_LIBRARY BOOSTTHREAD_LIBRARY)
