# - Find CppUnit
# This module finds an installed CppUnit package.
#
# It sets the following variables:
#  CPPUNIT_FOUND       - Set to false, or undefined, if CppUnit isn't found.
#  CPPUNIT_INCLUDE_DIR - The CppUnit include directory.
#  CPPUNIT_LIBRARY     - The CppUnit library to link against.

IF (USE_SYSTEM_LIB)
  FIND_PATH(CPPUNIT_INCLUDE_DIR cppunit/Test.h)
  FIND_LIBRARY(CPPUNIT_LIBRARY NAMES libcppunit.a)
ELSE (USE_SYSTEM_LIB)
  FIND_PATH(CPPUNIT_INCLUDE_DIR cppunit/Test.h PATHS 
    ${DEP_PATH}/include
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
  )
  FIND_LIBRARY(CPPUNIT_LIBRARY NAMES libcppunit.a PATHS
    ${DEP_PATH}/lib
    NO_DEFAULT_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_CMAKE_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
  )
ENDIF (USE_SYSTEM_LIB)



# handle the QUIETLY and REQUIRED arguments and set CPPUNIT_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "CppUnit" DEFAULT_MSG CPPUNIT_INCLUDE_DIR CPPUNIT_LIBRARY )
