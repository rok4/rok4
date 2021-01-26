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
#include "MetadataURL.h"

class CppUnitMetadataURL : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE ( CppUnitMetadataURL );

    CPPUNIT_TEST ( constructors );
    CPPUNIT_TEST ( getters );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void constructors();
    void getters();
    void tearDown();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitMetadataURL );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitMetadataURL, "CppUnitMetadataURL" );

void CppUnitMetadataURL::setUp() {

}

void CppUnitMetadataURL::constructors() {
    MetadataURL* mu1 = new MetadataURL ( "text","http://ign.fr","isoap" );
    MetadataURL mu2 ( "text","http://ign.fr","isoap" );
    MetadataURL mu3 ( *mu1 );
    MetadataURL mu4 ( "img","http://ign.fr","isoap" );
    MetadataURL mu5 ( "text","http://www.ign.fr","isoap" );
    MetadataURL mu6 ( "img","http://www.ign.fr","inspire" );

    CPPUNIT_ASSERT_MESSAGE ( "Constructor", mu2 == *mu1 );
    CPPUNIT_ASSERT_MESSAGE ( "Copy Constructor", mu3 == *mu1 );
    CPPUNIT_ASSERT_MESSAGE ( "Format diff", mu2 != mu4 );
    CPPUNIT_ASSERT_MESSAGE ( "HRef diff", mu2 != mu5 );
    CPPUNIT_ASSERT_MESSAGE ( "All diff", mu2 != mu6 );

    delete mu1;
}
void CppUnitMetadataURL::getters() {
    MetadataURL* mu1= new MetadataURL ( "text","http://ign.fr","isoap" );
    CPPUNIT_ASSERT_MESSAGE ( "GetFormat", !mu1->getFormat().compare ( "text" ) );
    CPPUNIT_ASSERT_MESSAGE ( "GetHRef", !mu1->getHRef().compare ( "http://ign.fr" ) );
    CPPUNIT_ASSERT_MESSAGE ( "GetType", !mu1->getType().compare ( "isoap" ) );
    delete mu1;
}

void CppUnitMetadataURL::tearDown() {

}
