#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <iostream>
#include <unistd.h>



class CppUnitTest1 : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CppUnitTest1 );
	// enregistrement des methodes de tests à jouer :
	CPPUNIT_TEST( test1methodeXXX );
	CPPUNIT_TEST( test2comportementYYY );
	CPPUNIT_TEST_SUITE_END();

protected:
	std::string* sortie;

public:
	void setUp();
	void tearDown();

protected:
	void test1methodeXXX();
	void test2comportementYYY();
};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitTest1 );

void CppUnitTest1::setUp()
{
	sortie = new std::string();	
	
}

void CppUnitTest1::tearDown()
{
	delete sortie;
}

void CppUnitTest1::test1methodeXXX()
{
	CPPUNIT_ASSERT(true);

} // test1methodeXXX


void CppUnitTest1::test2comportementYYY()
{ 
	CPPUNIT_ASSERT(false);
} // test2comportementYYY
