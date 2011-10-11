#Défini le nom du projet 
project(libimage)
#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
#Lecture de la version dans le fichier README 
if(NOT DEFINED ROK4_VERSION)
        FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../../README tmp REGEX "ROK4.*[0-9]+\\.[0-9]+\\.[0-9]+$")
        STRING(SUBSTRING ${tmp} 15 -1 subtmp)
        STRING(REPLACE "." ";" ROK4_VERSION ${subtmp})
endif(NOT DEFINED ROK4_VERSION)
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

if(NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(IMAGE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
endif(NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})

if(NOT DEFINED ROK4LIBSDIR)
  set(ROK4LIBSDIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)
endif(NOT DEFINED ROK4LIBSDIR)

set(BUILD_SHARED_LIBS OFF)




#Build Type si les build types par défaut de CMake ne conviennent pas
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

set(libimage_SRCS CompoundImage.cpp Data.cpp Decoder.cpp ExtendedCompoundImage.cpp Grid.cpp Kernel.cpp LibtiffImage.cpp MirrorImage.cpp ReprojectedImage.cpp ResampledImage.cpp TiffEncoder.cpp BilEncoder.cpp JPEGEncoder.cpp PNGEncoder.cpp FileDataSource.cpp ColorizePNGEncoder.cpp PaletteDataSource.cpp )




add_library(image STATIC ${libimage_SRCS})

########################################
#Définition des dépendances.
include(ROK4Dependencies)

set(DEP_INCLUDE_DIR ${JPEG_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} )

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${DEP_INCLUDE_DIR})
#Listes des bibliothèques à liées avec l'éxecutable
set(DEP_LIBRARY logger proj jpeg zlib tiff)
target_link_libraries(image ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

########################################
# Gestion des tests unitaires (CPPUnit)
# Les fichiers tests doivent être dans le répertoire tests/cppunit
# Les fichiers tests doivent être nommés CppUnitNOM_DU_TEST.cpp
# le lanceur de test doit être dans le répertoire tests/cppunit
# le lanceur de test doit être nommés main.cpp (disponible dans cmake/template)
# L'éxecutable "UnitTester" sera généré pour lancer tous les tests
# Vérifier les bibliothèques liées au lanceur de tests
#Activé uniquement si la variable UNITTEST est vraie
if(UNITTEST)
    include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${DEP_INCLUDE_DIR} ${CPPUNIT_INCLUDE_DIR})
    ENABLE_TESTING()

    # Exécution des tests unitaires CppUnit
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
  "tests/cppunit/CppUnit*.cpp" )
    ADD_EXECUTABLE(UnitTester-${PROJECT_NAME} tests/cppunit/main.cpp ${UnitTests_SRCS} tests/cppunit/TimedTestListener.cpp tests/cppunit/XmlTimedTestOutputterHook.cpp )
    #Bibliothèque à lier (ajouter la cible (executable/library) du projet
    TARGET_LINK_LIBRARIES(UnitTester-${PROJECT_NAME} cppunit image ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  - adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester-${PROJECT_NAME} ${TestName})
    ENDFOREACH(test) 

    #Transformation des sorties Xml CppUnit en Xml Junit
    find_package(Xsltproc)
    if(XSLTPROC_FOUND)
     configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules/cppunit2junit.xsl ${CMAKE_CURRENT_BINARY_DIR}/cppunit2junit.xsl @ONLY)

     add_custom_command(OUTPUT ${PROJECT_NAME}-junit-xml
                COMMAND ${XSLTPROC_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/cppunit2junit.xsl ${CMAKE_CURRENT_BINARY_DIR}/cpptestresults.xml > ${CMAKE_BINARY_DIR}/testresult-${PROJECT_NAME}.xml )

    add_custom_target(junitxml DEPENDS ${PROJECT_NAME}-junit-xml)
    endif(XSLTPROC_FOUND)

endif(UNITTEST)

########################################
#Documentation Doxygen
#Utilisation du fichier 
if(BUILD_DOC)
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc ALL
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

#Installe les différentes sortie du projet (projet, projetcore ou UnitTester)
# ici uniquement "projet"
INSTALL(TARGETS image 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

#Installe les différents headers nécessaires
FILE(GLOB headers-${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
INSTALL(FILES ${headers-${PROJECT_NAME}}
  DESTINATION include)

#Installe la documentation
INSTALL(FILES README 
  DESTINATION doc/${PROJECT_NAME})

if(BUILD_DOC)
if(DOXYGEN_FOUND)
INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc/"
  DESTINATION doc/${PROJECT_NAME})
endif(DOXYGEN_FOUND)
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
