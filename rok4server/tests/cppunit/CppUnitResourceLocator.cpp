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

#include "ResourceLocator.h"

class CppUnitResourceLocator : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE ( CppUnitResourceLocator );

    CPPUNIT_TEST ( constructors );
    CPPUNIT_TEST ( getters );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void constructors();
    void getters();
    void tearDown();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitResourceLocator );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitResourceLocator, "CppUnitResourceLocator" );

void CppUnitResourceLocator::setUp() {

}

void CppUnitResourceLocator::constructors() {
    ResourceLocator* rl1 = new ResourceLocator ( "text","http://ign.fr" );
    ResourceLocator rl2 ( "text","http://ign.fr" );
    ResourceLocator rl3 ( *rl1 );
    ResourceLocator rl4 ( "img","http://ign.fr" );
    ResourceLocator rl5 ( "text","http://www.ign.fr" );

    CPPUNIT_ASSERT_MESSAGE ( "Constructor", rl2 == *rl1 );
    CPPUNIT_ASSERT_MESSAGE ( "Copy Constructor", rl3 == *rl1 );
    CPPUNIT_ASSERT_MESSAGE ( "Format diff", rl2 != rl4 );
    CPPUNIT_ASSERT_MESSAGE ( "HRef diff", rl2 != rl5 );
    CPPUNIT_ASSERT_MESSAGE ( "All diff", rl5 != rl4 );

    delete rl1;
}
void CppUnitResourceLocator::getters() {
    ResourceLocator* rl1 = new ResourceLocator ( "text","http://ign.fr" );
    CPPUNIT_ASSERT_MESSAGE ( "GetFormat", !rl1->getFormat().compare ( "text" ) );
    CPPUNIT_ASSERT_MESSAGE ( "GetHRef", !rl1->getHRef().compare ( "http://ign.fr" ) );
    delete rl1;
}

void CppUnitResourceLocator::tearDown() {

}
