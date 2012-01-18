#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include "TileMatrix.h"

class CppUnitTileMatrix : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE( CppUnitTileMatrix );

    CPPUNIT_TEST( constructors );
    CPPUNIT_TEST( getters );

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
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CppUnitTileMatrix, "CppUnitTileMatrix" );

void CppUnitTileMatrix::setUp()
{
    id = "0";
    res = 209715.2;
    x0 = 0;
    y0 = 12000000;
    tileW = 256;
    tileH = 256;
    matrixW = 1;
    matrixH = 1;
}

void CppUnitTileMatrix::constructors()
{
    TileMatrix* tm = new TileMatrix(id, res, x0, y0, tileW, tileH, matrixW, matrixH);
    CPPUNIT_ASSERT_MESSAGE("TileMatrix Constructor",tm != NULL);
    
    TileMatrix* tmCopy = new TileMatrix(*tm);
    CPPUNIT_ASSERT_MESSAGE("TileMatrix Copy Constructor",tmCopy != NULL);
    
    CPPUNIT_ASSERT_MESSAGE("TileMatrix Identical", *tmCopy == *tm);
    
    delete tm;
    delete tmCopy;
}

void CppUnitTileMatrix::getters()
{
    TileMatrix* tm = new TileMatrix(id, res, x0, y0, tileW, tileH, matrixW, matrixH);
    CPPUNIT_ASSERT_MESSAGE("TileMatrix Constructor",tm != NULL);
    
    CPPUNIT_ASSERT_MESSAGE("TileMatrix getRes",tm->getRes()==res );
    CPPUNIT_ASSERT_MESSAGE("TileMatrix getX0",tm->getX0()==x0 );
    CPPUNIT_ASSERT_MESSAGE("TileMatrix getY0",tm->getY0()==y0 );
    CPPUNIT_ASSERT_MESSAGE("TileMatrix getTileW",tm->getTileW()==tileW );
    CPPUNIT_ASSERT_MESSAGE("TileMatrix getTileH",tm->getTileH()==tileH );
    CPPUNIT_ASSERT_MESSAGE("TileMatrix getMatrixH",tm->getMatrixH()==matrixH );
    CPPUNIT_ASSERT_MESSAGE("TileMatrix getMatrixW",tm->getMatrixW()==matrixW );
    CPPUNIT_ASSERT_MESSAGE("TileMatrix getId",tm->getId().compare(id)==0 );
    delete tm;
}

void CppUnitTileMatrix::tearDown()
{

}
