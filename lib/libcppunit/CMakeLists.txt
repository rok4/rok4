#Défini le nom du projet 
project(cppunit)
#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "12")
SET(CPACK_PACKAGE_VERSION_PATCH "1")

cmake_minimum_required(VERSION 2.6)

########################################
#Attention aux chemins
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules ${CMAKE_MODULE_PATH})

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../target)
endif(NOT DEFINED DEP_PATH)

if(NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(CPPUNIT_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/install/include/cppunit PARENT_SCOPE)
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

set(ENV{CFLAGS} "-O3")
add_custom_command(OUTPUT src/configure COMMAND mkdir -p src COMMAND cp -f -r -l ${CMAKE_CURRENT_SOURCE_DIR}/cppunit-1.12.1/* src)
add_custom_command(OUTPUT install/lib/libcppunit.a 
  COMMAND ./configure --enable-shared=no --prefix=${CMAKE_CURRENT_BINARY_DIR}/install --enable-doxygen=no 
  COMMAND make 
  COMMAND make install 
  DEPENDS src/configure 
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src)

add_custom_target(cppunit-build ALL DEPENDS install/lib/libcppunit.a)

if(NOT TARGET cppunit)
add_library(cppunit STATIC IMPORTED)
endif(NOT TARGET cppunit)

set_property(TARGET cppunit PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libcppunit.a)
add_dependencies(cppunit cppunit-build)


########################################
#Définition des dépendances.
#include(ROK4Dependencies)

#set(DEP_INCLUDE_DIR ${FCGI_INCLUDE_DIR} ${IMAGE_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} )

#Listes des bibliothèques à liées avec l'éxecutable à mettre à jour
#set(DEP_LIBRARY fcgi tinyxml logger image proj jpeg zlib tiff)

#include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR})

#target_link_libraries(${PROJECT_NAME} ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

########################################
#Installation dans les répertoires par défauts
#Pour installer dans le répertoire /opt/projet :
#cmake -DCMAKE_INSTALL_PREFIX=/opt/projet 

#Installe les différentes sortie du projet (projet, projetcore ou UnitTester)
# ici uniquement "projet"
#INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libz.a 
#  DESTINATION lib
#)

#Installe les différents headers nécessaires
#FILE(GLOB headers-${PROJECT_NAME} "${CMAKE_CURRENT_BINARY_DIR}/install/include/*.hxx" "${CMAKE_CURRENT_BINARY_DIR}/install/include/*.h" "${CMAKE_CURRENT_BINARY_DIR}/install/include/*.hpp")
#INSTALL(FILES ${headers-${PROJECT_NAME}}
#  DESTINATION include)

INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install/"
  DESTINATION .)



########################################
# Paramétrage de la gestion de package CPack
# Génère un fichier PROJET-VERSION-OS-32/64bit.tar.gz 


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  SET(BUILD_ARCHITECTURE "64bit")
else()
  SET(BUILD_ARCHITECTURE "32bit")
endif()
SET(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${BUILD_ARCHITECTURE}")
INCLUDE(CPack)
