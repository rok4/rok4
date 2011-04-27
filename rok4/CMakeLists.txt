#Défini le nom du projet 
project(Rok4Server)
#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "1")

cmake_minimum_required(VERSION 2.6)

########################################
#Attention aux chemins
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SURCE_DIR}/../cmake/Modules ${CMAKE_MODULE_PATH})

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SURCE_DIR}/../target)
endif(NOT DEFINED DEP_PATH)

if(NOT DEFINED ROK4LIBSDIR)
  set(ROK4LIBSDIR ${CMAKE_CURRENT_SURCE_DIR}/../lib)
endif(NOT DEFINED ROK4LIBSDIR)

set(BUILD_SHARED_LIBS OFF)

#Build Type si les build types par défaut de CMake ne conviennent pas
set(CMAKE_BUILD_TYPE specificbuild)
set(CMAKE_CXX_FLAGS_SPECIFICBUILD "-g -O0 -msse -msse2 -msse3")
set(CMAKE_C_FLAGS_SPECIFICBUILD "")


########################################
#définition des fichiers sources

set(rok4core_SRCS BilEncoder.cpp CapabilitiesBuilder.cpp ConfLoader.cpp CRS.cpp FileDataSource.cpp JPEGEncoder.cpp Layer.cpp Level.cpp Message.cpp PNGEncoder.cpp Pyramid.cpp Request.cpp ResponseSender.cpp ServiceException.cpp TiffEncoder.cpp TileMatrix.cpp TileMatrixSet.cpp )
set(rok4server_SRCS Rok4Server.cpp )



add_library(rok4core STATIC ${rok4core_SRCS})

add_executable(rok4 ${rok4server_SRCS})
########################################
#Définition des dépendances.
include(ROK4Dependencies)

set(DEP_INCLUDE_DIR ${FCGI_INCLUDE_DIR} ${IMAGE_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR})

#Listes des bibliothèques à liées avec l'éxecutable à mettre à jour
set(DEP_LIBRARY fcgi tinyxml logger image proj jpeg zlib) 

target_link_libraries(rok4core ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(rok4 rok4core)

########################################
# Gestion des tests unitaires (CPPUnit)
# Les fichiers tests doivent être dans le répertoire tests/cppunit
# Les fichiers tests doivent être nommés CppUnitNOM_DU_TEST.cpp
# le lanceur de test doit être dans le répertoire tests/cppunit
# le lanceur de test doit être nommés main.cpp (disponible dans cmake/template)
# L'éxecutable "UnitTester-${PROJECT_NAME}" sera généré pour lancer tous les tests
# Vérifier les bibliothèques liées au lanceur de tests
#Activé uniquement si la variable UNITTEST est vraie
if(UNITTEST)
    include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    ENABLE_TESTING()

    # Exécution des tests unitaires CppUnit
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
  "tests/cppunit/CppUnit*.cpp" )
    ADD_EXECUTABLE(UnitTester-${PROJECT_NAME} tests/cppunit/main.cpp ${UnitTests_SRCS} )
    #Bibliothèque à lier (ajouter la cible (executable/library) du projet
    TARGET_LINK_LIBRARIES(UnitTester-${PROJECT_NAME} cppunit rok4core ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} )
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  - adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester-${PROJECT_NAME} ${TestName})
    ENDFOREACH(test)
endif(UNITTEST)

########################################
#Documentation Doxygen
#Utilisation du fichier 
if(BUILD_DOC)
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)
endif(BUILD_DOC)


########################################
#Installation dans les répertoires par défauts
#Pour installer dans le répertoire /opt/projet :
#cmake -DCMAKE_INSTALL_PREFIX=/opt/projet 

#Installe les différentes sortie du projet (projet, projetcore ou UnitTester-${PROJECT_NAME})
# ici uniquement "projet"
INSTALL(TARGETS rok4 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

#Installe les différents headers nécessaires
#FILE(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
#INSTALL(FILES ${headers}
#  DESTINATION include)

#Installe la documentation
INSTALL(FILES README
  DESTINATION doc/${PROJECT_NAME})

if(BUILD_DOC)
if(DOXYGEN_FOUND)
INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc/"
  DESTINATION doc/${PROJECT_NAME})
endif(DOXYGEN_FOUND)
endif(BUILD_DOC)

#Installe la configuration
INSTALL(DIRECTORY "../config/"
  DESTINATION config)


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
