#Défini le nom du projet
project(Be4)

#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
list(GET BE4_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET BE4_VERSION 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET BE4_VERSION 2 CPACK_PACKAGE_VERSION_PATCH)

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
  include(DoxygenConfig)
endif(NOT DEFINED BUILD_DOC)

if(UNITTEST)
  enable_testing()
endif(UNITTEST)

include(ROK4Dependencies)

configure_file(be4version.h.in be4version.h ESCAPE_QUOTES @ONLY)
add_subdirectory(cache2work)
add_subdirectory(composeNtiff)
add_subdirectory(createNodata)
add_subdirectory(decimateNtiff)
add_subdirectory(manageNodata)
add_subdirectory(merge4tiff)
add_subdirectory(mergeNtiff)
add_subdirectory(overlayNtiff)
add_subdirectory(pyramide)
add_subdirectory(tiff2gray)
add_subdirectory(tiff2tile)
add_subdirectory(slab2tiles)
#######################
# utilitaire de test :
add_subdirectory(checkImage)
add_subdirectory(tiffck)
######################

# Documentation Doxygen
if(BUILD_DOC)
    include(DoxygenConfig)
endif(BUILD_DOC)
