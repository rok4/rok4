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
#include <iostream>

#include <fstream>
#include <vector>
#include "CapabilitiesBuilder.cpp"



class CppUnitCapabilitiesBuilder : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitCapabilitiesBuilder );
    // enregistrement des methodes de tests à jouer :
    CPPUNIT_TEST ( testnumToStr );
    CPPUNIT_TEST ( testdoubleToStr );
    CPPUNIT_TEST_SUITE_END();

protected:


public:
    void setUp();

    void tearDown();

protected:
    void testnumToStr();
    void testdoubleToStr();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitCapabilitiesBuilder );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitCapabilitiesBuilder, "CppUnitCapabilitiesBuilder" );

void CppUnitCapabilitiesBuilder::setUp() {

}

void CppUnitCapabilitiesBuilder::testnumToStr() {
    CPPUNIT_ASSERT_MESSAGE ( "conversion numToStr :\n", numToStr ( 1 ) == "1" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion numToStr :\n", numToStr ( 10000 ) == "10000" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion numToStr :\n", numToStr ( -10000 ) == "-10000" ) ;
}

void CppUnitCapabilitiesBuilder::testdoubleToStr() {
    // Results of approximations depend on the processor (32-bit, 64-bit...)
    // std::cout << doubleToStr(100.001);
    CPPUNIT_ASSERT_MESSAGE ( "conversion doubleToStr :\n", doubleToStr ( 101e-02 ).erase ( 5, std::string::npos ) == "1.010" || doubleToStr ( 101e-02 ).erase ( 5, std::string::npos ) == "1.009" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion doubleToStr :\n", doubleToStr ( 1.001 ).erase ( 6, std::string::npos ) == "1.0010" || doubleToStr ( 1.001 ).erase ( 6, std::string::npos ) == "1.0009" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion doubleToStr :\n", doubleToStr ( 100.001 ).erase ( 8, std::string::npos ) == "100.0010" || doubleToStr ( 100.001 ).erase ( 7, std::string::npos ) == "100.0009" ) ;
}

void CppUnitCapabilitiesBuilder::tearDown() {

}
