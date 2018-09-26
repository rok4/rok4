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

#include <string>

#include "CRS.h"
#include "TileMatrixSet.h"

class CppUnitTileMatrixSet : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE ( CppUnitTileMatrixSet );

    CPPUNIT_TEST ( constructors );
    CPPUNIT_TEST ( getters );

    CPPUNIT_TEST_SUITE_END();

protected:
    //TM
    double res;
    double x0;
    double y0;
    int tileW;
    int tileH;
    long int matrixW;
    long int matrixH;
    TileMatrix* tm1;
    TileMatrix* tm2;
    TileMatrix* tm3;
    //TMS
    std::string id;
    std::string title;
    std::string abstract;
    std::vector<Keyword> keyWords;
    std::vector<Keyword> keyWords2;
    CRS* crs;
    std::map<std::string,TileMatrix> tmList;
    std::map<std::string,TileMatrix> tmList2;
    std::map<std::string,TileMatrix> tmList3;

public:
    void setUp();
    void constructors();
    void getters();
    void tearDown();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitTileMatrixSet );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitTileMatrixSet, "CppUnitTileMatrixSet" );

void CppUnitTileMatrixSet::setUp() {
    //TM
    res = 209715.2;
    x0 = 0;
    y0 = 12000000;
    tileW = 256;
    tileH = 256;
    matrixW = 1;
    matrixH = 1;
    tm1 = new TileMatrix ( "1", res, x0, y0, tileW, tileH, matrixW, matrixH );
    tm2 = new TileMatrix ( "2", res, x0, y0, tileW, tileH, matrixW, matrixH );
    tm3 = new TileMatrix ( "3", res, x0, y0, tileW, tileH, matrixW, matrixH );
    //TMS
    id="LAMB93_10cm";
    title="Lambert93 10cm";
    abstract="En Lambert93 couche la plus basse résolution 10cm";
    keyWords.push_back ( Keyword ( "Lambert93",std::map<std::string,std::string>() ) );
    keyWords.push_back ( Keyword ( "10cm",std::map<std::string,std::string>() ) );
    keyWords2.push_back ( Keyword ( "Lambert93",std::map<std::string,std::string>() ) );
    crs = new CRS ( "IGNF:LAMB93" );
    tmList.insert ( std::pair<std::string, TileMatrix> ( tm1->getId(),*tm1 ) );
    tmList.insert ( std::pair<std::string, TileMatrix> ( tm2->getId(),*tm2 ) );
    tmList2.insert ( std::pair<std::string, TileMatrix> ( tm1->getId(),*tm1 ) );
    tmList3.insert ( std::pair<std::string, TileMatrix> ( tm2->getId(),*tm2 ) );
    tmList3.insert ( std::pair<std::string, TileMatrix> ( tm3->getId(),*tm3 ) );

}

void CppUnitTileMatrixSet::constructors() {

    TileMatrixSet* tms1 = new TileMatrixSet ( id, title, abstract, keyWords, *crs, tmList );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet Constructor",tms1 != NULL );

    TileMatrixSet* tms1Copy = new TileMatrixSet ( *tms1 );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet Copy Constructor",tms1Copy != NULL );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet Copy equality",*tms1Copy == *tms1 );

    TileMatrixSet* tms2 = new TileMatrixSet ( id, title, abstract, keyWords2, *crs, tmList2 );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet Constructor",tms2 != NULL );

    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet Simple Comparison",*tms2 != *tms1 );

    TileMatrixSet* tms3 = new TileMatrixSet ( id, title, abstract, keyWords, *crs, tmList3 );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet Constructor",tms3 != NULL );

    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet Simple Comparison",*tms3 == *tms1 );

    delete tms3;
    delete tms2;
    delete tms1Copy;
    delete tms1;
}
void CppUnitTileMatrixSet::getters() {
    TileMatrixSet* tms1 = new TileMatrixSet ( id, title, abstract, keyWords, *crs, tmList );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet Constructor",tms1 != NULL );

    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet getId",tms1->getId().compare ( id ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet getTitle",tms1->getTitle().compare ( title ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet getAbstract",tms1->getAbstract().compare ( abstract ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet getKeyWords",tms1->getKeyWords()->size() == keyWords.size() );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet getCrs",tms1->getCrs() == *crs );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrixSet getTmList",tms1->getTmList()->size() == tmList.size() );


    delete tms1;
}

void CppUnitTileMatrixSet::tearDown() {
    tmList3.clear();
    tmList2.clear();
    tmList.clear();
    delete crs;
    delete tm3;
    delete tm2;
    delete tm1;
}
