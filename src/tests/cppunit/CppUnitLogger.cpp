#include <cppunit/extensions/HelperMacros.h>
/* classe à tester
 * #include "MyTestClass.h"
 */
#include "Logger.h"



class CppUnitLogger : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitLogger );
  // enregistrement des methodes de tests à jouer :
  CPPUNIT_TEST( Test1 );
  CPPUNIT_TEST( Test2 );
  CPPUNIT_TEST_SUITE_END();

protected:
  float someValue;
  std::string str;
  // MyTestClass myObject;

public:
  void setUp();

protected:
  void Test1();
  void Test2();
};


CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitLogger );

void CppUnitLogger::setUp()
{
  someValue = 2.0;
  str = "Hello";
}

void CppUnitLogger::Test1()
{

//  LOGGER(INFO) << "Logger works " ;
  CPPUNIT_ASSERT_DOUBLES_EQUAL( someValue, 2.0f, 0.005f );
  someValue = 0;
  
  //System exceptions cause CppUnit to stop dead on its tracks
  //myObject.UseBadPointer();
    
  // A regular exception works nicely though
  // myObject.ThrowException();
}


void CppUnitLogger::Test2()
{
  CPPUNIT_ASSERT_EQUAL (str, std::string("Hello"));
//  CPPUNIT_ASSERT_DOUBLES_EQUAL( someValue, 0.0f, 0.005f );
}

