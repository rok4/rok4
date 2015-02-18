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
#include "BoundingBox.h"

class CppUnitCRS : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE ( CppUnitCRS );

    CPPUNIT_TEST ( constructors );
    CPPUNIT_TEST ( getters );
    CPPUNIT_TEST ( setters );

    CPPUNIT_TEST_SUITE_END();

protected:
    std::string crs_code1;
    std::string crs_code2;
    std::string crs_code3;
    std::string crs_code4;
    std::string crs_code5;
    std::string crs_code6;

    CRS* crs1;
    CRS* crs2;
    CRS* crs3;
    CRS* crs4;
    CRS* crs5;
    CRS* crs6;

    BoundingBox<double> bbox1 = (0.,0.,0.,0.);
    BoundingBox<double> bbox2 = (0.,0.,0.,0.);
    BoundingBox<double> bbox3 = (0.,0.,0.,0.);
    BoundingBox<double> bbox4 = (0.,0.,0.,0.);
    BoundingBox<double> bbox5 = (0.,0.,0.,0.);
    BoundingBox<double> bbox6 = (0.,0.,0.,0.);

public:
    void setUp();
    void constructors();
    void getters();
    void setters();
    //TODO BoundingBox
    //TODO MetersPerunit
    void tearDown();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitCRS );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitCRS, "CppUnitCRS" );

void CppUnitCRS::setUp() {
    crs_code1="EPSG:4326";
    crs_code2="CRS:84";
    crs_code3="EPSG:3857";
    crs_code4="IGNF:WGS84G";
    crs_code5="epsg:3857";
    crs_code6="ESPG:3857";

    crs1 = new CRS ( crs_code1 );
    crs2 = new CRS ( crs_code2 );
    crs3 = new CRS ( crs_code3 );
    crs4 = new CRS ( crs_code4 );
    crs5 = new CRS ( crs_code5 );
    crs6 = new CRS ( crs_code6 );

    bbox1 = (-180.,-90.,180.,90.);
    bbox2 = (-180.,-85.,180.,85.);
    bbox3 = (-180.,-85.,180.,85.);
    bbox4 = (-180.,-90.,180.,90.);
    bbox5 = (-180.,-85.,180.,85.);
    bbox6 = (-180.,-85.,180.,85.);

}

