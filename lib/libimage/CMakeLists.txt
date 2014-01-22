#Défini le nom du projet 
project(libimage)

#définit la version du projet : 0.0.1 MAJOR.MINOR.PATCH
list(GET ROK4_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET ROK4_VERSION 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET ROK4_VERSION 2 CPACK_PACKAGE_VERSION_PATCH)

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
# Définition des fichiers sources

CONFIGURE_FILE(image_config.h.in image_config.h ESCAPE_QUOTES @ONLY)

SET(
    libimage_SRCS Palette.cpp Data.cpp Decoder.cpp
    FileImage.cpp LibtiffImage.cpp LibpngImage.cpp Rok4Image.cpp
    ReprojectedImage.cpp ResampledImage.cpp Kernel.cpp Interpolation.cpp
    MirrorImage.cpp StyledImage.cpp EstompageImage.cpp
    ExtendedCompoundImage.cpp CompoundImage.cpp MergeImage.cpp
    Grid.cpp CRS.cpp TiffEncoder.cpp
    BilEncoder.cpp JPEGEncoder.cpp PNGEncoder.cpp FileDataSource.cpp PaletteConfig.cpp PaletteDataSource.cpp
    Format.cpp TiffHeaderDataSource.cpp
)

# OPTION : 'sources' JASPER
IF(HAVE_JASPER)
  SET(optional_libimage_SRCS Libjp2Image.cpp Jp2DriverJasper.cpp)
ENDIF(HAVE_JASPER)

# OPTION : 'sources' KDU
IF(HAVE_KAKADU)
  SET(optional_libimage_SRCS Libjp2Image.cpp Jp2DriverKakadu.cpp)
ENDIF(HAVE_KAKADU)

# OPTION : 'sources' OPENJPEG
IF(HAVE_OPENJPEG)
  SET(optional_libimage_SRCS Libjp2Image.cpp Jp2DriverOpenJpeg.cpp)
ENDIF(HAVE_OPENJPEG)

ADD_LIBRARY(image STATIC ${libimage_SRCS} ${optional_libimage_SRCS})

########################################
# Définition des dépendances.

INCLUDE(ROK4Dependencies)

SET(DEP_INCLUDE_DIR ${JPEG_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} ${LZW_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${PKB_INCLUDE_DIR})

# OPTION : 'include' JASPER
IF(HAVE_JASPER)
  SET(OPTIONAL_INCLUDE_DIR ${JASPER_INCLUDE_DIR})
ENDIF(HAVE_JASPER)

# OPTION : 'include' KDU
IF(HAVE_KAKADU)
  SET(OPTIONAL_INCLUDE_DIR ${KAKADU_INCLUDE_DIR})
ENDIF(HAVE_KAKADU)

# OPTION : 'include' OPENJPEG
IF(HAVE_OPENJPEG)
  SET(OPTIONAL_INCLUDE_DIR ${OPENJPEG_INCLUDE_DIR})
ENDIF(HAVE_OPENJPEG)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${DEP_INCLUDE_DIR} ${OPTIONAL_INCLUDE_DIR})

########################################
# Listes des bibliothèques à liées avec l'éxecutable
SET(DEP_LIBRARY logger proj jpeg zlib tiff lzw pkb png)

# OPTION : 'lib' JASPER
IF(HAVE_JASPER)
  SET(OPTIONAL_LIBRARY jasper)
ENDIF(HAVE_JASPER)

# OPTION : 'lib' KDU
IF(HAVE_KAKADU)
  SET(OPTIONAL_LIBRARY kdu)
ENDIF(HAVE_KAKADU)

# OPTION : 'lib' OPENJPEG
IF(HAVE_OPENJPEG)
  # FIXME : openjm2 ?
  SET(OPTIONAL_LIBRARY openjp2)
ENDIF(HAVE_OPENJPEG)

TARGET_LINK_LIBRARIES(image ${DEP_LIBRARY} ${OPTIONAL_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

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
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/cppunit/CppUnit*.cpp" )
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

     add_custom_command(OUTPUT ${PROJECT_NAME}-junit-xml COMMAND $<TARGET_FILE:UnitTester-${PROJECT_NAME}>
                COMMAND ${XSLTPROC_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/cppunit2junit.xsl ${CMAKE_CURRENT_BINARY_DIR}/cpptestresults.xml > ${CMAKE_BINARY_DIR}/testresult-${PROJECT_NAME}.xml )

    add_custom_target(junitxml DEPENDS ${PROJECT_NAME}-junit-xml)
    endif(XSLTPROC_FOUND)

endif(UNITTEST)

########################################
#Documentation Doxygen
#Utilisation du fichier 
if(BUILD_DOC)
    include(DoxygenConfig)
endif(BUILD_DOC)

INSTALL(FILES LICENCE README DESTINATION doc/${PROJECT_NAME})

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
