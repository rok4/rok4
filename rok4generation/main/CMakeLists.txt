#Récupère le nom du projet parent
SET(PARENT_PROJECT_NAME ${PROJECT_NAME})

#Défini le nom du projet 
project(pyramide)

#définit la version du projet : 1.0.0 MAJOR.MINOR.PATCH
list(GET ROK4_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET ROK4_VERSION 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET ROK4_VERSION 2 CPACK_PACKAGE_VERSION_PATCH)

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
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/4alamo-file.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/4alamo-file.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/be4-file.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/be4-file.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/be4-ceph.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/be4-ceph.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/be4-s3.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/be4-s3.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/be4-swift.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/be4-swift.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/joinCache-file.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/joinCache-file.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/joinCache-s3.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/joinCache-s3.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/joinCache-ceph.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/joinCache-ceph.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/create-layer.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/create-layer.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/create-list.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/create-list.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/coord2image.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/coord2image.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/tms-converter-gui.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/tms-converter-gui.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/be4-simulator.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/be4-simulator.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/ex-pyr.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/ex-pyr.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/pyr2pyr.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/pyr2pyr.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/sup-pyr.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/sup-pyr.pl @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/wmtSalaD.pl.in ${CMAKE_CURRENT_BINARY_DIR}/src/bin/wmtSalaD.pl @ONLY)

set(ENV{CFLAGS} "-O3")
add_custom_command(OUTPUT src/makefile COMMAND mkdir -p src COMMAND cp -f -r -l ${CMAKE_CURRENT_SOURCE_DIR}/* src)
add_custom_command(OUTPUT install_be4 COMMAND BE4_ENV="${CMAKE_CURRENT_BINARY_DIR}/install" make -f makefile COMMAND BE4_ENV="${CMAKE_CURRENT_BINARY_DIR}/install" make -f makefile install DEPENDS src/makefile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src)

add_custom_target(be4-build ALL DEPENDS install_be4)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR})

########################################

# Gestion des tests unitaires (PerlUnit)
# Les fichiers tests doivent être dans le répertoire ../tests/perlunit
# Les fichiers tests doivent être nommés PerlUnitCLASSE_TESTEE.t
# le lanceur de test doit être dans le répertoire ../tests/perlunit
# L'éxecutable "UnitTester-Nom_Projet" sera généré pour lancer tous les tests
# Activé uniquement si la variable UNITTEST est vraie

if(UNITTEST)
    include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPPUNIT_INCLUDE_DIR})
    ENABLE_TESTING()
    add_definitions(-DUNITTEST)

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/perlunit)
        # Exécution des tests unitaires PerlUnit
        FILE(GLOB_RECURSE UnitTests_SRCS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/tests/perlunit/" "PerlUnit*.t")

        FOREACH(test ${UnitTests_SRCS})
            MESSAGE(" - adding test ${test}")
            ADD_TEST(NAME "${test}" COMMAND perl -I ${CMAKE_CURRENT_BINARY_DIR}/src/lib/ ${CMAKE_CURRENT_BINARY_DIR}/src/tests/perlunit/${test})
        ENDFOREACH(test)

        # Nettoyage du dossier temporaire
        MESSAGE("  - adding cleaning")
        ADD_TEST(NAME "Cleaning" COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/src/tests/temp)

    endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/perlunit)
  
endif(UNITTEST)

########################################

# Installe les différents fichers nécessaires
INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install/" DESTINATION . USE_SOURCE_PERMISSIONS)

# Création des liens pour garder les appels à be4.pl et joinCache.pl corrects
INSTALL(CODE "execute_process(COMMAND ln -sf be4-file.pl ${CMAKE_INSTALL_PREFIX}/bin/be4.pl)")
INSTALL(CODE "execute_process(COMMAND ln -sf joinCache-file.pl ${CMAKE_INSTALL_PREFIX}/bin/joinCache.pl)")

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
