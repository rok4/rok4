#Défini le nom du projet 
project(Rok4Server)

list(GET ROK4_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET ROK4_VERSION 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET ROK4_VERSION 2 CPACK_PACKAGE_VERSION_PATCH)

cmake_minimum_required(VERSION 2.6)

########################################
#Attention aux chemins
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules ${CMAKE_MODULE_PATH})

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../target)
endif(NOT DEFINED DEP_PATH)

if(NOT DEFINED ROK4LIBSDIR)
  set(ROK4LIBSDIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
endif(NOT DEFINED ROK4LIBSDIR)

set(BUILD_SHARED_LIBS OFF)

#Build Type si les build types par défaut de CMake ne conviennent pas
if(DEBUG_BUILD)
  set(CMAKE_BUILD_TYPE debugbuild)
  set(CMAKE_CXX_FLAGS_DEBUGBUILD "-g -O0 -msse -msse2 -msse3 -lrt")
  set(CMAKE_C_FLAGS_DEBUGBUILD "-g -std=c99 -lrt")
else(DEBUG_BUILD)
  set(CMAKE_BUILD_TYPE specificbuild)
  set(CMAKE_CXX_FLAGS_SPECIFICBUILD "-O3 -msse -msse2 -msse3 -lrt")
  set(CMAKE_C_FLAGS_SPECIFICBUILD "-std=c99 -lrt")
endif(DEBUG_BUILD)


########################################
#définition des fichiers sources
configure_file(config.h.in config.h ESCAPE_QUOTES @ONLY)

add_subdirectory(po)

set(rok4core_SRCS  GetFeatureInfoEncoder.cpp MetadataURL.cpp ResourceLocator.cpp LegendURL.cpp Style.cpp ConfLoader.cpp Layer.cpp Level.cpp Message.cpp Pyramid.cpp Request.cpp ResponseSender.cpp ServiceException.cpp TileMatrix.cpp TileMatrixSet.cpp Rok4Api.cpp Keyword.cpp Rok4Server.cpp ProcessFactory.cpp WebService.cpp Source.cpp UtilsWMS.cpp UtilsWMTS.cpp UtilsTMS.cpp 
TileMatrixSetXML.cpp TileMatrixXML.cpp ServerXML.cpp ServicesXML.cpp LayerXML.cpp StyleXML.cpp PyramidXML.cpp LevelXML.cpp)
set(rok4server_SRCS main.cpp )
#set(rok4apitest_SRCS test_api.c )
#set(rok4commandtest_SRCS test_command.cpp )


add_library(rok4core STATIC ${rok4core_SRCS})

add_executable(rok4 ${rok4server_SRCS})
#add_executable(test_api ${rok4apitest_SRCS})
#add_executable(test_command ${rok4commandtest_SRCS})


########################################
#Définition des dépendances.
include(ROK4Dependencies)

set(DEP_INCLUDE_DIR ${FCGI_INCLUDE_DIR} ${IMAGE_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${CURL_INCLUDE_DIR})


if(BUILD_OBJECT)
    set (DEP_INCLUDE_DIR ${DEP_INCLUDE_DIR})
endif(BUILD_OBJECT)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR})

#Listes des bibliothèques à lier avec l'éxecutable à mettre à jour
set(DEP_LIBRARY fcgi tinyxml logger image proj zlib tiff png curl)

if(BUILD_OBJECT)
    set (DEP_LIBRARY ${DEP_LIBRARY})
endif(BUILD_OBJECT)

target_link_libraries(rok4core ${DEP_LIBRARY} )
target_link_libraries(rok4 rok4core)
#target_link_libraries(test_api rok4core)
#target_link_libraries(test_command rok4core)

#set_target_properties(test_api PROPERTIES LINKER_LANGUAGE C)

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
    include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPPUNIT_INCLUDE_DIR})
    ENABLE_TESTING()
    add_definitions(-DUNITTEST)
    # Exécution des tests unitaires CppUnit
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/cppunit/CppUnit*.cpp" )
    ADD_EXECUTABLE(UnitTester-${PROJECT_NAME} tests/cppunit/main.cpp ${UnitTests_SRCS} tests/cppunit/TimedTestListener.cpp tests/cppunit/XmlTimedTestOutputterHook.cpp )
    #Bibliothèque à lier (ajouter la cible (executable/library) du projet
    TARGET_LINK_LIBRARIES(UnitTester-${PROJECT_NAME} cppunit rok4core ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_RADOS_LIBS_INIT} ${CMAKE_OPENSSL_LIBS_INIT}  ${CMAKE_DL_LIBS})
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  - adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester-${PROJECT_NAME} ${TestName})
    ENDFOREACH(test)

    #Transformation des sorties Xml CppUnit en Xml Junit
    find_package(Xsltproc)
    if(XSLTPROC_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/cppunit2junit.xsl ${CMAKE_CURRENT_BINARY_DIR}/cppunit2junit.xsl @ONLY)
    add_custom_command(OUTPUT ${PROJECT_NAME}-junit-xml COMMAND $<TARGET_FILE:UnitTester-${PROJECT_NAME}>
		COMMAND ${XSLTPROC_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/cppunit2junit.xsl ${CMAKE_CURRENT_BINARY_DIR}/cpptestresults.xml > ${CMAKE_BINARY_DIR}/testresult-${PROJECT_NAME}.xml )

    add_custom_target(junitxml DEPENDS ${PROJECT_NAME}-junit-xml)
    endif(XSLTPROC_FOUND)
    
endif(UNITTEST)


########################################
#Installation dans les répertoires par défauts
#Pour installer dans le répertoire /opt/projet :
#cmake -DCMAKE_INSTALL_PREFIX=/opt/projet 

#Installe les différentes sortie du projet (projet, projetcore ou UnitTester-${PROJECT_NAME})
# ici uniquement "projet"
INSTALL(TARGETS rok4 rok4core
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

#Installe les différents headers nécessaires
#FILE(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
INSTALL(FILES Rok4Api.h DESTINATION include)

#Installe la documentation
INSTALL(FILES LICENCE DESTINATION share/doc/${PROJECT_NAME})

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
