message(STATUS "Building a debian package")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alexandre Viard <alexandre.viard@ign.fr>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "ROK4 WMS/WMTS Server and BE4 data generation tools")
set(CPACK_DEBIAN_PACKAGE_SECTION "science")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
else()
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
endif()

set(CPACK_BINARY_DEB "ON")
set(CPACK_GENERATOR "DEB")
