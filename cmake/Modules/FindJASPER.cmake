# CMake module to search for JASPER library
#
# headers and librairies are searched for in JASPER_HOME dir,
#
# If it's found it sets JASPER_FOUND to TRUE
# and following variables are set:
#    JASPER_INCLUDE_DIR
#    JASPER_LIBRARY

   # Path overwritten by user with option : cmake -D JASPER_PREFIX=...
   # Path search by default

   IF(JASPER_PREFIX)
      MESSAGE(STATUS "[JASPER] External CUSTOM Search LIBRARY in : ${JASPER_PREFIX}")

      # set INCLUDE_DIR
      FIND_PATH(INCLUDE_TMP NAMES jasper.h PATHS
         ${JASPER_PREFIX}/include/jasper/
         NO_DEFAULT_PATH
         NO_CMAKE_ENVIRONMENT_PATH
         NO_CMAKE_PATH
         NO_SYSTEM_ENVIRONMENT_PATH
         NO_CMAKE_SYSTEM_PATH
      )

      SET(JASPER_INCLUDE_DIR ${INCLUDE_TMP})

      # set LIBRARY_DIR
      FIND_PATH(LIBRARY_TMP NAMES libjasper.a PATHS
         ${JASPER_PREFIX}/lib/
         NO_DEFAULT_PATH
         NO_CMAKE_ENVIRONMENT_PATH
         NO_CMAKE_PATH
         NO_SYSTEM_ENVIRONMENT_PATH
         NO_CMAKE_SYSTEM_PATH
      )

      SET(JASPER_LIBRARY ${LIBRARY_TMP}/libjasper.a)

   ELSE(JASPER_PREFIX)
      MESSAGE(STATUS "[JASPER] External SYSTEM Search LIBRARY...")

      # only for unix
      IF(UNIX) 
        # set INCLUDE_DIR
        FIND_PATH(JASPER_INCLUDE_DIR NAMES jasper.h PATHS
          /usr/local/include/jasper
          /usr/include/jasper
        )
        
        # set LIBRARY_DIR
        FIND_LIBRARY(JASPER_LIBRARY NAMES libjasper.a PATHS
          /usr/local/lib 
          /usr/lib 
        )
      ENDIF(UNIX)
   ENDIF(JASPER_PREFIX)

   IF (JASPER_INCLUDE_DIR AND JASPER_LIBRARY)
     SET(JASPER_FOUND TRUE)
   ENDIF(JASPER_INCLUDE_DIR AND JASPER_LIBRARY)

   IF (JASPER_FOUND)

        MESSAGE(STATUS "[JASPER] Found library : ${JASPER_LIBRARY}")
        MESSAGE(STATUS "[JASPER] Found include : ${JASPER_INCLUDE_DIR}")

   ELSE(JASPER_FOUND)

        MESSAGE(STATUS "[JASPER] Could not find JASPER project, so call an internal JASPER project !")

   ENDIF(JASPER_FOUND)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Jasper" DEFAULT_MSG JASPER_INCLUDE_DIR JASPER_LIBRARY)

