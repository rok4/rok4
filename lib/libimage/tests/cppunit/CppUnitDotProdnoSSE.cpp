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

#undef __SSE4__
#undef __SSE3__
#undef __SSE2__
#undef __SSE__
#include "Utils.h"
#include <sys/time.h>
#include <cstdlib>



#include <iostream>
using namespace std;


class CppUnitDotProdnoSSE : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitDotProdnoSSE );

    CPPUNIT_TEST ( performance );
    CPPUNIT_TEST ( test_dot_prod );
//  CPPUNIT_TEST( test_mult );
    CPPUNIT_TEST_SUITE_END();


public:
    void setUp() {};

protected:


    double chrono_dp ( int c, int k, float* to, const float* from, const float* W, int nb_iteration ) {
        timeval BEGIN, NOW;
        gettimeofday ( &BEGIN, NULL );
        for ( int i = 0; i < nb_iteration; i++ ) dot_prod ( c, k, to, from, W );
        gettimeofday ( &NOW, NULL );
        double time = NOW.tv_sec - BEGIN.tv_sec + ( NOW.tv_usec - BEGIN.tv_usec ) /1000000.;
//    time += to[0] * 1e-20;
        return time;
    }


    void performance() {
        int nb_iteration = 1000000;
        int length = 1000;

        float from[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float to[2000]    __attribute__ ( ( aligned ( 32 ) ) );
        float W[128]      __attribute__ ( ( aligned ( 32 ) ) );
        for ( int i = 0; i < 2000; i++ ) from[i] = i;
        for ( int i = 0; i < 128; i++ ) W[i] = i;

        cerr << " -= Dot Product no SSE=-" << endl;
        for ( int k = 1; k <= 6; k++ )
            for ( int c = 1; c <= 4; c++ ) {
                double t = chrono_dp ( c, k, to, from, W, nb_iteration );
                cerr << t << "s : " << nb_iteration << " dot products K=" << k << " C=" << c << endl;
            }
        cerr << endl;
    }

    void test_dot_prod() {
        float from[2000]  __attribute__ ( ( aligned ( 32 ) ) );
        float to[2000]    __attribute__ ( ( aligned ( 32 ) ) );
        float W[128]      __attribute__ ( ( aligned ( 32 ) ) );

        for ( int i = 0; i < 2000; i++ ) from[i] = i;
        for ( int i = 0; i < 128; i++ ) W[i] = i/4;

        for ( int k = 1; k <= 30; k++ )
            for ( int c = 1; c <= 4; c++ ) {
                memset ( to, 0, sizeof ( to ) );
                dot_prod ( c, k, to, from, W );

                for ( int i = 0; i < 4*c; i++ ) {
                    double p = 0;
                    for ( int j = 0; j < k; j++ ) p += from[4*j*c + i] * W[4*j + i%4];
                    CPPUNIT_ASSERT_DOUBLES_EQUAL ( p, to[i], 1e-5 );
                }
                for ( int i = 4*c; i < 2000; i++ ) CPPUNIT_ASSERT_EQUAL ( 0.F, to[i] );
            }
//      cerr << "Test Dot Product no SSE OK" << endl << endl;
    }



};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitDotProdnoSSE );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitDotProdnoSSE, "CppUnitDotProdnoSSE" );
