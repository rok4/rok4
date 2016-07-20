#Défini le nom du projet
project(curl)
#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
SET(CPACK_PACKAGE_VERSION_MAJOR "7")
SET(CPACK_PACKAGE_VERSION_MINOR "35")
SET(CPACK_PACKAGE_VERSION_PATCH "0")

cmake_minimum_required(VERSION 2.8.4)

########################################
#Attention aux chemins
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules ${CMAKE_MODULE_PATH})

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../target)
endif(NOT DEFINED DEP_PATH)

if(NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(CURL_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/install/include PARENT_SCOPE)
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


if(NOT TARGET curl)
  add_library(curl STATIC IMPORTED)
endif(NOT TARGET curl)

set(ENV{CFLAGS} "-O3")

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/build/config.status)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/build/.libs/libcurl.a)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build)
#add_custom_command(TARGET curl-configure PRE_BUILD COMMAND mkdir -p build)
add_custom_command(OUTPUT build/config.status COMMAND CFLAGS=-O3 ${CMAKE_CURRENT_SOURCE_DIR}/curl-7.43.0/configure --enable-static=yes --enable-shared=no --prefix=${CMAKE_CURRENT_BINARY_DIR}/install --enable-debug --enable-optimize --enable-hidden-symbols --disable-ftp --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-sspi --disable-libcurl-option --disable-soname-bump --disable-cookies --disable-tls-srp --disable-crypto-auth --disable-ldap --disable-ares --disable-manual --without-zlib --without-winssl --without-darwinssl --without-gnutls --without-polar-ssl --without-cyassl --without-nss --without-axtls --without-ca-bundle --without-ca-path --without-libmetalink --without-libidn --without-winidn --without-nghttp2 --without-libssh2 --without-librtmp ${CURL_OPTIONS} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build)
add_custom_command(OUTPUT build/.libs/libcurl.a COMMAND make COMMAND make install DEPENDS curl-configure WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build)

add_custom_target(curl-configure DEPENDS build/config.status)
add_custom_target(curl-build ALL DEPENDS build/.libs/libcurl.a)

if(NOT TARGET curl)
add_library(curl STATIC IMPORTED)
endif(NOT TARGET curl)

set(CURL_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libcurl.a PARENT_SCOPE)
set_property(TARGET curl PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libcurl.a)
add_dependencies(curl-build curl-configure)
add_dependencies(curl curl-build)

########################################
#Définition des dépendances.
include(ROK4Dependencies)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/install/lib)

########################################
#Installation dans les répertoires par défauts
#Pour installer dans le répertoire /opt/projet :
#cmake -DCMAKE_INSTALL_PREFIX=/opt/projet

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
