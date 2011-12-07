#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include "TimedTestListener.h"
#include "XmlTimedTestOutputterHook.h"

main( int argc, char* argv[] )
{
  // Create the event manager and test controller
  CPPUNIT_NS::TestResult controller;
  TimedTestListener ttlistener;

  // Add a listener that colllects test result
  CPPUNIT_NS::TestResultCollector result;
  controller.addListener( &result );        
  controller.addListener( &ttlistener);

  // Add a listener that print dots as test run.
  //CPPUNIT_NS::BriefTestProgressListener progress;
  //controller.addListener( &progress );      

  //controller.push
  
  // Add the top suite to the test runner
  CPPUNIT_NS::TestRunner runner;
  
  if ( argc == 1 ) {
	runner.addTest( CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest() );
  }
  if ( argc == 2 ) {
	  runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry(argv[1]).makeTest());
  }
  
  runner.run( controller );
  
  // Print test in a compiler compatible format.
  CPPUNIT_NS::CompilerOutputter outputter( &result, std::cerr );
  outputter.write();   

  //XML Output
  std::ofstream xmlFileOut("cpptestresults.xml");
  CPPUNIT_NS::XmlOutputter xmlOut(&result, xmlFileOut);
  XmlTimedTestOutputterHook *xmlTimeHook = new XmlTimedTestOutputterHook(&ttlistener);
  xmlOut.addHook(xmlTimeHook);
  xmlOut.write();
 
  
  return result.wasSuccessful() ? 0 : 1;
}

