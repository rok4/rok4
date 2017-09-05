#Défini le nom du projet 
project(openjpeg)

#définit la version du projet : 2.0.0 MAJOR.MINOR.PATCH
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")

########################################

#Attention aux chemins
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules ${CMAKE_MODULE_PATH})

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../target)
endif(NOT DEFINED DEP_PATH)

if(NOT ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(JPEG2000_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
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

# SET(
#     libopenjpeg_SRCS
#     bio.c cio.c dwt.c event.c image.c invert.c j2k.c jp2.c mct.c
#     mqc.c openjpeg.c opj_clock.c pi.c raw.c t1.c t2.c tcd.c tgt.c function_list.c
# )

SET(
    libopenjpeg_SRCS
    bio.c           dwt.c            image.c   jp2.c  openjpeg.c      pi.c            t1.c                tcd.c           tpix_manager.c
    cidx_manager.c  event.c          invert.c  mct.c  opj_clock.c     ppix_manager.c  t1_generate_luts.c  tgt.c
    cio.c           function_list.c  j2k.c     mqc.c  phix_manager.c  raw.c           t2.c                thix_manager.c
)


ADD_LIBRARY(jpeg2000 STATIC ${libopenjpeg_SRCS})


########################################

include(ROK4Dependencies)

#set(DEP_INCLUDE_DIR ${FCGI_INCLUDE_DIR} ${IMAGE_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR} ${LZW_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} )

#Listes des bibliothèques à liées avec l'éxecutable à mettre à jour
#set(DEP_LIBRARY fcgi tinyxml logger image proj jpeg zlib tiff lzw)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR})

target_link_libraries(jpeg2000 ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

########################################
#Documentation Doxygen
#Utilisation du fichier 
# if(BUILD_DOC)
#     include(DoxygenConfig)
# endif(BUILD_DOC)
# 
INSTALL(FILES AUTHORS CHANGES LICENSE NEWS README THANKS DESTINATION doc/${PROJECT_NAME})

########################################
#Installation dans les répertoires par défauts
#Pour installer dans le répertoire /opt/projet :
#cmake -DCMAKE_INSTALL_PREFIX=/opt/projet 

#Installe les différentes sortie du projet (projet, projetcore ou UnitTester)
# ici uniquement "projet"
INSTALL(TARGETS jpeg2000 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

#Installe les différents headers nécessaires
FILE(GLOB headers-${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
INSTALL(FILES ${headers-${PROJECT_NAME}} DESTINATION include)

########################################
# Paramétrage de la gestion de package CPack
# Génère un fichier PROJET-VERSION-OS-ARCH.tar.gz 

SET(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${BUILD_ARCHITECTURE}")
INCLUDE(CPack)
