project(testqtuio)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake/Modules ${CMAKE_MODULE_PATH})

set(ROK4_DEP_PATH ${CMAKE_SOURCE_DIR}/../target)
set(CMAKE_INSTALL_PREFIX ${ROK4_DEP_PATH})

MESSAGE(STATUS "Deploy in : ${ROK4_DEP_PATH}")

set(BUILD_SHARED_LIBS OFF)

find_package(Threads REQUIRED)
if (NOT CMAKE_USE_PTHREADS_INIT)
  message(FATAL_ERROR "Need the PThread library")
endif (NOT CMAKE_USE_PTHREADS_INIT)
 

#Rok4Server
find_package(Fcgi REQUIRED)
find_package(Image REQUIRED)
find_package(Jpeg REQUIRED)
find_package(Logger REQUIRED)
find_package(Proj REQUIRED)
find_package(TinyXML REQUIRED)
find_package(Zlib REQUIRED)


#Be4
#find_package(TIFF REQUIRED)

#Définition des dépendances.
#set(ROK4_DEP_LIB ${FCGI_LIBRARY} ${IMAGE_LIBRARY} ${JPEG_LIBRARY} ${LOGGER_LIBRARY} ${PROJ_LIBRARY} ${TINYXML_LIBRARY} ${ZLIB_LIBRARY})
set(ROK4_DEP_INCLUDE_DIR ${FCGI_INCLUDE_DIR} ${IMAGE_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

add_library(thread STATIC IMPORTED)
set_property(TARGET thread PROPERTY IMPORTED_LOCATION ${CMAKE_THREAD_LIBS_INIT})

add_library(fcgi STATIC IMPORTED)
set_property(TARGET fcgi PROPERTY IMPORTED_LOCATION ${FCGI_LIBRARY})

add_library(image STATIC IMPORTED)
set_property(TARGET image PROPERTY IMPORTED_LOCATION ${IMAGE_LIBRARY})

add_library(jpeg STATIC IMPORTED)
set_property(TARGET jpeg PROPERTY IMPORTED_LOCATION ${JPEG_LIBRARY})

add_library(logger STATIC IMPORTED)
set_property(TARGET logger PROPERTY IMPORTED_LOCATION ${LOGGER_LIBRARY})

add_library(proj STATIC IMPORTED)
set_property(TARGET proj PROPERTY IMPORTED_LOCATION ${PROJ_LIBRARY})

add_library(tinyxml STATIC IMPORTED)
set_property(TARGET tinyxml PROPERTY IMPORTED_LOCATION ${TINYXML_LIBRARY})

add_library(zlib STATIC IMPORTED)
set_property(TARGET zlib PROPERTY IMPORTED_LOCATION ${ZLIB_LIBRARY})

set(ROK4_DEP_LIBRARY fcgi image jpeg logger proj tinyxml zlib)

#Rok4 Build Type
set(CMAKE_BUILD_TYPE rok4server)
set(CMAKE_CXX_FLAGS_ROK4SERVER "-g -O0 -msse -msse2 -msse3")
set(CMAKE_C_FLAGS_ROK4SERVER "")


#définition des fichiers sources

set(Rok4Objects_SRCS BilEncoder.cpp CapabilitiesBuilder.cpp ConfLoader.cpp CRS.cpp FileDataSource.cpp JPEGEncoder.cpp Layer.cpp Level.cpp Message.cpp PNGEncoder.cpp Pyramid.cpp Request.cpp ResponseSender.cpp ServiceException.cpp TiffEncoder.cpp TileMatrix.cpp TileMatrixSet.cpp )
set(Rok4Server_SRCS Rok4Server.cpp ${Rok4Objects_SRCS} )


include_directories(${CMAKE_CURRENT_BINARY_DIR} ${ROK4_DEP_INCLUDE_DIR})

add_executable(Rok4Server ${Rok4Server_SRCS})
target_link_libraries(Rok4Server ${ROK4_DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

#Rok4 Test
#Activé uniquement si la variable ROK4TEST est vrai
if(ROK4TEST)
  find_package(CppUnit REQUIRED)
    add_library(cppunit STATIC IMPORTED)
    set_property(TARGET cppunit PROPERTY IMPORTED_LOCATION ${CPPUNIT_LIBRARY})

    include_directories(${CMAKE_CURRENT_BINARY_DIR} ${ROK4_DEP_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    ENABLE_TESTING()

    # Exécution des tests unitaires CppUnit de Rok4
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
  "tests/cppunit/CppUnit*.cpp" )
    ADD_EXECUTABLE(UnitTester tests/cppunit/main.cpp ${UnitTests_SRCS} ${Rok4Objects_SRCS} )
    TARGET_LINK_LIBRARIES(UnitTester cppunit ${ROK4_DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} )
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  ######  adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester ${TestName})
    ENDFOREACH(test)
	

endif(ROK4TEST)
#find_package(Xerces REQUIRED) #TODO Use Xerces-C with ccpunit ???
