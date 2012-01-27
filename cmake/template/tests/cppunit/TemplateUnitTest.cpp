#include <cppunit/extensions/HelperMacros.h>

#include <string>

class TEMPLATECLASS : public CPPUNIT_NS::TestFixture {
    
    CPPUNIT_TEST_SUITE( TEMPLATECLASS );

    CPPUNIT_TEST( constructors );
    CPPUNIT_TEST( getters );

    CPPUNIT_TEST_SUITE_END();
    
public:
    void setUp();
    void constructors();
    void getters();
    void tearDown();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( TEMPLATECLASS );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TEMPLATECLASS, "TEMPLATECLASS" );

void TEMPLATECLASS::setUp()
{

}

void TEMPLATECLASS::constructors()
{

}
void TEMPLATECLASS::getters()
{

}

void TEMPLATECLASS::tearDown()
{

}
