#Défini le nom du projet 
project(Rok4)
#définit la version du projet
#Lecture de la version de ROK4 dans le fichier du projet
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/README version_line REGEX "ROK4.*[0-9]+\\.[0-9]+\\.[0-9]+(-SNAPSHOT)?")
STRING(SUBSTRING ${version_line} 15 -1 version)
if( ${version} STREQUAL "0.0.0" )
    if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pom.xml )
        FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/pom.xml version_line REGEX "<!--TAG_VERSION-->")
        STRING(REGEX REPLACE "[ ]*<version>([0-9]+\\.[0-9]+\\.[0-9]+(-SNAPSHOT)?)</version>[ ]*<!--TAG_VERSION-->[ ]*" "\\1" version "${version_line}" )
    endif( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pom.xml )
endif( ${version} STREQUAL "0.0.0" )

STRING(REPLACE "." ";" ROK4_VERSION ${version})
STRING(REPLACE "." ";" BE4_VERSION ${version})
list(GET ROK4_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET ROK4_VERSION 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET ROK4_VERSION 2 CPACK_PACKAGE_VERSION_PATCH)

cmake_minimum_required(VERSION 2.6)

set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)

if(NOT DEFINED CPACK_INCLUDE_TOPLEVEL_DIRECTORY)
  SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
endif(NOT DEFINED CPACK_INCLUDE_TOPLEVEL_DIRECTORY)

cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_MODULE_PATH ${Rok4_SOURCE_DIR}/cmake/Modules ${CMAKE_MODULE_PATH})
message(STATUS "Modules CMake : ${CMAKE_MODULE_PATH}")

set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/target)
endif(NOT DEFINED DEP_PATH)

set(ROK4LIBSDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
if(NOT DEFINED BUILD_DEPENDENCIES)
  set(BUILD_DEPENDENCIES TRUE)
endif(NOT DEFINED BUILD_DEPENDENCIES)
if(NOT DEFINED BUILD_ROK4)
  set(BUILD_ROK4 TRUE)
endif(NOT DEFINED BUILD_ROK4)
if(NOT DEFINED BUILD_BE4)
  set(BUILD_BE4 TRUE)
endif(NOT DEFINED BUILD_BE4)
if(NOT DEFINED BUILD_DOC)
  set(BUILD_DOC TRUE)
  set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
endif(NOT DEFINED BUILD_DOC)
if(NOT DEFINED DEBUG_BUILD)
  set(DEBUG_BUILD FALSE)
endif(NOT DEFINED DEBUG_BUILD)
if(UNITTEST)
  enable_testing()
  set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
endif(UNITTEST)

if(DEB_PACKAGE)
  set(USE_SYSTEM_LIB TRUE)
  include(Debian)
else(DEB_PACKAGE)
  set(CPACK_GENERATOR "TGZ")
endif(DEB_PACKAGE)

if(RPM_PACKAGE)
  set(USE_SYSTEM_LIB FALSE)
  include(RPM)
else(RPM_PACKAGE)
  set(CPACK_GENERATOR "TGZ")
endif(RPM_PACKAGE)

##################################################################################
# optional external dependencies

# try to configure and build OPENJPEG support
SET(WITH_OPENJPEG FALSE CACHE BOOL "Determines whether OPENJPEG support should be built")

IF(WITH_OPENJPEG)
  SET(OPENJPEG_PREFIX "" CACHE PATH "Path to OPENJPEG base directory")
  SET(USE_DRIVERJPEG2000 TRUE)
ENDIF(WITH_OPENJPEG)

IF(WITH_OPENJPEG)
  MESSAGE(STATUS "OPENJPEG, search external package in system or in a custom path...")
  FIND_PACKAGE(OPENJPEG)
ENDIF(WITH_OPENJPEG)

IF(OPENJPEG_FOUND)
  # message
  MESSAGE(STATUS "OPENJPEG, found (external) !")
  SET(HAVE_OPENJPEG TRUE)
  # SET (USE_DRIVERJPEG2000 TRUE)
  # target
  # FIXME : a t on besoin de cette lib. ?
  # ADD_LIBRARY(openjp2 STATIC IMPORTED)
  # ADD_LIBRARY(openmj2 STATIC IMPORTED)
  # FIXME : a t on besoin de cette lib. ?
  # SET_PROPERTY(TARGET openjp2 PROPERTY IMPORTED_LOCATION ${OPENJPEG_LIBRARY})
  # SET_PROPERTY(TARGET openmj2 PROPERTY IMPORTED_LOCATION ${OPENJPEG_LIBRARY})
ENDIF(OPENJPEG_FOUND)
##################################################################################

include(${Rok4_SOURCE_DIR}/cmake/Modules/ROK4Dependencies.cmake)

if(BUILD_ROK4)
MESSAGE(STATUS "Building ROK4 Server")
  add_subdirectory(rok4)
endif(BUILD_ROK4)

if(BUILD_BE4)
MESSAGE(STATUS "Building Be4 generation toolchain")
  add_subdirectory(be4)
endif(BUILD_BE4)

#Installe les fichiers de configurations
INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/"
  DESTINATION config)
