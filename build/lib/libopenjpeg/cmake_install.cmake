# Install script for directory: /rok4-tobuild/lib/libopenjpeg

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
    SET(CMAKE_INSTALL_CONFIG_NAME "debugbuild")
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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/doc/openjpeg" TYPE FILE FILES
    "/rok4-tobuild/lib/libopenjpeg/AUTHORS"
    "/rok4-tobuild/lib/libopenjpeg/CHANGES"
    "/rok4-tobuild/lib/libopenjpeg/LICENSE"
    "/rok4-tobuild/lib/libopenjpeg/NEWS"
    "/rok4-tobuild/lib/libopenjpeg/README"
    "/rok4-tobuild/lib/libopenjpeg/THANKS"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/rok4-tobuild/build/lib/libopenjpeg/libjpeg2000.a")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/rok4-tobuild/lib/libopenjpeg/invert.h"
    "/rok4-tobuild/lib/libopenjpeg/bio.h"
    "/rok4-tobuild/lib/libopenjpeg/cidx_manager.h"
    "/rok4-tobuild/lib/libopenjpeg/cio.h"
    "/rok4-tobuild/lib/libopenjpeg/dwt.h"
    "/rok4-tobuild/lib/libopenjpeg/event.h"
    "/rok4-tobuild/lib/libopenjpeg/function_list.h"
    "/rok4-tobuild/lib/libopenjpeg/image.h"
    "/rok4-tobuild/lib/libopenjpeg/indexbox_manager.h"
    "/rok4-tobuild/lib/libopenjpeg/j2k.h"
    "/rok4-tobuild/lib/libopenjpeg/jp2.h"
    "/rok4-tobuild/lib/libopenjpeg/mct.h"
    "/rok4-tobuild/lib/libopenjpeg/mqc.h"
    "/rok4-tobuild/lib/libopenjpeg/openjpeg.h"
    "/rok4-tobuild/lib/libopenjpeg/opj_clock.h"
    "/rok4-tobuild/lib/libopenjpeg/opj_includes.h"
    "/rok4-tobuild/lib/libopenjpeg/opj_intmath.h"
    "/rok4-tobuild/lib/libopenjpeg/opj_inttypes.h"
    "/rok4-tobuild/lib/libopenjpeg/opj_malloc.h"
    "/rok4-tobuild/lib/libopenjpeg/opj_stdint.h"
    "/rok4-tobuild/lib/libopenjpeg/pi.h"
    "/rok4-tobuild/lib/libopenjpeg/raw.h"
    "/rok4-tobuild/lib/libopenjpeg/t1.h"
    "/rok4-tobuild/lib/libopenjpeg/t1_luts.h"
    "/rok4-tobuild/lib/libopenjpeg/t2.h"
    "/rok4-tobuild/lib/libopenjpeg/tcd.h"
    "/rok4-tobuild/lib/libopenjpeg/tgt.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

