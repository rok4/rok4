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

#include <cppunit/extensions/HelperMacros.h>

#include "Style.h"

#include <string>

#include <stdlib.h>
#include <time.h>


class CppUnitStyle : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE ( CppUnitStyle );

    CPPUNIT_TEST ( constructors );
    CPPUNIT_TEST ( getters );

    CPPUNIT_TEST_SUITE_END();

protected:
    //Empty element
    std::string id0;
    std::vector<std::string> titles0;
    std::vector<std::string> abstracts0;
    std::vector<Keyword> keywords0;
    std::vector<LegendURL> legendURLs0;

    //One element
    std::string id1;
    std::vector<std::string> titles1;
    std::vector<std::string> abstracts1;
    std::vector<Keyword> keywords1;
    LegendURL* legendURL1;
    std::vector<LegendURL> legendURLs1;

    // Two elements
    std::string id2;
    std::vector<std::string> titles2;
    std::vector<std::string> abstracts2;
    std::vector<Keyword> keywords2;
    LegendURL* legendURL2;
    LegendURL* legendURL3;
    std::vector<LegendURL> legendURLs2;

    //Emtpy Palette
    Palette* palette0;
    //Effective Palette
    std::map<double,Colour> colours;
    Palette* palette1;
    Pente pente;
    Aspect aspect;


    Style* style;

public:
    void setUp();
    void constructors();
    void getters();
    void tearDown();
};


CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitStyle );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitStyle, "CppUnitStyle" );

void CppUnitStyle::setUp() {
    id0 = "";

    id1 = "one";
    titles1.push_back ( "Title1" );
    abstracts1.push_back ( "Abstract1" );
    keywords1.push_back ( Keyword ( "Keyword1",std::map<std::string,std::string>() ) );
    legendURL1 = new LegendURL ( "image/jpeg","http://ign.fr",400,400,25000,100000 );
    legendURLs1.push_back ( *legendURL1 );

    id2 = "two";
    titles2.push_back ( "Title2" );
    titles2.push_back ( "Title3" );
    abstracts2.push_back ( "Abstract2" );
    abstracts2.push_back ( "Abstract3" );
    keywords2.push_back ( Keyword ( "Keyword2",std::map<std::string,std::string>() ) );
    keywords2.push_back ( Keyword ( "Keyword3",std::map<std::string,std::string>() ) );
    legendURL2 = new LegendURL ( "image/jpeg","http://ign.fr",400,400,25000,100000 );
    legendURL3 = new LegendURL ( "image/jpeg","http://ign.fr",200,200,50000,200000 );
    legendURLs2.push_back ( *legendURL2 );
    legendURLs2.push_back ( *legendURL3 );

    palette0 = new Palette();

    srand ( time ( NULL ) );
    for ( int i = 0 ; i < 255; ++i ) {
        colours.insert ( std::pair<double,Colour> ( i,Colour ( 256 * ( rand() / ( RAND_MAX +1.0 ) ),256 * ( rand() / ( RAND_MAX +1.0 ) ),256 * ( rand() / ( RAND_MAX +1.0 ) ),256 * ( rand() / ( RAND_MAX +1.0 ) ) ) ) );
    }
    palette1 = new Palette ( colours,false,false,false );
    palette1->buildPalettePNG();

}

void CppUnitStyle::constructors() {
    Style* style;
    style = new Style ( id0,titles0,abstracts0,keywords0,legendURLs0,*palette0,pente,aspect );
    CPPUNIT_ASSERT_MESSAGE ( "Style initialisation with empty element",style != NULL );

    delete style;
    style = NULL;

    style = new Style ( id1,titles1,abstracts1,keywords1,legendURLs1,*palette1,pente,aspect  );
    CPPUNIT_ASSERT_MESSAGE ( "Style initialisation with one element",style != NULL );

    delete style;
    style = NULL;

    style = new Style ( id2,titles2,abstracts2,keywords2,legendURLs2,*palette1,pente,aspect  );
    CPPUNIT_ASSERT_MESSAGE ( "Style initialisation with two elements",style != NULL );

    delete style;
    style = NULL;
}


void CppUnitStyle::getters() {
    style = new Style ( id0,titles0,abstracts0,keywords0,legendURLs0,*palette0,pente,aspect  );
    CPPUNIT_ASSERT_MESSAGE ( "Style initialisation with empty element",style != NULL );

    CPPUNIT_ASSERT_MESSAGE ( "Style with empty element getId",style->getId().compare ( id0 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with empty element getId",style->getTitles().size() ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with empty element getId",style->getAbstracts().size() ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with empty element getId",style->getKeywords()->size() ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with empty element getId",style->getLegendURLs().size() ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with empty element getId",style->getPalette()->getPalettePNGSize() ==0 );

    delete style;

    style = NULL;

    style = new Style ( id1,titles1,abstracts1,keywords1,legendURLs1,*palette1,pente,aspect  );
    CPPUNIT_ASSERT_MESSAGE ( "Style initialisation with one element",style != NULL );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getId",style->getId().compare ( id1 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getTitles",style->getTitles().size() ==1 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getTitles[0]",style->getTitles().at ( 0 ).compare ( "Title1" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getAbstracts",style->getAbstracts().size() ==1 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getAbstracts[0]",style->getAbstracts().at ( 0 ).compare ( "Abstract1" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getKeywords",style->getKeywords()->size() ==1 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getKeywords[0]",style->getKeywords()->at ( 0 ).getContent().compare ( "Keyword1" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getLegendURLs",style->getLegendURLs().size() ==1 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getLegendURLs[0]",style->getLegendURLs().at ( 0 ) ==*legendURL1 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getPalette",* ( style->getPalette() ) !=*palette0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with one element getPalette",* ( style->getPalette() ) ==*palette1 );
    delete style;
    style = NULL;

    style = new Style ( id2,titles2,abstracts2,keywords2,legendURLs2,*palette1,pente,aspect  );
    CPPUNIT_ASSERT_MESSAGE ( "Style initialisation with two elements",style != NULL );
    CPPUNIT_ASSERT_MESSAGE ( "Style initialisation with one element",style != NULL );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getId",style->getId().compare ( id2 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getTitles",style->getTitles().size() ==2 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getTitles[0]",style->getTitles().at ( 0 ).compare ( "Title2" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getTitles[0]",style->getTitles().at ( 1 ).compare ( "Title3" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getAbstracts",style->getAbstracts().size() ==2 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getAbstracts[0]",style->getAbstracts().at ( 0 ).compare ( "Abstract2" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getAbstracts[0]",style->getAbstracts().at ( 1 ).compare ( "Abstract3" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getKeywords",style->getKeywords()->size() ==2 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getKeywords[0]",style->getKeywords()->at ( 0 ).getContent().compare ( "Keyword2" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getKeywords[0]",style->getKeywords()->at ( 1 ).getContent().compare ( "Keyword3" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getLegendURLs",style->getLegendURLs().size() ==2 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getLegendURLs[0]",style->getLegendURLs().at ( 0 ) ==*legendURL2 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getLegendURLs[0]",style->getLegendURLs().at ( 1 ) ==*legendURL3 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getPalette",* ( style->getPalette() ) !=*palette0 );
    CPPUNIT_ASSERT_MESSAGE ( "Style with two elements getPalette",* ( style->getPalette() ) ==*palette1 );

    delete style;
    style = NULL;
}


void CppUnitStyle::tearDown() {
    legendURLs0.clear();
    legendURLs1.clear();
    legendURLs2.clear();

    delete legendURL1;
    delete legendURL2;
    delete legendURL3;
    delete palette0;
    delete palette1;
}

