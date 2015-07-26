# Install script for directory: /rok4-tobuild

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/rok4")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "1")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/config" TYPE DIRECTORY FILES "/rok4-tobuild/config/")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  INCLUDE("/rok4-tobuild/build/lib/libfcgi/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libjpeg/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libproj/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libtinyxml/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/liblzw/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libpkb/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libz/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libtiff/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/liblogger/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libpng/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libopenjpeg/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libimage/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/lib/libxerces/cmake_install.cmake")
  INCLUDE("/rok4-tobuild/build/rok4/cmake_install.cmake")

ENDIF(NOT CMAKE_INSTALL_LOCAL_ONLY)

IF(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
ELSE(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
ENDIF(CMAKE_INSTALL_COMPONENT)

FILE(WRITE "/rok4-tobuild/build/${CMAKE_INSTALL_MANIFEST}" "")
FOREACH(file ${CMAKE_INSTALL_MANIFEST_FILES})
  FILE(APPEND "/rok4-tobuild/build/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
ENDFOREACH(file)
