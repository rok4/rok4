#Défini le nom du projet 
project(be4)
#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
#Lecture de la version dans le fichier README
if(NOT DEFINED ROK4_VERSION)
        FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../README tmp REGEX "BE4.*[0-9]+\\.[0-9]+\\.[0-9]+$")
        STRING(SUBSTRING ${tmp} 15 -1 subtmp)
        STRING(REPLACE "." ";" ROK4_VERSION ${subtmp})
endif(NOT DEFINED ROK4_VERSION)
list(GET ROK4_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET ROK4_VERSION 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET ROK4_VERSION 2 CPACK_PACKAGE_VERSION_PATCH)
cmake_minimum_required(VERSION 2.6)


cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules ${CMAKE_MODULE_PATH})

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../target)
endif(NOT DEFINED DEP_PATH)

set(ROK4LIBSDIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
if(NOT DEFINED BUILD_DEPENDENCIES)
  set(BUILD_DEPENDENCIES TRUE BOOL)
endif(NOT DEFINED BUILD_DEPENDENCIES)
if(NOT DEFINED BUILD_DOC)
  set(BUILD_DOC TRUE BOOL)
  set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
endif(NOT DEFINED BUILD_DOC)



if(UNITTEST)
  enable_testing()
endif(UNITTEST)


include(ROK4Dependencies)

MESSAGE(STATUS "Building Be4 generation toolchain")
add_subdirectory(mergeNtiff)
add_subdirectory(merge4tiff)
add_subdirectory(tiff2tile)
add_subdirectory(tiff2gray)
add_subdirectory(removeWhite)
add_subdirectory(pyramide)
add_subdirectory(tiffck)
add_subdirectory(createNodata)
add_subdirectory(nodataIdentifier)
