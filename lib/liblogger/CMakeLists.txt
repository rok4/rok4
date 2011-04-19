#Défini le nom du projet 
project(liblogger)
#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "1")

cmake_minimum_required(VERSION 2.6)

########################################
#Attention aux chemins
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake/Modules ${CMAKE_MODULE_PATH})

set(DEP_PATH ${CMAKE_SOURCE_DIR}/../../target)

set(BUILD_SHARED_LIBS OFF)

########################################
#Définition des dépendances.
#Décommenter les dépendances nécessaires

find_package(Threads REQUIRED)
if (NOT CMAKE_USE_PTHREADS_INIT)
  message(FATAL_ERROR "Need the PThread library")
endif (NOT CMAKE_USE_PTHREADS_INIT)
add_library(thread STATIC IMPORTED)
set_property(TARGET thread PROPERTY IMPORTED_LOCATION ${CMAKE_THREAD_LIBS_INIT})

#find_package(Fcgi REQUIRED)
#add_library(fcgi STATIC IMPORTED)
#set_property(TARGET fcgi PROPERTY IMPORTED_LOCATION ${FCGI_LIBRARY})

#find_package(Image REQUIRED)
#add_library(image STATIC IMPORTED)
#set_property(TARGET image PROPERTY IMPORTED_LOCATION ${IMAGE_LIBRARY})

#find_package(Jpeg REQUIRED)
#add_library(jpeg STATIC IMPORTED)
#set_property(TARGET jpeg PROPERTY IMPORTED_LOCATION ${JPEG_LIBRARY})

#find_package(Logger REQUIRED)
#add_library(logger STATIC IMPORTED)
#set_property(TARGET logger PROPERTY IMPORTED_LOCATION ${LOGGER_LIBRARY})

#find_package(Proj REQUIRED)
#add_library(proj STATIC IMPORTED)
#set_property(TARGET proj PROPERTY IMPORTED_LOCATION ${PROJ_LIBRARY})

#find_package(TinyXML REQUIRED)
#add_library(tinyxml STATIC IMPORTED)
#set_property(TARGET tinyxml PROPERTY IMPORTED_LOCATION ${TINYXML_LIBRARY})

#find_package(Zlib REQUIRED)
#add_library(zlib STATIC IMPORTED)
#set_property(TARGET zlib PROPERTY IMPORTED_LOCATION ${ZLIB_LIBRARY})

#find_package(TIFF REQUIRED)
#add_library(tiff STATIC IMPORTED)
#set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${TIFF_LIBRARY})

#set(DEP_INCLUDE_DIR ${FCGI_INCLUDE_DIR} ${IMAGE_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${TIIF_INCLUDE_DIR} )

#Listes des bibliothèques à liées avec l'éxecutable à mettre à jour
#set(DEP_LIBRARY fcgi tinyxml logger image proj jpeg zlib tiff)

#Build Type si les build types par défaut de CMake ne conviennent pas
#set(CMAKE_BUILD_TYPE specificbuild)
#set(CMAKE_CXX_FLAGS_SPECIFICBUILD "-g -O0 -msse -msse2 -msse3")
#set(CMAKE_C_FLAGS_SPECIFICBUILD "")


########################################
#définition des fichiers sources

set(libLogger_SRCS Accumulator.cpp Logger.cpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR})

add_library(logger STATIC ${libLogger_SRCS})
target_link_libraries(logger ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

########################################
# Gestion des tests unitaires (CPPUnit)
# Les fichiers tests doivent être dans le répertoire tests/cppunit
# Les fichiers tests doivent être nommés CppUnitNOM_DU_TEST.cpp
# le lanceur de test doit être dans le répertoire tests/cppunit
# le lanceur de test doit être nommés main.cpp (disponible dans cmake/template)
# L'éxecutable "UnitTester" sera généré pour lancer tous les tests
# Vérifier les bibliothèques liées au lanceur de tests
#Activé uniquement si la variable UNITTEST est vrai
if(UNITTEST)
  find_package(CppUnit REQUIRED)
    add_library(cppunit STATIC IMPORTED)
    set_property(TARGET cppunit PROPERTY IMPORTED_LOCATION ${CPPUNIT_LIBRARY})

    include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    ENABLE_TESTING()

    # Exécution des tests unitaires CppUnit
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
  "tests/cppunit/CppUnit*.cpp" )
    ADD_EXECUTABLE(UnitTester tests/cppunit/main.cpp ${UnitTests_SRCS} )
    #Bibliothèque à lier (ajouter la cible (executable/library) du projet
    TARGET_LINK_LIBRARIES(UnitTester cppunit logger ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} )
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  - adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester ${TestName})
    ENDFOREACH(test)
endif(UNITTEST)

########################################
#Documentation Doxygen
#Utilisation du fichier 
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)


########################################
#Installation dans les répertoires par défauts
#Pour installer dans le répertoire /opt/projet :
#cmake -DCMAKE_INSTALL_PREFIX=/opt/projet 

#Installe les différentes sortie du projet (projet, projetcore ou UnitTester)
# ici uniquement "projet"
INSTALL(TARGETS logger 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

#Installe les différents headers nécessaires
FILE(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
INSTALL(FILES ${headers}
  DESTINATION include)

#Installe la documentation
INSTALL(FILES README
  DESTINATION doc/${PROJECT_NAME})
INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc/"
  DESTINATION doc/${PROJECT_NAME})



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
