# CMake module to search for OPENJPEG library
#
# headers and librairies are searched for in OPENJPEG_HOME dir,
#
# If it's found it sets OPENJPEG_FOUND to TRUE
# and following variables are set:
#    OPENJPEG_INCLUDE_DIR
#    OPENJPEG_LIBRARY

   # Path overwritten by user with option : cmake -D OPENJPEG_PREFIX=...
   # Path search by default

   IF(OPENJPEG_PREFIX)
      MESSAGE(STATUS "[OPENJPEG] External CUSTOM Search LIBRARY in : ${OPENJPEG_PREFIX}")

      # set INCLUDE_DIR
      FIND_PATH(INCLUDE_TMP NAMES openjpeg.h PATHS   
         ${OPENJPEG_PREFIX}/include/openjpeg-2.0/
         NO_DEFAULT_PATH
         NO_CMAKE_ENVIRONMENT_PATH
         NO_CMAKE_PATH
         NO_SYSTEM_ENVIRONMENT_PATH
         NO_CMAKE_SYSTEM_PATH
      )

      SET(OPENJPEG_INCLUDE_DIR ${INCLUDE_TMP})

      # set LIBRARY_DIR
      # FIXME : libopenmj2.a ?
      FIND_PATH(LIBRARY_TMP NAMES libopenjp2.a PATHS
         ${OPENJPEG_PREFIX}/lib/
         NO_DEFAULT_PATH
         NO_CMAKE_ENVIRONMENT_PATH
         NO_CMAKE_PATH
         NO_SYSTEM_ENVIRONMENT_PATH
         NO_CMAKE_SYSTEM_PATH
      )

      # FIXME : libopenmj2.a ?
      SET(OPENJPEG_LIBRARY ${LIBRARY_TMP}/libopenjp2.a)

   ELSE(OPENJPEG_PREFIX)
      MESSAGE(STATUS "[OPENJPEG] External SYSTEM Search LIBRARY...")

      # only for unix
      IF(UNIX) 
        # set INCLUDE_DIR
        FIND_PATH(OPENJPEG_INCLUDE_DIR NAMES openjpeg.h PATHS
          /usr/local/include/openjpeg-2.0
          /usr/include/openjpeg-2.0
        )
        
        # set LIBRARY_DIR
        # FIXME : libopenmj2 ?
        FIND_LIBRARY(OPENJPEG_LIBRARY NAMES libopenjp2.a PATHS
          /usr/local/lib 
          /usr/lib 
        )
      ENDIF(UNIX)
   ENDIF(OPENJPEG_PREFIX)

   IF (OPENJPEG_INCLUDE_DIR AND OPENJPEG_LIBRARY)
     SET(OPENJPEG_FOUND TRUE)
   ENDIF(OPENJPEG_INCLUDE_DIR AND OPENJPEG_LIBRARY)

   IF (OPENJPEG_FOUND)

        MESSAGE(STATUS "[OPENJPEG] Found library : ${OPENJPEG_LIBRARY}")
        MESSAGE(STATUS "[OPENJPEG] Found include : ${OPENJPEG_INCLUDE_DIR}")

   ELSE(OPENJPEG_FOUND)

        MESSAGE(STATUS "[OPENJPEG] Could not find OPENJPEG project, so call an internal OPENJPEG project !")

   ENDIF(OPENJPEG_FOUND)

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "OpenJpeg" DEFAULT_MSG OPENJPEG_INCLUDE_DIR OPENJPEG_LIBRARY)

