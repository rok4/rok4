#include <cppunit/extensions/HelperMacros.h>

#include <string.h>
#include "format.h"

class CppUnitFormat : public CPPUNIT_NS::TestFixture {
	CPPUNIT_TEST_SUITE (CppUnitFormat);
	CPPUNIT_TEST(testFormat);
	CPPUNIT_TEST(formatFromString);
	CPPUNIT_TEST_SUITE_END();
	
public:
	void testFormat();
	void formatFromString();
};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitFormat );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CppUnitFormat, "CppUnitFormat" );

void CppUnitFormat::testFormat()
{
	eformat_data ukn = UNKNOWN;
	eformat_data tri8 = TIFF_RAW_INT8;
	eformat_data tji8 = TIFF_JPG_INT8; 
	eformat_data tpi8 = TIFF_PNG_INT8;
	eformat_data tli8 = TIFF_LZW_INT8;
	eformat_data trf32 = TIFF_RAW_FLOAT32;
	eformat_data tlf32 = TIFF_LZW_FLOAT32;
	
	CPPUNIT_ASSERT_MESSAGE("Unknown is false", !(ukn));
	CPPUNIT_ASSERT_MESSAGE("TIFF_RAW_INT8", format::toString(tri8).compare("TIFF_RAW_INT8") == 0);
	CPPUNIT_ASSERT_MESSAGE("TIFF_JPG_INT8", format::toString(tji8).compare("TIFF_JPG_INT8") == 0);
	CPPUNIT_ASSERT_MESSAGE("TIFF_PNG_INT8", format::toString(tpi8).compare("TIFF_PNG_INT8") == 0);
	CPPUNIT_ASSERT_MESSAGE("TIFF_LZW_INT8", format::toString(tli8).compare("TIFF_LZW_INT8") == 0);
	CPPUNIT_ASSERT_MESSAGE("TIFF_RAW_FLOAT32", format::toString(trf32).compare("TIFF_RAW_FLOAT32") == 0);
	CPPUNIT_ASSERT_MESSAGE("TIFF_LZW_FLOAT32", format::toString(tlf32).compare("TIFF_LZW_FLOAT32") == 0);
}

void CppUnitFormat::formatFromString()
{
	CPPUNIT_ASSERT_MESSAGE("TIFF_RAW_INT8", format::fromString("TIFF_RAW_INT8") == TIFF_RAW_INT8);
	CPPUNIT_ASSERT_MESSAGE("TIFF_JPG_INT8", format::fromString("TIFF_JPG_INT8") == TIFF_JPG_INT8);
	CPPUNIT_ASSERT_MESSAGE("TIFF_PNG_INT8", format::fromString("TIFF_PNG_INT8") == TIFF_PNG_INT8);
	CPPUNIT_ASSERT_MESSAGE("TIFF_LZW_INT8", format::fromString("TIFF_LZW_INT8") == TIFF_LZW_INT8);
	CPPUNIT_ASSERT_MESSAGE("TIFF_RAW_FLOAT32", format::fromString("TIFF_RAW_FLOAT32") == TIFF_RAW_FLOAT32);
	CPPUNIT_ASSERT_MESSAGE("TIFF_LZW_FLOAT32", format::fromString("TIFF_LZW_FLOAT32") == TIFF_LZW_FLOAT32);
	CPPUNIT_ASSERT_MESSAGE("Wrong Value", format::fromString("Wrong") == UNKNOWN);
}