void CppUnitCRS::constructors() {
    CRS crs1cpy ( *crs1 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crs1cpy == *crs1 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crs1cpy.cmpRequestCode ( crs1->getRequestCode() ) );

    CRS crsempty;
    CPPUNIT_ASSERT_MESSAGE ( "CRS default Constructor",crsempty.getProj4Code().compare ( "noProj4Code" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS default Constructor",crsempty.cmpRequestCode ( "" ) );
}
void CppUnitCRS::getters() {
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj4 compatible",crs1->isProj4Compatible() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj4 compatible",crs2->isProj4Compatible() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj4 compatible",crs3->isProj4Compatible() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj4 compatible",crs4->isProj4Compatible() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj4 compatible",crs5->isProj4Compatible() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS proj4 compatible",!crs6->isProj4Compatible() );

    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",crs1->isLongLat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",crs2->isLongLat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",!crs3->isLongLat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",crs4->isLongLat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",!crs5->isLongLat() );
    CPPUNIT_ASSERT_MESSAGE ( "CRS is LongLat",!crs6->isLongLat() );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getRequestCode",crs1->getRequestCode().compare ( crs_code1 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getRequestCode",crs2->getRequestCode().compare ( crs_code2 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getRequestCode",crs3->getRequestCode().compare ( crs_code3 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getRequestCode",crs4->getRequestCode().compare ( crs_code4 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getRequestCode",crs5->getRequestCode().compare ( crs_code5 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getRequestCode",crs6->getRequestCode().compare ( crs_code6 ) ==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getProj4Code",crs1->getProj4Code().compare ( "epsg:4326" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getProj4Code",crs2->getProj4Code().compare ( "epsg:4326" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getProj4Code",crs3->getProj4Code().compare ( crs_code5 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getProj4Code",crs4->getProj4Code().compare ( crs_code4 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getProj4Code",crs5->getProj4Code().compare ( crs_code5 ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getProj4Code",crs6->getProj4Code().compare ( "noProj4Code" ) ==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getAuthority",crs1->getAuthority().compare ( "EPSG" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getAuthority",crs2->getAuthority().compare ( "CRS" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getAuthority",crs3->getAuthority().compare ( "EPSG" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getAuthority",crs4->getAuthority().compare ( "IGNF" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getAuthority",crs5->getAuthority().compare ( "epsg" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getAuthority",crs6->getAuthority().compare ( "ESPG" ) ==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getIdentifier",crs1->getIdentifier().compare ( "4326" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getIdentifier",crs2->getIdentifier().compare ( "84" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getIdentifier",crs3->getIdentifier().compare ( "3857" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getIdentifier",crs4->getIdentifier().compare ( "WGS84G" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getIdentifier",crs5->getIdentifier().compare ( "3857" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getIdentifier",crs6->getIdentifier().compare ( "3857" ) ==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs1->getCrsDefinitionArea().xmin.compare(bbox1.xmin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs1->getCrsDefinitionArea().xmax.compare(bbox1.xmax)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs1->getCrsDefinitionArea().ymin.compare(bbox1.ymin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs1->getCrsDefinitionArea().ymax.compare(bbox1.ymax)==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs2->getCrsDefinitionArea().xmin.compare(bbox2.xmin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs2->getCrsDefinitionArea().xmax.compare(bbox2.xmax)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs2->getCrsDefinitionArea().ymin.compare(bbox2.ymin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs2->getCrsDefinitionArea().ymax.compare(bbox2.ymax)==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs3->getCrsDefinitionArea().xmin.compare(bbox3.xmin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs3->getCrsDefinitionArea().xmax.compare(bbox3.xmax)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs3->getCrsDefinitionArea().ymin.compare(bbox3.ymin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs3->getCrsDefinitionArea().ymax.compare(bbox3.ymax)==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs4->getCrsDefinitionArea().xmin.compare(bbox4.xmin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs4->getCrsDefinitionArea().xmax.compare(bbox4.xmax)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs4->getCrsDefinitionArea().ymin.compare(bbox4.ymin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs4->getCrsDefinitionArea().ymax.compare(bbox4.ymax)==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs5->getCrsDefinitionArea().xmin.compare(bbox5.xmin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs5->getCrsDefinitionArea().xmax.compare(bbox5.xmax)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs5->getCrsDefinitionArea().ymin.compare(bbox5.ymin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs5->getCrsDefinitionArea().ymax.compare(bbox5.ymax)==0 );

    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs6->getCrsDefinitionArea().xmin.compare(bbox6.xmin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs6->getCrsDefinitionArea().xmax.compare(bbox6.xmax)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs6->getCrsDefinitionArea().ymin.compare(bbox6.ymin)==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS getCRSDefinitionArea",crs6->getCrsDefinitionArea().ymax.compare(bbox6.ymax)==0 );

}

void CppUnitCRS::setters() {
    CRS crsempty;
    CPPUNIT_ASSERT_MESSAGE ( "CRS default Constructor",crsempty.getProj4Code().compare ( "noProj4Code" ) ==0 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS default Constructor",crsempty.cmpRequestCode ( "" ) );

    crsempty.setRequestCode ( crs_code1 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crsempty == *crs1 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crsempty.cmpRequestCode ( crs1->getRequestCode() ) );

    crsempty.setRequestCode ( crs_code2 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crsempty == *crs2 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crsempty.cmpRequestCode ( crs2->getRequestCode() ) );

    crsempty.setRequestCode ( crs_code3 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crsempty == *crs3 );
    CPPUNIT_ASSERT_MESSAGE ( "CRS Copy Constructor",crsempty.cmpRequestCode ( crs3->getRequestCode() ) );
}


void CppUnitCRS::tearDown() {
    delete crs1;
    delete crs2;
    delete crs3;
    delete crs4;
    delete crs5;
    delete crs6;
}
