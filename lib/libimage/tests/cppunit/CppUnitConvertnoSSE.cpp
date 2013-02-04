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

#undef __SSE4__
#undef __SSE3__
#undef __SSE2__
#undef __SSE__


#include "Utils.h"
#include <sys/time.h>
#include <cstdlib>

#include <iostream>
using namespace std;

template<typename T> inline const char* name();
template<> inline const char* name<uint8_t>() {return "uint8";}
template<> inline const char* name<float>() {return "float";}


class CppUnitConvertnoSSE : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitConvertnoSSE );
  // enregistrement des methodes de tests à jouer :
  CPPUNIT_TEST( uint8_to_float );
  CPPUNIT_TEST( performance );
  CPPUNIT_TEST_SUITE_END();


public:
  void setUp() {};

protected:


  template<typename T, typename F>
  double chrono(T* to, F* from, int length, int nb_iteration) {
    timeval BEGIN, NOW;
    gettimeofday(&BEGIN, NULL);
    for(int i = 0; i < nb_iteration; i++) convert(to, from, length);
    gettimeofday(&NOW, NULL);
    double time = NOW.tv_sec - BEGIN.tv_sec + (NOW.tv_usec - BEGIN.tv_usec)/1000000.;
    return time;
  }

  template<typename T, typename F>
  void performance() {
    int nb_iteration = 50000;
    int length = 1000;

    F from[4096]  __attribute__ ((aligned (32)));
    T to[4096]    __attribute__ ((aligned (32)));
    memset(from, 0, sizeof(from));

    cerr << " -= Conversion " << name<F>() << " -> " << name<T>() << " no SSE =-" << endl;

    double t = chrono(to, from, length, nb_iteration); 
    cerr << t << "s : " << nb_iteration << " (x" << length << ") " << name<F>() << " (aligned) -> " << name<T>() << " (aligned)" << endl;

    t = chrono(to, from+1, length, nb_iteration);
    cerr << t << "s : " << nb_iteration << " (x" << length << ") " << name<F>() << " (non aligned) -> " << name<T>() << " (aligned)" << endl;

    t = chrono(to+1, from, length, nb_iteration);
    cerr << t << "s : " << nb_iteration << " (x" << length << ") " << name<F>() << " (aligned) -> " << name<T>() << " (non aligned)" << endl;

    t = chrono(to+2, from+1, length, nb_iteration);
    cerr << t << "s : " << nb_iteration << " (x" << length << ") " << name<F>() << " (non aligned) -> " << name<T>() << " (non aligned)" << endl;
    cerr << endl;

  }

  void performance() {
    performance<uint8_t,float>();
    performance<float,uint8_t>();
    performance<uint8_t,uint8_t>();
  }


  void uint8_to_float() {
    double t = 0;
    
    uint8_t FROM8[32768];
    float  FLOAT[32768];
    uint8_t TO8[32768];
    for(int i = 0; i < 32768; i++) FROM8[i] = i;
    for(int i = 0; i < 100; i++) {
      int i1 = 4* (rand() % (16384/4));
      int i2 = 4* (rand() % (16384/4));
      int i3 = 4* (rand() % (16384/4));
      int l = 4* (rand() % (16384/4));
      memset(FLOAT, 0, sizeof(FLOAT));
      memset(TO8, 0, sizeof(TO8));
      
      convert(FLOAT + i2, FROM8 + i1, l);
      convert(TO8 + i3, FLOAT + i2, l);

      for(int i = 0; i < i2; i++) CPPUNIT_ASSERT_EQUAL(0.F, FLOAT[i]);
      for(int i = i2 + l; i < 32768; i++) CPPUNIT_ASSERT_EQUAL(0.F, FLOAT[i]);
      for(int i = 0; i < i3; i++) CPPUNIT_ASSERT_EQUAL(0, (int)TO8[i]);
      for(int i = i3 + l; i < 32768; i++) CPPUNIT_ASSERT_EQUAL(0, (int)TO8[i]);
      for(int i = 0; i < l; i++) CPPUNIT_ASSERT_EQUAL((int)FROM8[i1 + i], (int) FLOAT[i2+i]);
      for(int i = 0; i < l; i++) CPPUNIT_ASSERT_EQUAL((int)FROM8[i1 + i], (int)TO8[i3 + i]);
    }
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitConvertnoSSE );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CppUnitConvertnoSSE, "CppUnitConvertnoSSE" );
