#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <iostream>
#include <vector>
#include "ConfLoader.h"
#include "tinyxml.h"
#include "Style.h"


class CppUnitConfLoaderStyle : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CppUnitConfLoaderStyle );
	// enregistrement des methodes de tests à jouer :
	CPPUNIT_TEST(emptyDoc);
	CPPUNIT_TEST(emptyElement);
	CPPUNIT_TEST_SUITE_END();

protected:
	TiXmlDocument* styleDoc;
	TiXmlElement* styleElem 	;
	TiXmlElement* identifierElem;
	TiXmlElement* titleElem ;
	TiXmlElement* abstractElem ;
	TiXmlElement* keywordsElem ;
	TiXmlElement* keyword1Elem ;
	TiXmlElement* keyword2Elem ;
	TiXmlElement* LegendURLElem ;

	
public:
	void setUp();
	
	void emptyDoc();
	void emptyDocInspire();
	void emptyElement();
	void teardDown();
	

};


CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitConfLoaderStyle );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CppUnitConfLoaderStyle, "CppUnitConfLoaderStyle" );

void CppUnitConfLoaderStyle::setUp()
{
	styleDoc = new TiXmlDocument();
	styleDoc->LinkEndChild(new TiXmlDeclaration("1.0","UTF-8",""));

	styleElem = new TiXmlElement ( "style" );
	
	identifierElem= new TiXmlElement ( "Identifier" );
	
	titleElem = new TiXmlElement ( "Title" );
	
	abstractElem = new TiXmlElement ( "Abstract" );
	
	keywordsElem = new TiXmlElement ( "Keywords" );
	
	keyword1Elem = new TiXmlElement ( "Keyword" );
	keyword2Elem = new TiXmlElement ( "Keyword" );
	
	LegendURLElem = new TiXmlElement ( "LegendURL" );
	
	
	styleDoc->LinkEndChild(styleElem);
	
	
}


void CppUnitConfLoaderStyle::teardDown()
{
	delete styleDoc;
}

void CppUnitConfLoaderStyle::emptyDoc()
{
	Style *style = ConfLoader::parseStyle(styleDoc,std::string(),false);
	CPPUNIT_ASSERT_MESSAGE("Style Normal",style == NULL);
}

void CppUnitConfLoaderStyle::emptyDocInspire()
{
	Style *style = ConfLoader::parseStyle(styleDoc,std::string(),true);
	CPPUNIT_ASSERT_MESSAGE("Style Inspire",style == NULL);
}


void CppUnitConfLoaderStyle::emptyElement()
{
	styleDoc->LinkEndChild(styleElem);
	Style *style = ConfLoader::parseStyle(styleDoc,std::string(),false);
	CPPUNIT_ASSERT_MESSAGE("Tag Style",style == NULL);
	
	styleElem->LinkEndChild(identifierElem);
	styleElem->LinkEndChild(titleElem);
	styleElem->LinkEndChild(abstractElem);
	styleElem->LinkEndChild(keywordsElem);
	styleElem->LinkEndChild(LegendURLElem);
	style = ConfLoader::parseStyle(styleDoc,std::string(),false);
	CPPUNIT_ASSERT_MESSAGE("All Tags No Data",style == NULL);
	
	identifierElem->LinkEndChild(new TiXmlText("normal"));
	style = ConfLoader::parseStyle(styleDoc,std::string(),false);
	CPPUNIT_ASSERT_MESSAGE("Identifier set",style == NULL);
	
	titleElem->LinkEndChild(new TiXmlText("normal"));
	style = ConfLoader::parseStyle(styleDoc,std::string(),false);
	CPPUNIT_ASSERT_MESSAGE("Title set",style != NULL);
	CPPUNIT_ASSERT_MESSAGE("Title Content",style->getId().compare("normal")==0);
	CPPUNIT_ASSERT_MESSAGE("Title Content",style->getTitles().at(0).compare("normal")==0);
	
}



