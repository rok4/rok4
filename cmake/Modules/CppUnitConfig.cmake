include_directories(${CMAKE_CURRENT_BINARY_DIR} ${DEP_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
  ENABLE_TESTING()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/cppunit)
    MESSAGE("-- Detecting test classes")
    # Exécution des tests unitaires CppUnit
    FILE(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
  "tests/cppunit/CppUnit*.cpp" )
    ADD_EXECUTABLE(UnitTester-${PROJECT_NAME} tests/cppunit/main.cpp ${UnitTests_SRCS} tests/cppunit/TimedTestListener.cpp tests/cppunit/XmlTimedTestOutputterHook.cpp)
    #Bibliothèque à lier (ajouter la cible (executable/library) du projet
    TARGET_LINK_LIBRARIES(UnitTester-${PROJECT_NAME} cppunit lib${PROJECT_NAME} ${DEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
    FOREACH(test ${UnitTests_SRCS})
          MESSAGE("  - adding test ${test}")
          GET_FILENAME_COMPONENT(TestName ${test} NAME_WE)
          ADD_TEST(${TestName} UnitTester-${PROJECT_NAME} ${TestName})
    ENDFOREACH(test)
  endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/cppunit)
  #Transformation des sorties Xml CppUnit en Xml Junit
    find_package(Xsltproc)
    if(XSLTPROC_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules/cppunit2junit.xsl ${CMAKE_CURRENT_BINARY_DIR}/cppunit2junit.xsl @ONLY) 
    add_custom_command(OUTPUT ${PROJECT_NAME}-junit-xml COMMAND $<TARGET_FILE:UnitTester-${PROJECT_NAME}>
                COMMAND ${XSLTPROC_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/cppunit2junit.xsl ${CMAKE_CURRENT_BINARY_DIR}/cpptestresults.xml > ${CMAKE_BINARY_DIR}/testresult-${PROJECT_NAME}.xml )

    add_custom_target(junitxml DEPENDS ${PROJECT_NAME}-junit-xml)
    endif(XSLTPROC_FOUND)