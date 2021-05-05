/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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

#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <iostream>
#include <vector>
#include "ConfLoader.h"
#include "tinyxml.h"
#include "Style.h"


class CppUnitConfLoaderStyle : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitConfLoaderStyle );
    // enregistrement des methodes de tests à jouer :
    CPPUNIT_TEST ( emptyDoc );
    CPPUNIT_TEST ( emptyElement );
    CPPUNIT_TEST ( validStyle );

    CPPUNIT_TEST ( emptyDocInspire );
    CPPUNIT_TEST ( emptyElementInspire );
    CPPUNIT_TEST ( validStyleInspire );

    CPPUNIT_TEST ( paletteStyle );

    CPPUNIT_TEST_SUITE_END();

protected:
    TiXmlDocument* styleDoc;
    TiXmlElement* styleElem     ;
    TiXmlElement* identifierElem;
    TiXmlElement* titleElem ;
    TiXmlElement* abstractElem ;
    TiXmlElement* keywordsElem ;
    TiXmlElement* keyword1Elem ;
    TiXmlElement* keyword2Elem ;
    TiXmlElement* LegendURLElem ;

    Style *style;


public:
    void setUp();

    void emptyDoc();
    void emptyDocInspire();

    void emptyElement();
    void emptyElementInspire();

    void validStyle();
    void validStyleInspire();

    void paletteStyle();

    void tearDown();


};


CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitConfLoaderStyle );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitConfLoaderStyle, "CppUnitConfLoaderStyle" );

void CppUnitConfLoaderStyle::setUp() {
    styleDoc = new TiXmlDocument();
    styleDoc->LinkEndChild ( new TiXmlDeclaration ( "1.0","UTF-8","" ) );

    styleElem = new TiXmlElement ( "style" );

    identifierElem= new TiXmlElement ( "Identifier" );

    titleElem = new TiXmlElement ( "Title" );

    abstractElem = new TiXmlElement ( "Abstract" );

    keywordsElem = new TiXmlElement ( "Keywords" );

    keyword1Elem = new TiXmlElement ( "Keyword" );
    keyword2Elem = new TiXmlElement ( "Keyword" );

    LegendURLElem = new TiXmlElement ( "LegendURL" );


    styleDoc->LinkEndChild ( styleElem );


}


void CppUnitConfLoaderStyle::tearDown() {
    //FIXME
    //delete styleDoc;
}

void CppUnitConfLoaderStyle::emptyDoc() {
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "Style Normal",style == NULL );
}

void CppUnitConfLoaderStyle::emptyDocInspire() {
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "Style Inspire",style == NULL );
}


void CppUnitConfLoaderStyle::emptyElement() {
    styleDoc->LinkEndChild ( styleElem );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "Tag Style",style == NULL );

    styleElem->LinkEndChild ( identifierElem );
    styleElem->LinkEndChild ( titleElem );
    styleElem->LinkEndChild ( abstractElem );
    styleElem->LinkEndChild ( keywordsElem );
    styleElem->LinkEndChild ( LegendURLElem );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "All Tags No Data",style == NULL );

}

void CppUnitConfLoaderStyle::emptyElementInspire() {
    styleDoc->LinkEndChild ( styleElem );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "Tag Style",style == NULL );

    styleElem->LinkEndChild ( identifierElem );
    styleElem->LinkEndChild ( titleElem );
    styleElem->LinkEndChild ( abstractElem );
    styleElem->LinkEndChild ( keywordsElem );
    styleElem->LinkEndChild ( LegendURLElem );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "All Tags No Data",style == NULL );
}


void CppUnitConfLoaderStyle::validStyle() {
    styleElem->LinkEndChild ( identifierElem );
    styleElem->LinkEndChild ( titleElem );
    styleElem->LinkEndChild ( abstractElem );
    styleElem->LinkEndChild ( keywordsElem );
    styleElem->LinkEndChild ( LegendURLElem );

    identifierElem->LinkEndChild ( new TiXmlText ( "normal" ) );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "Identifier set",style == NULL );

    titleElem->LinkEndChild ( new TiXmlText ( "normal" ) );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );

    CPPUNIT_ASSERT_MESSAGE ( "Title set",style != NULL );
    CPPUNIT_ASSERT_MESSAGE ( "Identifier Content",style->getId().compare ( "normal" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Title Content",style->getTitles().at ( 0 ).compare ( "normal" ) ==0 );
    TiXmlElement *title1 = new TiXmlElement ( "Title" );
    title1->LinkEndChild ( new TiXmlText ( "Title2" ) );
    TiXmlElement *title2 = new TiXmlElement ( "Title" );
    title2->LinkEndChild ( new TiXmlText ( "Title3" ) );
    styleElem->LinkEndChild ( title1 );
    styleElem->LinkEndChild ( title2 );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "Multiple Title set",style != NULL );
    CPPUNIT_ASSERT_MESSAGE ( "Multiple Title handle", style->getTitles().size() == 3 );
}

