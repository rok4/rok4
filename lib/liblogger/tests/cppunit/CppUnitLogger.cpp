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
#include "Logger.h"

//#define LOGGER(x) if(Logger::getAccumulator(x)) Logger::getLogger(x)


class CppUnitLogger : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitLogger );
    // enregistrement des methodes de tests à jouer :
    CPPUNIT_TEST ( test_logger );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() {};

protected:
    void test_logger() {
        std::stringstream out;
        Accumulator* acc = new StreamAccumulator ( out );
        Logger::setAccumulator ( DEBUG, acc );

        for ( int i = 0; i < 200; i++ ) LOGGER ( DEBUG ) << i << std::endl;
        //Logger::setAccumulator ( DEBUG, 0 );
        Logger::stopLogger();

        for ( int i = 0; i < 200; i++ ) {
            std::string s1, s2;
            int n;
            out >> s1 >> s2 >> n;
            CPPUNIT_ASSERT_EQUAL ( i, n );
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitLogger );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitLogger, "CppUnitLogger" );
