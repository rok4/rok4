#include <cppunit/extensions/HelperMacros.h>
#include "Utils.h"
#include <sys/time.h>
#include <cstdlib>

#include <iostream>
using namespace std;


class CppUnitMux : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitMux );

  CPPUNIT_TEST( performance );  
  CPPUNIT_TEST( test_multiplex );  
  CPPUNIT_TEST( test_demultiplex );  
  CPPUNIT_TEST_SUITE_END();


public:
  void setUp() {};

protected:

  void performance() {
    timeval BEGIN, NOW;
    int nb_iteration = 1000000;
    int length = 1000;

    float T1[2000]  __attribute__ ((aligned (32)));
    float T2[2000]  __attribute__ ((aligned (32)));
    float T3[2000]  __attribute__ ((aligned (32)));
    float T4[2000]  __attribute__ ((aligned (32)));
    float T[10000]  __attribute__ ((aligned (32)));

    cerr << " -= Multiplex =-" << endl;

    gettimeofday(&BEGIN, NULL);
    for(int i = 0; i < nb_iteration; i++) multiplex(T, T1, T2, T3, T4, length);
    gettimeofday(&NOW, NULL);
    double t = NOW.tv_sec - BEGIN.tv_sec + (NOW.tv_usec - BEGIN.tv_usec)/1000000.;
    cerr << t << "s : " << nb_iteration << " (x" << length << ") multiplex (aligned)" << endl;
    cerr << endl;

    cerr << " -= DeMultiplex =-" << endl;

    gettimeofday(&BEGIN, NULL);
    for(int i = 0; i < nb_iteration; i++) demultiplex(T1, T2, T3, T4, T, length);
    gettimeofday(&NOW, NULL);
    t = NOW.tv_sec - BEGIN.tv_sec + (NOW.tv_usec - BEGIN.tv_usec)/1000000.;
    cerr << t << "s : " << nb_iteration << " (x" << length << ") demultiplex (aligned)" << endl;
    cerr << endl;

  }

  void test_multiplex() {
    float T1[2000]  __attribute__ ((aligned (32)));
    float T2[2000]  __attribute__ ((aligned (32)));
    float T3[2000]  __attribute__ ((aligned (32)));
    float T4[2000]  __attribute__ ((aligned (32)));
    float T[10000]  __attribute__ ((aligned (32)));

    for(int i = 0; i < 2000; i++) {
      T1[i] = i;
      T2[i] = 10000 + i;
      T3[i] = 20000 + i;
      T4[i] = 30000 + i;
    }

    for(int k = 0; k < 2000; k++) {
      memset(T, 0, sizeof(T));
      int length = rand()%500;
      multiplex(T, T1, T2, T3, T4, length);

      for(int i = 0; i < length; i++) {
        CPPUNIT_ASSERT_EQUAL(T1[i], T[4*i]);
        CPPUNIT_ASSERT_EQUAL(T2[i], T[4*i+1]);
        CPPUNIT_ASSERT_EQUAL(T3[i], T[4*i+2]);
        CPPUNIT_ASSERT_EQUAL(T4[i], T[4*i+3]);
      }
      for(int i = 4*length; i < 10000; i++) CPPUNIT_ASSERT_EQUAL(0.F, T[i]);
    }
//    cerr << "Test Multiplex OK" << endl;
  }

  void test_demultiplex() {
    float T1[2000]  __attribute__ ((aligned (32)));
    float T2[2000]  __attribute__ ((aligned (32)));
    float T3[2000]  __attribute__ ((aligned (32)));
    float T4[2000]  __attribute__ ((aligned (32)));
    float T[10000]  __attribute__ ((aligned (32)));

    for(int i = 0; i < 10000; i++) T[i] = i;

    for(int k = 0; k < 1000; k++) {
      memset(T1, 0, sizeof(T1));
      memset(T2, 0, sizeof(T1));
      memset(T3, 0, sizeof(T1));
      memset(T4, 0, sizeof(T1));
      int length = rand()%500;
      demultiplex(T1, T2, T3, T4, T, length);

      for(int i = 0; i < length; i++) {
        CPPUNIT_ASSERT_EQUAL(T[4*i], T1[i]);
        CPPUNIT_ASSERT_EQUAL(T[4*i+1], T2[i]);
        CPPUNIT_ASSERT_EQUAL(T[4*i+2], T3[i]);
        CPPUNIT_ASSERT_EQUAL(T[4*i+3], T4[i]);
      }
      for(int i = length; i < 2000; i++) {
        CPPUNIT_ASSERT_EQUAL(0.F, T1[i]);
        CPPUNIT_ASSERT_EQUAL(0.F, T2[i]);
        CPPUNIT_ASSERT_EQUAL(0.F, T3[i]);
        CPPUNIT_ASSERT_EQUAL(0.F, T4[i]);
      }
    }
//    cerr << "Test DeMultiplex OK" << endl;
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitMux );

