#Défini le nom du projet 
project(Rok4)

#définit la version du projet
#Lecture de la version de ROK4 dans le fichier du projet
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/README.md version_line REGEX "ROK4 Version : [0-9A-Za-z\\-_\\.]+")
STRING(SUBSTRING ${version_line} 15 -1 ROK4_VERSION)

MESSAGE("Version ${ROK4_VERSION}")

cmake_minimum_required(VERSION 2.8.4)

set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)

if(NOT DEFINED CPACK_INCLUDE_TOPLEVEL_DIRECTORY)
  SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
endif(NOT DEFINED CPACK_INCLUDE_TOPLEVEL_DIRECTORY)

set (CMAKE_CXX_FLAGS "-std=c++11 -lstdc++ -lm")

set(CMAKE_MODULE_PATH ${Rok4_SOURCE_DIR}/cmake/Modules ${CMAKE_MODULE_PATH})
message(STATUS "Modules CMake : ${CMAKE_MODULE_PATH}")

set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)

if(NOT DEFINED DEP_PATH)
  set(DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/target)
endif(NOT DEFINED DEP_PATH)

set(ROK4LIBSDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK -DBOOST_SYSTEM_USE_UTF8)

if(NOT DEFINED BUILD_DEPENDENCIES)
  set(BUILD_DEPENDENCIES TRUE CACHE BOOL "Build dependencies")
endif(NOT DEFINED BUILD_DEPENDENCIES)

if(NOT DEFINED BUILD_ROK4)
  set(BUILD_ROK4 TRUE CACHE BOOL "Build Rok4Server")
endif(NOT DEFINED BUILD_ROK4)

if(NOT DEFINED BUILD_BE4)
  set(BUILD_BE4 TRUE CACHE BOOL "Build Rok4Generation")
endif(NOT DEFINED BUILD_BE4)

if(NOT DEFINED BUILD_OBJECT)
  set(BUILD_OBJECT FALSE CACHE BOOL "Build Object storage")
  set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
endif(NOT DEFINED BUILD_OBJECT)

if(NOT DEFINED KDU_USE)
  set(KDU_USE FALSE CACHE BOOL "Build libimage using kakadu (to read JPEG 2000)")
endif(NOT DEFINED KDU_USE)

if(KDU_USE)
  if(NOT DEFINED KDU_LIBRARY_PATH)
    set(KDU_LIBRARY_PATH "/usr/kakadu-6.4" CACHE STRING "Path to kakadu library directory (to read JPEG 2000), containing libkdu.a and libkdu_aux.a. Not necessary if kakadu path is in the environment variable PATH")
  endif(NOT DEFINED KDU_LIBRARY_PATH)
  
  if(NOT DEFINED KDU_THREADING)
    set(KDU_THREADING "0" CACHE STRING "Number of threads when using Kakadu")
  endif(NOT DEFINED KDU_THREADING)
endif(KDU_USE)

if(NOT DEFINED BUILD_DOC)
  set(BUILD_DOC TRUE CACHE BOOL "Build Documentation")
  set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
endif(NOT DEFINED BUILD_DOC)

if(NOT DEFINED DEBUG_BUILD)
  set(DEBUG_BUILD FALSE CACHE BOOL "Mode debug ")
endif(NOT DEFINED DEBUG_BUILD)

set(UNITTEST FALSE CACHE BOOL "Build Test")
if(UNITTEST)
  enable_testing()
  set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
  set(PACKAGE_LIB TRUE)
endif(UNITTEST)

if(NOT DEFINED PACKAGE_LIB)
  set(PACKAGE_LIB FALSE CACHE BOOL "Do no include lib in final package")
endif(NOT DEFINED PACKAGE_LIB)

if(DEB_PACKAGE)
  set(USE_SYSTEM_LIB TRUE)
  include(Debian)
elseif(RPM_PACKAGE)
  set(USE_SYSTEM_LIB FALSE)
  include(RPM)
else(DEB_PACKAGE)
  set(CPACK_GENERATOR "TGZ")
endif(DEB_PACKAGE)

if(NOT DEFINED USE_SYSTEM_LIB)
  set(USE_SYSTEM_LIB FALSE)
endif(NOT DEFINED USE_SYSTEM_LIB)


configure_file(rok4version.h.in rok4version.h ESCAPE_QUOTES @ONLY)

######################################## DOCUMENTATION

if(BUILD_DOC)

  add_custom_target(doc)

  # Documentation Doxygen

  FILE(GLOB dox_configs ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/Doxyfile.*.in)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    
    MESSAGE("-- Detecting documentation language")
    FOREACH(doxconf ${dox_configs})
        STRING(LENGTH ${doxconf} doxconf_length)
        MATH( EXPR doxconf_pos "${doxconf_length} - 5")
        STRING(SUBSTRING ${doxconf} ${doxconf_pos} 2 doxconf_lang)
        MESSAGE("  - adding ${doxconf_lang}")
        set (doxconf_out ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.${doxconf_lang})
        configure_file(${doxconf} ${doxconf_out} @ONLY)
        FILE(STRINGS ${doxconf_out} doxconf_outdir_line REGEX "OUTPUT_DIRECTORY.*=.*$")
        STRING(REGEX REPLACE "OUTPUT_DIRECTORY.*= *" "" doxconf_outdir ${doxconf_outdir_line})
        add_custom_target(doc-${doxconf_lang}
            mkdir -p ${doxconf_outdir}
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.${doxconf_lang} 
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen" VERBATIM)
        add_dependencies(doc doc-${doxconf_lang})
    ENDFOREACH(doxconf)
  endif(DOXYGEN_FOUND)

  # Documentation NaturalDocs
  find_package(NaturalDocs)
  if(NATURALDOCS_FOUND)

      MESSAGE("  - adding Natuladocs")
      add_custom_command(OUTPUT make_naturaldocs_command 
        COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/target/docs/naturaldocs/html 
        COMMAND naturaldocs -r --input ${CMAKE_CURRENT_BINARY_DIR}/rok4generation/main/src/ --exclude-input ${CMAKE_CURRENT_BINARY_DIR}/rok4generation/main/src/blib -img ${CMAKE_CURRENT_SOURCE_DIR}/docs/images --no-auto-group --documented-only --output HTML ${CMAKE_CURRENT_SOURCE_DIR}/target/docs/naturaldocs/html --project ${CMAKE_CURRENT_SOURCE_DIR}/docs/naturaldocs/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/rok4generation/main/)

      add_custom_target(make_naturaldocs_target DEPENDS make_naturaldocs_command)

      add_dependencies(doc make_naturaldocs_target)

  endif(NATURALDOCS_FOUND)

  # Documentation mkdocs
  find_package(Mkdocs)
  if(MKDOCS_FOUND)
    
    MESSAGE("  - adding mkdocs")
          
    add_custom_command(OUTPUT make_mkdocs_command 
      COMMAND rm -r ${CMAKE_CURRENT_SOURCE_DIR}/target/docs/mkdocs
      COMMAND LC_ALL=C.UTF-8 mkdocs build --config-file ${CMAKE_CURRENT_SOURCE_DIR}/docs/mkdocs/mkdocs.yml --site-dir /tmp/mkdocs-rok4
      COMMAND mv /tmp/mkdocs-rok4 ${CMAKE_CURRENT_SOURCE_DIR}/target/docs/mkdocs
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_custom_target(make_mkdocs_target DEPENDS make_mkdocs_command)
    add_dependencies(doc make_mkdocs_target)
        
  endif(MKDOCS_FOUND)


  MESSAGE("  - adding index")
        
  add_custom_command(OUTPUT make_indexdoc_command 
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/docs/index.html ${CMAKE_CURRENT_SOURCE_DIR}/docs/images/favicon.png ${CMAKE_CURRENT_SOURCE_DIR}/docs/images/rok4.png ${CMAKE_CURRENT_SOURCE_DIR}/target/docs/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  add_custom_target(make_indexdoc_target DEPENDS make_indexdoc_command)
  add_dependencies(doc make_indexdoc_target)

  # Installation
  INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/target/docs/" DESTINATION share/doc/rok4)

endif(BUILD_DOC)
    
##################################################################################

include(${Rok4_SOURCE_DIR}/cmake/Modules/ROK4Dependencies.cmake)

if(BUILD_ROK4)
  MESSAGE(STATUS "Building ROK4 Server")
  add_subdirectory(rok4server)
endif(BUILD_ROK4)

if(BUILD_BE4)
  MESSAGE(STATUS "Building generation tools")
  add_subdirectory(rok4generation)
endif(BUILD_BE4)

if(DEB_PACKAGE)

  #Installe les fichiers de configurations
  INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/" DESTINATION /etc/rok4/config)
  #Installe les fichiers de service

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/service/rok4")
    INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/service/rok4" 
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      DESTINATION /etc/init.d)
  endif()

else(DEB_PACKAGE)

  #Installe les fichiers de configurations
  INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/" DESTINATION etc/rok4/config)
  
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/service/rok4")
    #Installe les fichiers de service
    INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/service/rok4"
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      DESTINATION etc/init.d)
  endif()

endif(DEB_PACKAGE)
