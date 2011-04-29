#Défini le nom du projet 
project(be4)
#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")

cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules ${CMAKE_MODULE_PATH})

set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../target)
set(ROK4LIBSDIR ${CMAKE_CURRENT_SOURCE_DIR}../lib)
set(BUILD_DEPENDENCIES TRUE BOOL)
set(BUILD_DOC TRUE BOOL)
set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
if(UNITTEST)
  enable_testing()
endif(UNITTEST)


include(ROK4Dependencies)

MESSAGE(STATUS "Building Be4 generation toolchain")
add_subdirectory(DallesBase)
add_subdirectory(merge4tiff)
add_subdirectory(tiff2tile)
add_subdirectory(tiff2gray)
add_subdirectory(pyramide)
