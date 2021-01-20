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
#include "TileMatrix.h"

class CppUnitTileMatrix : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE ( CppUnitTileMatrix );

    CPPUNIT_TEST ( constructors );
    CPPUNIT_TEST ( getters );

    CPPUNIT_TEST_SUITE_END();

protected:
    std::string id;
    double res;
    double x0;
    double y0;
    int tileW;
    int tileH;
    long int matrixW;
    long int matrixH;
public:
    void setUp();
    void constructors();
    void getters();
    void tearDown();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitTileMatrix );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitTileMatrix, "CppUnitTileMatrix" );

void CppUnitTileMatrix::setUp() {
    id = "0";
    res = 209715.2;
    x0 = 0;
    y0 = 12000000;
    tileW = 256;
    tileH = 256;
    matrixW = 1;
    matrixH = 1;
}

void CppUnitTileMatrix::constructors() {
    TileMatrix* tm = new TileMatrix ( id, res, x0, y0, tileW, tileH, matrixW, matrixH );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix Constructor",tm != NULL );

    TileMatrix* tmCopy = new TileMatrix ( *tm );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix Copy Constructor",tmCopy != NULL );

    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix Identical", *tmCopy == *tm );

    delete tm;
    delete tmCopy;
}

void CppUnitTileMatrix::getters() {
    TileMatrix* tm = new TileMatrix ( id, res, x0, y0, tileW, tileH, matrixW, matrixH );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix Constructor",tm != NULL );

    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix getRes",tm->getRes() ==res );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix getX0",tm->getX0() ==x0 );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix getY0",tm->getY0() ==y0 );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix getTileW",tm->getTileW() ==tileW );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix getTileH",tm->getTileH() ==tileH );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix getMatrixH",tm->getMatrixH() ==matrixH );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix getMatrixW",tm->getMatrixW() ==matrixW );
    CPPUNIT_ASSERT_MESSAGE ( "TileMatrix getId",tm->getId().compare ( id ) ==0 );
    delete tm;
}

void CppUnitTileMatrix::tearDown() {

}
