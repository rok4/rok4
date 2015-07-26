# Install script for directory: /rok4-tobuild/lib/libimage

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/doc/libimage" TYPE FILE FILES
    "/rok4-tobuild/lib/libimage/LICENCE"
    "/rok4-tobuild/lib/libimage/README"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/rok4-tobuild/build/lib/libimage/libimage.a")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/rok4-tobuild/lib/libimage/BilEncoder.h"
    "/rok4-tobuild/lib/libimage/BilzImage.h"
    "/rok4-tobuild/lib/libimage/BoundingBox.h"
    "/rok4-tobuild/lib/libimage/CompoundImage.h"
    "/rok4-tobuild/lib/libimage/CRS.h"
    "/rok4-tobuild/lib/libimage/Data.h"
    "/rok4-tobuild/lib/libimage/DecimatedImage.h"
    "/rok4-tobuild/lib/libimage/Decoder.h"
    "/rok4-tobuild/lib/libimage/EmptyImage.h"
    "/rok4-tobuild/lib/libimage/EstompageImage.h"
    "/rok4-tobuild/lib/libimage/ExtendedCompoundImage.h"
    "/rok4-tobuild/lib/libimage/FileDataSource.h"
    "/rok4-tobuild/lib/libimage/FileImage.h"
    "/rok4-tobuild/lib/libimage/Format.h"
    "/rok4-tobuild/lib/libimage/Grid.h"
    "/rok4-tobuild/lib/libimage/Image.h"
    "/rok4-tobuild/lib/libimage/Interpolation.h"
    "/rok4-tobuild/lib/libimage/Jpeg2000Image.h"
    "/rok4-tobuild/lib/libimage/JPEGEncoder.h"
    "/rok4-tobuild/lib/libimage/Kernel.h"
    "/rok4-tobuild/lib/libimage/LibkakaduImage.h"
    "/rok4-tobuild/lib/libimage/LibopenjpegImage.h"
    "/rok4-tobuild/lib/libimage/LibpngImage.h"
    "/rok4-tobuild/lib/libimage/LibtiffImage.h"
    "/rok4-tobuild/lib/libimage/Line.h"
    "/rok4-tobuild/lib/libimage/MergeImage.h"
    "/rok4-tobuild/lib/libimage/MirrorImage.h"
    "/rok4-tobuild/lib/libimage/OneBitConverter.h"
    "/rok4-tobuild/lib/libimage/Palette.h"
    "/rok4-tobuild/lib/libimage/PaletteConfig.h"
    "/rok4-tobuild/lib/libimage/PaletteDataSource.h"
    "/rok4-tobuild/lib/libimage/PNGEncoder.h"
    "/rok4-tobuild/lib/libimage/RawImage.h"
    "/rok4-tobuild/lib/libimage/ReprojectedImage.h"
    "/rok4-tobuild/lib/libimage/ResampledImage.h"
    "/rok4-tobuild/lib/libimage/Rok4Image.h"
    "/rok4-tobuild/lib/libimage/StyledImage.h"
    "/rok4-tobuild/lib/libimage/TiffDeflateEncoder.h"
    "/rok4-tobuild/lib/libimage/TiffEncoder.h"
    "/rok4-tobuild/lib/libimage/TiffHeader.h"
    "/rok4-tobuild/lib/libimage/TiffHeaderDataSource.h"
    "/rok4-tobuild/lib/libimage/TiffLZWEncoder.h"
    "/rok4-tobuild/lib/libimage/TiffNodataManager.h"
    "/rok4-tobuild/lib/libimage/TiffPackBitsEncoder.h"
    "/rok4-tobuild/lib/libimage/TiffRawEncoder.h"
    "/rok4-tobuild/lib/libimage/Utils.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

