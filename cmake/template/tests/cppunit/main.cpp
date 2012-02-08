/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière 
 * 
 * Géoportail SAV <geop_services@geoportail.fr>
 * 
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 * 
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * 
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

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

