#Défini le nom du projet
project(Rok4Generation)

#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
list(GET ROK4_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET ROK4_VERSION 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET ROK4_VERSION 2 CPACK_PACKAGE_VERSION_PATCH)

cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules ${CMAKE_MODULE_PATH})

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../target)
endif(NOT DEFINED DEP_PATH)

set(ROK4LIBSDIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
if(NOT DEFINED BUILD_DEPENDENCIES)
  set(BUILD_DEPENDENCIES TRUE BOOL)
endif(NOT DEFINED BUILD_DEPENDENCIES)

if(UNITTEST)
  enable_testing()
endif(UNITTEST)

include(ROK4Dependencies)


add_subdirectory(main/)

add_subdirectory(tools/cache2work)
add_subdirectory(tools/checkWork)
add_subdirectory(tools/composeNtiff)
add_subdirectory(tools/decimateNtiff)
add_subdirectory(tools/manageNodata)
add_subdirectory(tools/merge4tiff)
add_subdirectory(tools/mergeNtiff)
add_subdirectory(tools/overlayNtiff)
add_subdirectory(tools/work2cache)
add_subdirectory(tools/pbf2cache)