void CppUnitConfLoaderStyle::validStyleInspire() {
    styleElem->LinkEndChild ( identifierElem );
    styleElem->LinkEndChild ( titleElem );
    styleElem->LinkEndChild ( abstractElem );
    styleElem->LinkEndChild ( keywordsElem );
    styleElem->LinkEndChild ( LegendURLElem );

    identifierElem->LinkEndChild ( new TiXmlText ( "normal" ) );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "Identifier set",style == NULL );

    titleElem->LinkEndChild ( new TiXmlText ( "normal" ) );

    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "Title set",style == NULL );

    abstractElem->LinkEndChild ( new TiXmlText ( "abstract" ) );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "Abstract set",style == NULL );

    LegendURLElem->SetAttribute ( "format","image/jpg" );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "LegendURL type",style == NULL );

    LegendURLElem->SetAttribute ( "xlink:simpleLink","simple" );
    LegendURLElem->SetAttribute ( "xlink:href","http://inspire" );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "LegendURL xlink",style != NULL );

    LegendURLElem->SetAttribute ( "height", "cent" );
    LegendURLElem->SetAttribute ( "width", "cent" );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "LegendURL size non numeric",style == NULL );


    LegendURLElem->SetAttribute ( "height", "100" );
    LegendURLElem->SetAttribute ( "width", "100" );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "LegendURL size",style != NULL );

    LegendURLElem->SetAttribute ( "minScaleDenominator", "largeScale" );
    LegendURLElem->SetAttribute ( "maxScaleDenominator", "largeScale" );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "LegendURL scale non numeric",style == NULL );



    LegendURLElem->SetAttribute ( "minScaleDenominator", "100" );
    LegendURLElem->SetAttribute ( "maxScaleDenominator", "200" );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),true );
    CPPUNIT_ASSERT_MESSAGE ( "LegendURL scale",style != NULL );

    CPPUNIT_ASSERT_MESSAGE ( "Identifier Content",style->getId().compare ( "normal" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Title Content",style->getTitles().at ( 0 ).compare ( "normal" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Abstract Content",style->getAbstracts().at ( 0 ).compare ( "abstract" ) ==0 );
}

void CppUnitConfLoaderStyle::paletteStyle() {
    styleElem->LinkEndChild ( identifierElem );
    styleElem->LinkEndChild ( titleElem );
    styleElem->LinkEndChild ( abstractElem );
    styleElem->LinkEndChild ( keywordsElem );
    styleElem->LinkEndChild ( LegendURLElem );

    identifierElem->LinkEndChild ( new TiXmlText ( "normal" ) );
    titleElem->LinkEndChild ( new TiXmlText ( "normal" ) );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "No palette",style != NULL );


    TiXmlElement *paletteElem = new TiXmlElement ( "palette" );
    styleElem->LinkEndChild ( paletteElem );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "Empty palette",style == NULL );

    paletteElem->SetAttribute ( "maxValue","255" );

    TiXmlElement *colourElem = new TiXmlElement ( "colour" );
    paletteElem->LinkEndChild ( colourElem );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "Empty colour",style == NULL );

    TiXmlElement *redElem = new TiXmlElement ( "red" );
    colourElem->LinkEndChild ( redElem );
    TiXmlElement *greenElem = new TiXmlElement ( "green" );
    colourElem->LinkEndChild ( greenElem );
    TiXmlElement *blueElem = new TiXmlElement ( "blue" );
    colourElem->LinkEndChild ( blueElem );
    TiXmlElement *alphaElem = new TiXmlElement ( "alpha" );

    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "Empty colour component",style == NULL );

    redElem->LinkEndChild ( new TiXmlText ( "255" ) );
    greenElem->LinkEndChild ( new TiXmlText ( "0" ) );
    blueElem->LinkEndChild ( new TiXmlText ( "12" ) );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "Wrong colour component",style != NULL );


    paletteElem->SetAttribute ( "maxValue","-3" );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "Negative maxValue",style != NULL );

    paletteElem->SetAttribute ( "maxValue","255" );
    style = ConfLoader::parseStyle ( styleDoc,std::string(),false );
    CPPUNIT_ASSERT_MESSAGE ( "maxValue",style != NULL );


}



