#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <iostream>
#include <vector>
#include "ConfLoader.h"



class CppUnitConfLoader : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CppUnitConfLoader );
	// enregistrement des methodes de tests à jouer :
	CPPUNIT_TEST_SUITE_END();

protected:

public:
	void setUp();

protected:

};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitConfLoader );

void CppUnitConfLoader::setUp()
{
}



