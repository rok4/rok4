#Défini le nom du projet 
project(xerces)
#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "1")

cmake_minimum_required(VERSION 2.6)

########################################
#Attention aux chemins
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules ${CMAKE_MODULE_PATH})


INSTALL(DIRECTORY "xerces/"
  DESTINATION lib/xerces)



########################################
# Paramétrage de la gestion de package CPack
# Génère un fichier PROJET-VERSION-OS-ARCH.tar.gz 

SET(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${BUILD_ARCHITECTURE}")
INCLUDE(CPack)
