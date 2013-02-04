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

#include "LegendURL.h"

class CppUnitLegendURL : public CPPUNIT_NS::TestFixture {
    
    CPPUNIT_TEST_SUITE( CppUnitLegendURL );

    CPPUNIT_TEST( constructors );
    CPPUNIT_TEST( getters );

    CPPUNIT_TEST_SUITE_END();
    
public:
    void setUp();
    void constructors();
    void getters();
    void tearDown();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitLegendURL );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CppUnitLegendURL, "CppUnitLegendURL" );

void CppUnitLegendURL::setUp()
{

}

void CppUnitLegendURL::constructors()
{
    LegendURL* lu1 = new LegendURL("text","http://ign.fr",200,300,250000.0,500.0);
    LegendURL lu2("text","http://ign.fr",200,300,250000.0,500.0);
    LegendURL lu3(*lu1);
    LegendURL lu4("text","http://ign.fr",100,300,250000.0,500.0);
    LegendURL lu5("text","http://ign.fr",200,400,250000.0,500.0);
    LegendURL lu6("text","http://ign.fr",200,300,200000.0,500.0);
    LegendURL lu7("text","http://ign.fr",200,300,200000.0,600.0);
    
    CPPUNIT_ASSERT_MESSAGE ( "Constructor", lu2 == *lu1 );
    CPPUNIT_ASSERT_MESSAGE ( "Copy Constructor", lu3 == *lu1 );
    CPPUNIT_ASSERT_MESSAGE ( "Width diff", lu2 != lu4 );
    CPPUNIT_ASSERT_MESSAGE ( "Height diff", lu2 != lu5 );
    CPPUNIT_ASSERT_MESSAGE ( "MinScale diff", lu2 != lu6 );
    CPPUNIT_ASSERT_MESSAGE ( "MaxScale diff", lu2 != lu7 );
    
    delete lu1;
}
void CppUnitLegendURL::getters()
{
    LegendURL* lu1= new LegendURL("text","http://ign.fr",200,300,250000.0,500.0);
    CPPUNIT_ASSERT_MESSAGE ("GetFormat", !lu1->getFormat().compare("text"));
    CPPUNIT_ASSERT_MESSAGE ("GetHRef", !lu1->getHRef().compare("http://ign.fr"));
    CPPUNIT_ASSERT_MESSAGE ("GetWidth", lu1->getWidth()==200);
    CPPUNIT_ASSERT_MESSAGE ("GetHeight", lu1->getHeight()==300);
    CPPUNIT_ASSERT_MESSAGE ("GetMinScale", lu1->getMinScaleDenominator()==250000.0);
    CPPUNIT_ASSERT_MESSAGE ("GetMaxScale", lu1->getMaxScaleDenominator()==500.0);
    delete lu1;
}

void CppUnitLegendURL::tearDown()
{

}
