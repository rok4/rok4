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


class CppUnitMult : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitMult );

    CPPUNIT_TEST ( performance );
    CPPUNIT_TEST ( test_add_mult );
    CPPUNIT_TEST ( test_mult );
    CPPUNIT_TEST_SUITE_END();


public:
    void setUp() {};

protected:


    double chrono_mult ( float* to, float* from, float w, int length, int nb_iteration ) {
        timeval BEGIN, NOW;
        gettimeofday ( &BEGIN, NULL );
        for ( int i = 0; i < nb_iteration; i++ ) mult ( to, from, w, length );
        gettimeofday ( &NOW, NULL );
        double time = NOW.tv_sec - BEGIN.tv_sec + ( NOW.tv_usec - BEGIN.tv_usec ) /1000000.;
        return time;
    }

    double chrono_add_mult ( float* to, float* from, float w, int length, int nb_iteration ) {
        timeval BEGIN, NOW;
        gettimeofday ( &BEGIN, NULL );
        for ( int i = 0; i < nb_iteration; i++ ) add_mult ( to, from, w, length );
        gettimeofday ( &NOW, NULL );
        double time = NOW.tv_sec - BEGIN.tv_sec + ( NOW.tv_usec - BEGIN.tv_usec ) /1000000.;
        return time;
    }


    void performance() {
        int nb_iteration = 50000;
        int length = 1000;

        float from[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float to[2000]    __attribute__ ( ( aligned ( 32 ) ) );
        float w = 1.23;
        memset ( from, 0, sizeof ( from ) );

        cerr << " -= Multiplications =-" << endl;
        for ( int d1 = 0; d1 < 4; d1++ )
            for ( int d2 = 0; d2 < 4; d2++ ) {
                double t = chrono_mult ( to + d1, from + d2, w, length, nb_iteration );
                cerr << t << "s : " << nb_iteration << " (x" << length << ") mult (aligned+" << d1 << ") -> (aligned+" << d2 << ")" << endl;
            }
        cerr << endl;

        cerr << " -= Addition-Multiplications =-" << endl;
        for ( int d1 = 0; d1 < 4; d1++ )
            for ( int d2 = 0; d2 < 4; d2++ ) {
                double t = chrono_add_mult ( to + d1, from + d2, w, length, nb_iteration );
                cerr << t << "s : " << nb_iteration << " (x" << length << ") add_mult (aligned+" << d1 << ") -> (aligned+" << d2 << ")" << endl;
            }
        cerr << endl;
    }

    void test_mult() {
        float from[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float to[2000]    __attribute__ ( ( aligned ( 32 ) ) );
        for ( int k = 0; k < 2000; k++ ) from[k] = k;

        for ( int k = 0; k < 2000; k++ ) {
            int i1 = rand() %100;
            int i2 = rand() %100;
            int length = rand() %500;
            float w = double ( rand() ) /double ( RAND_MAX );

            for ( int i = 0; i < 2000; i++ ) to[i] = i;
            mult ( to + i2, from + i1, w, length );
            for ( int i = 0; i < i2; i++ ) CPPUNIT_ASSERT_EQUAL ( float ( i ), to[i] );
            for ( int i = 0; i < length; i++ ) CPPUNIT_ASSERT_EQUAL ( float ( i+i1 ) *w, to[i+i2] );
            for ( int i = length+i2; i < 2000; i++ ) CPPUNIT_ASSERT_EQUAL ( float ( i ), to[i] );
        }
//    cerr << "Test mult OK" << endl;
    }

    void test_add_mult() {
        float from[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float to[2000]    __attribute__ ( ( aligned ( 32 ) ) );
        for ( int k = 0; k < 2000; k++ ) from[k] = float ( k );

        for ( int k = 0; k < 2000; k++ ) {
            int i1 = rand() %100;
            int i2 = rand() %100;
            int length = rand() %500;
            float w = double ( rand() ) /double ( RAND_MAX );

            for ( int i = 0; i < 2000; i++ ) to[i] = float ( i );
            add_mult ( to + i2, from + i1, w, length );
            for ( int i = 0; i < i2; i++ ) CPPUNIT_ASSERT_EQUAL ( float ( i ), to[i] );
            for ( int i = 0; i < length; i++ ) {
                CPPUNIT_ASSERT_DOUBLES_EQUAL ( float ( i2+i ) + float ( i1+i ) *w, to[i2+i], 1e-4 );
            }
            for ( int i = length+i2; i < 2000; i++ ) CPPUNIT_ASSERT_EQUAL ( float ( i ), to[i] );
        }
//    cerr << "Test add_mult OK" << endl;
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitMult );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitMult, "CppUnitMult" );
