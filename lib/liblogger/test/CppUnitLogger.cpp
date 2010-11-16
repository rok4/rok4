#include <cppunit/extensions/HelperMacros.h>
#include "Logger.h"

//#define LOGGER(x) if(Logger::getAccumulator(x)) Logger::getLogger(x)


class CppUnitLogger : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitLogger );
  // enregistrement des methodes de tests à jouer :
  CPPUNIT_TEST( test_logger );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() {};

protected:
  void test_logger() {
    std::stringstream out;
    Accumulator* acc = new StreamAccumulator(out);
		Logger::setAccumulator(DEBUG, acc);

  	for(int i = 0; i < 100; i++) LOGGER(DEBUG) << i << std::endl;
		Logger::setAccumulator(DEBUG, 0);

    for(int i = 0; i < 100; i++) {
			std::string s1, s2, s3;
			int n;
      out >> s1 >> s2 >> s3 >> n;
      CPPUNIT_ASSERT_EQUAL(i, n);
    }
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitLogger );

