project(libImage)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake/Modules ${CMAKE_MODULE_PATH})

set(LIBIMAGE_DEP_PATH ${CMAKE_SOURCE_DIR}/../../target)
set(ROK4_DEP_PATH ${LIBIMAGE_DEP_PATH})
set(CMAKE_INSTALL_PREFIX ${LIBIMAGE_DEP_PATH})

MESSAGE(STATUS "Deploy in : ${LIBIMAGE_DEP_PATH}")

#set(BUILD_SHARED_LIBS OFF)

find_package(Threads REQUIRED)
if (NOT CMAKE_USE_PTHREADS_INIT)
  message(FATAL_ERROR "Need the PThread library")
endif (NOT CMAKE_USE_PTHREADS_INIT)
 

#Dépendances
find_package(Jpeg REQUIRED)
find_package(TIFF REQUIRED)
find_package(Logger REQUIRED)
find_package(Proj REQUIRED)
find_package(Zlib REQUIRED)


#Définition des dépendances.
set(LIBIMAGE_DEP_INCLUDE_DIR ${IMAGE_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${LOGGER_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

add_library(thread STATIC IMPORTED)
set_property(TARGET thread PROPERTY IMPORTED_LOCATION ${CMAKE_THREAD_LIBS_INIT})

add_library(jpeg STATIC IMPORTED)
set_property(TARGET jpeg PROPERTY IMPORTED_LOCATION ${JPEG_LIBRARY})

add_library(logger STATIC IMPORTED)
set_property(TARGET logger PROPERTY IMPORTED_LOCATION ${LOGGER_LIBRARY})

add_library(proj STATIC IMPORTED)
set_property(TARGET proj PROPERTY IMPORTED_LOCATION ${PROJ_LIBRARY})

add_library(tiff STATIC IMPORTED)
set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${TIFF_LIBRARY})

add_library(zlib STATIC IMPORTED)
set_property(TARGET zlib PROPERTY IMPORTED_LOCATION ${ZLIB_LIBRARY})

set(LIBIMAGE_DEP_LIBRARY proj logger tiff zlib jpeg)

#Image Build Type
set(CMAKE_BUILD_TYPE libimage)
set(CMAKE_CXX_FLAGS_LIBIMAGE "-O3 -p -msse -msse2 -msse3")
set(CMAKE_C_FLAGS_LIBIMAGE "")


#définition des fichiers sources

set(LibImage_SRCS CompoundImage.cpp Data.cpp Decoder.cpp ExtendedCompoundImage.cpp Grid.cpp Kernel.cpp LibtiffImage.cpp MirrorImage.cpp ReprojectedImage.cpp ResampledImage.cpp)


include_directories(${CMAKE_CURRENT_BINARY_DIR} ${LIBIMAGE_DEP_INCLUDE_DIR})

add_library(image STATIC ${LibImage_SRCS})
#add_library(imageshared SHARED ${LibImage_SRCS})

#target_link_libraries(imageshared ${LIBIMAGE_DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(image ${LIBIMAGE_DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
SET_TARGET_PROPERTIES(image PROPERTIES OUTPUT_NAME image)
#SET_TARGET_PROPERTIES(imageshared PROPERTIES OUTPUT_NAME image)

#LibImage Test
#Activé uniquement si la variable UNITTEST est vrai
if(UNITTEST)
  find_package(CppUnit REQUIRED)
    add_library(cppunit STATIC IMPORTED)
    set_property(TARGET cppunit PROPERTY IMPORTED_LOCATION ${CPPUNIT_LIBRARY})

    include_directories(${CMAKE_CURRENT_BINARY_DIR} ${ROK4_DEP_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    ENABLE_TESTING()

    # Exécution des tests unitaires CppUnit de Rok4
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
  "tests/cppunit/CppUnit*.cpp" )
    ADD_EXECUTABLE(UnitTester tests/cppunit/main.cpp ${UnitTests_SRCS} )
    TARGET_LINK_LIBRARIES(UnitTester cppunit image ${LIBIMAGE_DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} )
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  ######  adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester ${TestName})
    ENDFOREACH(test)
endif(UNITTEST)
