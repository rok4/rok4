#include <cppunit/extensions/HelperMacros.h>

#undef __SSE3__
#undef __SSE2__
#undef __SSE__
#include "Utils.h"
#include <sys/time.h>
#include <cstdlib>



#include <iostream>
using namespace std;


class CppUnitDotProdnoSSE : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitDotProdnoSSE );

  CPPUNIT_TEST( performance );  
  CPPUNIT_TEST( test_dot_prod );  
//  CPPUNIT_TEST( test_mult );  
  CPPUNIT_TEST_SUITE_END();


public:
  void setUp() {};

protected:


  double chrono_dp(int c, int k, float* to, const float* from, const float* W, int nb_iteration) {
    timeval BEGIN, NOW;
    gettimeofday(&BEGIN, NULL);
    for(int i = 0; i < nb_iteration; i++) dot_prod(c, k, to, from, W);
    gettimeofday(&NOW, NULL);
    double time = NOW.tv_sec - BEGIN.tv_sec + (NOW.tv_usec - BEGIN.tv_usec)/1000000.;
    return time;
  }
 

  void performance() {
    int nb_iteration = 10000000;
    int length = 1000;

    float from[2000]  __attribute__ ((aligned (32)));
    float to[2000]    __attribute__ ((aligned (32)));
    float W[128]      __attribute__ ((aligned (32)));
    memset(from, 0, sizeof(from));
    memset(W, 0, sizeof(W));

    cerr << " -= Dot Product no SSE=-" << endl;
    for(int k = 1; k <= 6; k++) 
      for(int c = 1; c <= 4; c++) {
        double t = chrono_dp(c, k, to, from, W, nb_iteration);
        cerr << t << "s : " << nb_iteration << " dot products K=" << k << " C=" << c << endl;
      }
    cerr << endl;
  }

  void test_dot_prod() {
    float from[2000]  __attribute__ ((aligned (32)));
    float to[2000]    __attribute__ ((aligned (32)));
    float W[128]      __attribute__ ((aligned (32)));

    for(int i = 0; i < 2000; i++) from[i] = i;
    for(int i = 0; i < 128; i++) W[i] = i/4;
    
    for(int k = 1; k <= 30; k++) 
      for(int c = 1; c <= 4; c++) {
        memset(to, 0, sizeof(to));
        dot_prod(c, k, to, from, W);

        for(int i = 0; i < 4*c; i++) {
          double p = 0;
          for(int j = 0; j < k; j++) p += from[4*j*c + i] * W[4*j + i%4];
          CPPUNIT_ASSERT_DOUBLES_EQUAL(p, to[i], 1e-5);
        }
        for(int i = 4*c; i < 2000; i++) CPPUNIT_ASSERT_EQUAL(0.F, to[i]);
      }
//      cerr << "Test Dot Product no SSE OK" << endl << endl;
  }



};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitDotProdnoSSE );

