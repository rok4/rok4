# CMake module to search for KAKADU library
#
# headers and librairies are searched for in KAKADU_HOME dir,
#
# If it's found it sets KAKADU_FOUND to TRUE
# and following variables are set:
#    KAKADU_INCLUDE_DIR
#    KAKADU_LIBRARY

   # Path overwritten by user with option : cmake -D KAKADU_PREFIX=...
   # Path search by default

   IF(KAKADU_PREFIX)
      MESSAGE(STATUS "[KAKADU] External CUSTOM Search LIBRARY in : ${KAKADU_PREFIX}")

      # set INCLUDE_DIR
      FIND_PATH(INCLUDE_TMP NAMES kdu.h PATHS
         ${KAKADU_PREFIX}/include/kdu/
         NO_DEFAULT_PATH
         NO_CMAKE_ENVIRONMENT_PATH
         NO_CMAKE_PATH
         NO_SYSTEM_ENVIRONMENT_PATH
         NO_CMAKE_SYSTEM_PATH
      )

      SET(KAKADU_INCLUDE_DIR ${INCLUDE_TMP})

      # set LIBRARY_DIR
      FIND_PATH(LIBRARY_TMP NAMES libkdu.a PATHS
         ${KAKADU_PREFIX}/lib/
         NO_DEFAULT_PATH
         NO_CMAKE_ENVIRONMENT_PATH
         NO_CMAKE_PATH
         NO_SYSTEM_ENVIRONMENT_PATH
         NO_CMAKE_SYSTEM_PATH
      )

      SET(KAKADU_LIBRARY ${LIBRARY_TMP}/libkdu.a)

   ELSE(KAKADU_PREFIX)
      MESSAGE(STATUS "[KAKADU] External SYSTEM Search LIBRARY...")

      # only for unix
      IF(UNIX) 
        # set INCLUDE_DIR
        FIND_PATH(KAKADU_INCLUDE_DIR NAMES KAKADU.h PATHS
          /usr/local/include/kdu
          /usr/include/kdu
        )
        
        # set LIBRARY_DIR
        FIND_LIBRARY(KAKADU_LIBRARY NAMES libkdu.a PATHS
          /usr/local/lib 
          /usr/lib 
        )
      ENDIF(UNIX)
   ENDIF(KAKADU_PREFIX)

   IF (KAKADU_INCLUDE_DIR AND KAKADU_LIBRARY)
     SET(KAKADU_FOUND TRUE)
   ENDIF(KAKADU_INCLUDE_DIR AND KAKADU_LIBRARY)

   IF (KAKADU_FOUND)

        MESSAGE(STATUS "[KAKADU] Found library : ${KAKADU_LIBRARY}")
        MESSAGE(STATUS "[KAKADU] Found include : ${KAKADU_INCLUDE_DIR}")

   ELSE(KAKADU_FOUND)

        MESSAGE(STATUS "[KAKADU] Could not find KAKADU project, so you could try an other custom path to KAKADU !")

   ENDIF(KAKADU_FOUND)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Kakadu" DEFAULT_MSG KAKADU_INCLUDE_DIR KAKADU_LIBRARY)

