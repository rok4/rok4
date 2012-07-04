#Défini le nom du projet 
project(pyramide)
#définit la version du projet : 1.0.0 MAJOR.MINOR.PATCH
#Lecture de la version dans le fichier README 
if(NOT DEFINED BE4_VERSION)
        FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../../README tmp REGEX "BE4.*[0-9]+\\.[0-9]+\\.[0-9]+$")
        STRING(SUBSTRING ${tmp} 15 -1 ROK4_VERSION_TEXT)
        STRING(REPLACE "." ";" BE4_VERSION ${ROK4_VERSION_TEXT})
endif(NOT DEFINED BE4_VERSION)
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


########################################
#définition des fichiers sources

#add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS})

########################################
#Définition des dépendances.
include(ROK4Dependencies)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/src/makefile)

set(DEP_INCLUDE_DIR ${TIFF_INCLUDE_DIR} )

set(VERSION_TEXT ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Makefile.PL.in ${CMAKE_CURRENT_BINARY_DIR}/src/Makefile.PL @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/be4.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/be4.pl @ONLY)

set(ENV{CFLAGS} "-O3")
add_custom_command(OUTPUT src/makefile COMMAND mkdir -p src COMMAND cp -f -r -l ${CMAKE_CURRENT_SOURCE_DIR}/* src)
add_custom_command(OUTPUT install/lib COMMAND BE4_ENV="${CMAKE_CURRENT_BINARY_DIR}/install" make -f makefile COMMAND BE4_ENV="${CMAKE_CURRENT_BINARY_DIR}/install" make -f makefile install DEPENDS src/makefile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src)


add_custom_target(be4-build ALL DEPENDS install/lib)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR})

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
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
  "tests/cppunit/CppUnit*.cpp" )
    ADD_EXECUTABLE(UnitTester-${PROJECT_NAME} tests/cppunit/main.cpp ${UnitTests_SRCS} )
    #Bibliothèque à lier (ajouter la cible (executable/library) du projet
    TARGET_LINK_LIBRARIES(UnitTester-${PROJECT_NAME} cppunit lib${PROJECT_NAME} ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} )
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  - adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester-${PROJECT_NAME} ${TestName})
    ENDFOREACH(test)
  endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/cppunit)
endif(UNITTEST)

########################################
#Documentation Doxygen
#Utilisation du fichier 
if(BUILD_DOC)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc ALL
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  endif(DOXYGEN_FOUND)
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
  find_package(NaturalDocs)
    if(NATURALDOCS_FOUND)
	add_custom_target(doc ALL make doc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src
    COMMENT "Generating API documentation with NaturalDocs" VERBATIM
    )
    endif(NATURALDOCS_FOUND)
endif(BUILD_DOC)
INSTALL(FILES LICENCE
    DESTINATION doc/${PROJECT_NAME})


########################################
#Installation dans les répertoires par défauts
#Pour installer dans le répertoire /opt/projet :
#cmake -DCMAKE_INSTALL_PREFIX=/opt/projet 


#Installe les différents fichers nécessaires
INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install/"
  DESTINATION . USE_SOURCE_PERMISSIONS)

#Installe la documentation
INSTALL(FILES QUESTIONS README CHANGES INSTALL FIXME 
  DESTINATION doc/${PROJECT_NAME})
if(BUILD_DOC)
  if(DOXYGEN_FOUND)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc/"
  DESTINATION doc/${PROJECT_NAME})
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
  endif(DOXYGEN_FOUND)
  if(NATURALDOCS_FOUND)
     INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/src/docs/"
             DESTINATION doc/${PROJECT_NAME})   
  endif(NATURALDOCS_FOUND)
endif(BUILD_DOC)

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
