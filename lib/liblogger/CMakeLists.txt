project(libLogger)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake/Modules ${CMAKE_MODULE_PATH})

set(LIBLOGGER_DEP_PATH ${CMAKE_SOURCE_DIR}/../../target)
set(ROK4_DEP_PATH ${LIBLOGGER_DEP_PATH})
set(CMAKE_INSTALL_PREFIX ${LIBLOGGER_DEP_PATH})

MESSAGE(STATUS "Deploy in : ${LIBLOGGER_DEP_PATH}")

set(BUILD_SHARED_LIBS OFF)

find_package(Threads REQUIRED)
if (NOT CMAKE_USE_PTHREADS_INIT)
  message(FATAL_ERROR "Need the PThread library")
endif (NOT CMAKE_USE_PTHREADS_INIT)
 

#Dépendances

#Définition des dépendances.
add_library(thread STATIC IMPORTED)
set_property(TARGET thread PROPERTY IMPORTED_LOCATION ${CMAKE_THREAD_LIBS_INIT})

#Logger Build Type
set(CMAKE_BUILD_TYPE liblogger)
set(CMAKE_CXX_FLAGS_LIBLOGGER "-O0 -g")
set(CMAKE_C_FLAGS_LIBLOGGER "")


#définition des fichiers sources

set(LibLogger_SRCS Accumulator.cpp Logger.cpp)


include_directories(${CMAKE_CURRENT_BINARY_DIR} ${LIBLOGGER_DEP_INCLUDE_DIR})

add_library(logger STATIC ${LibLogger_SRCS})

target_link_libraries(logger ${LIBLOGGER_DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

#LibLogger Test
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
    TARGET_LINK_LIBRARIES(UnitTester cppunit logger ${LIBLOGGER_DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} )
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  ######  adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester ${TestName})
    ENDFOREACH(test)
endif(UNITTEST)
