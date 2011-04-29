#Défini le nom du projet 
project(jpeg)
#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")

cmake_minimum_required(VERSION 2.8.4)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  SET(BUILD_ARCH "64")
else()
  SET(BUILD_ARCH "32")
endif()

########################################
#Attention aux chemins
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules ${CMAKE_MODULE_PATH})

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../target)
endif(NOT DEFINED DEP_PATH)

if(NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(JPEG_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/install/include PARENT_SCOPE)
endif(NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})

if(NOT DEFINED ROK4LIBSDIR)
  set(ROK4LIBSDIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)
endif(NOT DEFINED ROK4LIBSDIR)

set(BUILD_SHARED_LIBS OFF)


#Build Type si les build types par défaut de CMake ne conviennent pas
#set(CMAKE_BUILD_TYPE specificbuild)
#set(CMAKE_CXX_FLAGS_SPECIFICBUILD "-g -O0 -msse -msse2 -msse3")
#set(CMAKE_C_FLAGS_SPECIFICBUILD "")


########################################
#définition des fichiers sources



if(NOT TARGET jpeg)
  add_library(${PROJECT_NAME} STATIC IMPORTED)
endif(NOT TARGET jpeg)

add_custom_command(OUTPUT install/lib/libjpeg.a 
  COMMAND mkdir -p install/lib 
  COMMAND cp -f -r -l ${CMAKE_CURRENT_SOURCE_DIR}/libturbojpeg-${BUILD_ARCH}/libjpeg.a install/lib
  COMMAND mkdir -p install/include
  COMMAND cp -f -r -l ${CMAKE_CURRENT_SOURCE_DIR}/libturbojpeg-${BUILD_ARCH}/*.h install/include
  )

add_custom_target(jpeg-copy ALL DEPENDS install/lib/libjpeg.a)

set_property(TARGET jpeg PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libturbojpeg-${BUILD_ARCH}/libjpeg.a)
add_dependencies(jpeg jpeg-copy)

########################################
#Définition des dépendances.
include(ROK4Dependencies)

#set(DEP_INCLUDE_DIR ${FCGI_INCLUDE_DIR} ${IMAGE_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} )

#Listes des bibliothèques à liées avec l'éxecutable à mettre à jour
#set(DEP_LIBRARY fcgi tinyxml logger image proj jpeg zlib tiff)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR})

#target_link_libraries(${PROJECT_NAME} ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

########################################
#Installation dans les répertoires par défauts
#Pour installer dans le répertoire /opt/projet :
#cmake -DCMAKE_INSTALL_PREFIX=/opt/projet 

#Installe les différentes sortie du projet (projet, projetcore ou UnitTester)
# ici uniquement "projet"
#INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/libturbojpeg-${BUILD_ARCH}/libjpeg.a 
#  DESTINATION lib
#)

#Installe les différents headers nécessaires
#FILE(GLOB headers-${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/libturbojpeg-${BUILD_ARCH}/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/libturbojpeg-${BUILD_ARCH}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/libturbojpeg-${BUILD_ARCH}/*.hpp")
#INSTALL(FILES ${headers-${PROJECT_NAME}}
#  DESTINATION include)

INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install/"
  DESTINATION .)

########################################
# Paramétrage de la gestion de package CPack
# Génère un fichier PROJET-VERSION-OS-32/64bit.tar.gz 

SET(CPACK_GENERATOR "TGZ")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  SET(BUILD_ARCHITECTURE "64bit")
else()
  SET(BUILD_ARCHITECTURE "32bit")
endif()
SET(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${BUILD_ARCHITECTURE}")
INCLUDE(CPack)
