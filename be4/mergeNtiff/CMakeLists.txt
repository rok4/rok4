#Récupère le nom du projet parent
SET(PARENT_PROJECT_NAME ${PROJECT_NAME})

#Défini le nom du projet 
project(mergeNtiff)

#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
list(GET BE4_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET BE4_VERSION 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET BE4_VERSION 2 CPACK_PACKAGE_VERSION_PATCH)


cmake_minimum_required(VERSION 2.6)

########################################
#Attention aux chemins
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules ${CMAKE_MODULE_PATH})

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../target)
endif(NOT DEFINED DEP_PATH)

if(NOT DEFINED ROK4LIBSDIR)
  set(ROK4LIBSDIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)
endif(NOT DEFINED ROK4LIBSDIR)

set(BUILD_SHARED_LIBS OFF)


#Build Type si les build types par défaut de CMake ne conviennent pas
#set(CMAKE_BUILD_TYPE specificbuild)
#set(CMAKE_CXX_FLAGS_SPECIFICBUILD "-g -O0 -msse -msse2 -msse3")
#set(CMAKE_C_FLAGS_SPECIFICBUILD "")
if(DEBUG_BUILD)
  set(CMAKE_BUILD_TYPE debugbuild)
  set(CMAKE_CXX_FLAGS_DEBUGBUILD "-g -O0 -msse -msse2 -msse3")
  set(CMAKE_C_FLAGS_DEBUGBUILD "-g -std=c99")
else(DEBUG_BUILD)
  set(CMAKE_BUILD_TYPE specificbuild)
  set(CMAKE_CXX_FLAGS_SPECIFICBUILD "-O3 -msse -msse2 -msse3")
  set(CMAKE_C_FLAGS_SPECIFICBUILD "-std=c99")
endif(DEBUG_BUILD)



########################################
#définition des fichiers sources

set(${PROJECT_NAME}_SRCS mergeNtiff.cpp )

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS})

########################################
#Définition des dépendances.
include(ROK4Dependencies)

#set(DEP_INCLUDE_DIR ${JPEG2000_INCLUDE_DIR} ${IMAGE_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} )
#set(DEP_INCLUDE_DIR ${IMAGE_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} )
set(DEP_INCLUDE_DIR ${IMAGE_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${TIFF_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR})

#Listes des bibliothèques à liées avec l'éxecutable à mettre à jour
#set(DEP_LIBRARY tiff zlib jpeg proj logger image)
set(DEP_LIBRARY tiff proj logger image)

#target_link_libraries(${PROJECT_NAME} ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_RADOS_LIBS_INIT}  ${CMAKE_DL_LIBS})
target_link_libraries(${PROJECT_NAME} ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_RADOS_LIBS_INIT} )

########################################
# Gestion des tests unitaires (CPPUnit)
# Les fichiers tests doivent être dans le répertoire tests/cppunit
# Les fichiers tests doivent être nommés CppUnitNOM_DU_TEST.cpp
# le lanceur de test doit être dans le répertoire tests/cppunit
# le lanceur de test doit être nommés main.cpp (disponible dans cmake/template)
# L'éxecutable "UnitTester-Nom_Projet" sera généré pour lancer tous les tests
# Vérifier les bibliothèques liées au lanceur de tests
#Activé uniquement si la variable UNITTEST est vraie
if(UNITTEST)
  include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPPUNIT_INCLUDE_DIR})
  ENABLE_TESTING()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/cppunit)
    # Exécution des tests unitaires CppUnit
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/cppunit/CppUnit*.cpp" )
    ADD_EXECUTABLE(UnitTester-${PROJECT_NAME} tests/cppunit/main.cpp ${UnitTests_SRCS} )
    #Bibliothèque à lier (ajouter la cible (executable/library) du projet
    TARGET_LINK_LIBRARIES(UnitTester-${PROJECT_NAME} cppunit lib${PROJECT_NAME} ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_RADOS_LIBS_INIT}  ${CMAKE_DL_LIBS})
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  - adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester-${PROJECT_NAME} ${TestName})
    ENDFOREACH(test)
  endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/cppunit)
endif(UNITTEST)

########################################
#Documentation

INSTALL(FILES LICENCE README DESTINATION share/doc/${PARENT_PROJECT_NAME}/${PROJECT_NAME})

########################################
#Installation dans les répertoires par défauts
#Pour installer dans le répertoire /opt/projet :
#cmake -DCMAKE_INSTALL_PREFIX=/opt/projet 

#Installe les différentes sortie du projet (projet, projetcore ou UnitTester)
# ici uniquement "projet"
INSTALL(TARGETS ${PROJECT_NAME} 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

#Installe les différents headers nécessaires
FILE(GLOB headers-${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
INSTALL(FILES ${headers-${PROJECT_NAME}} DESTINATION include)


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
