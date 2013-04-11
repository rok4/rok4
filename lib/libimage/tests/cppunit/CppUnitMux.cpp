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
#include "Utils.h"
#include <sys/time.h>
#include <cstdlib>

#include <iostream>
using namespace std;


class CppUnitMux : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitMux );

    CPPUNIT_TEST ( performance );
    CPPUNIT_TEST ( test_multiplex );
    CPPUNIT_TEST ( test_demultiplex );
    CPPUNIT_TEST_SUITE_END();


public:
    void setUp() {};

protected:

    void performance() {
        timeval BEGIN, NOW;
        int nb_iteration = 10000;
        int length = 1000;

        float T1[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T2[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T3[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T4[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T[10000]  __attribute__ ( ( aligned ( 32 ) ) );

        cerr << " -= Multiplex =-" << endl;

        gettimeofday ( &BEGIN, NULL );
        for ( int i = 0; i < nb_iteration; i++ ) multiplex ( T, T1, T2, T3, T4, length );
        gettimeofday ( &NOW, NULL );
        double t = NOW.tv_sec - BEGIN.tv_sec + ( NOW.tv_usec - BEGIN.tv_usec ) /1000000.;
        cerr << t << "s : " << nb_iteration << " (x" << length << ") multiplex (aligned)" << endl;
        cerr << endl;

        cerr << " -= Multiplex unaligned =-" << endl;

        gettimeofday ( &BEGIN, NULL );
        for ( int i = 0; i < nb_iteration; i++ ) multiplex_unaligned ( T, T1, T2+1, T3+2, T4+3, length );
        gettimeofday ( &NOW, NULL );
        t = NOW.tv_sec - BEGIN.tv_sec + ( NOW.tv_usec - BEGIN.tv_usec ) /1000000.;
        cerr << t << "s : " << nb_iteration << " (x" << length << ") multiplex (aligned)" << endl;
        cerr << endl;

        cerr << " -= DeMultiplex =-" << endl;

        gettimeofday ( &BEGIN, NULL );
        for ( int i = 0; i < nb_iteration; i++ ) demultiplex ( T1, T2, T3, T4, T, length );
        gettimeofday ( &NOW, NULL );
        t = NOW.tv_sec - BEGIN.tv_sec + ( NOW.tv_usec - BEGIN.tv_usec ) /1000000.;
        cerr << t << "s : " << nb_iteration << " (x" << length << ") demultiplex (aligned)" << endl;
        cerr << endl;

    }

    void test_multiplex() {
        float T1[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T2[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T3[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T4[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T[10000]  __attribute__ ( ( aligned ( 32 ) ) );

        for ( int i = 0; i < 2000; i++ ) {
            T1[i] = i;
            T2[i] = 10000 + i;
            T3[i] = 20000 + i;
            T4[i] = 30000 + i;
        }

        for ( int k = 0; k < 2000; k++ ) {
            memset ( T, 0, sizeof ( T ) );
            int length = rand() %500;
            multiplex ( T, T1, T2, T3, T4, length );

            for ( int i = 0; i < length; i++ ) {
                CPPUNIT_ASSERT_EQUAL ( T1[i], T[4*i] );
                CPPUNIT_ASSERT_EQUAL ( T2[i], T[4*i+1] );
                CPPUNIT_ASSERT_EQUAL ( T3[i], T[4*i+2] );
                CPPUNIT_ASSERT_EQUAL ( T4[i], T[4*i+3] );
            }
            for ( int i = 4*length; i < 10000; i++ ) CPPUNIT_ASSERT_EQUAL ( 0.F, T[i] );
        }
//    cerr << "Test Multiplex OK" << endl;
    }

    void test_demultiplex() {
        float T1[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T2[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T3[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T4[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float T[10000]  __attribute__ ( ( aligned ( 32 ) ) );

        for ( int i = 0; i < 10000; i++ ) T[i] = i;

        for ( int k = 0; k < 1000; k++ ) {
            memset ( T1, 0, sizeof ( T1 ) );
            memset ( T2, 0, sizeof ( T1 ) );
            memset ( T3, 0, sizeof ( T1 ) );
            memset ( T4, 0, sizeof ( T1 ) );
            int length = rand() %500;
            demultiplex ( T1, T2, T3, T4, T, length );

            for ( int i = 0; i < length; i++ ) {
                CPPUNIT_ASSERT_EQUAL ( T[4*i], T1[i] );
                CPPUNIT_ASSERT_EQUAL ( T[4*i+1], T2[i] );
                CPPUNIT_ASSERT_EQUAL ( T[4*i+2], T3[i] );
                CPPUNIT_ASSERT_EQUAL ( T[4*i+3], T4[i] );
            }
            for ( int i = length; i < 2000; i++ ) {
                CPPUNIT_ASSERT_EQUAL ( 0.F, T1[i] );
                CPPUNIT_ASSERT_EQUAL ( 0.F, T2[i] );
                CPPUNIT_ASSERT_EQUAL ( 0.F, T3[i] );
                CPPUNIT_ASSERT_EQUAL ( 0.F, T4[i] );
            }
        }
//    cerr << "Test DeMultiplex OK" << endl;
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitMux );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitMux, "CppUnitMux" );
