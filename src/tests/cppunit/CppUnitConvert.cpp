#include <cppunit/extensions/HelperMacros.h>
#include "Convert.h"
#include <sys/time.h>


#include <iostream>
#include <sstream>
using namespace std;

class CppUnitConvert : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitConvert );
  // enregistrement des methodes de tests à jouer :
  CPPUNIT_TEST( uint8_to_float );
  CPPUNIT_TEST( chrono );
  CPPUNIT_TEST_SUITE_END();


public:
  void setUp() {};

protected:


  template<typename T, typename F>
  void chrono() {
    int nb_iteration = 100000;
    F from[65536] ;
    T to[65536];
    memset(from, 0, sizeof(F));
    
    timeval BEGIN, NOW;
    gettimeofday(&BEGIN, NULL);
    for(int i = 0; i < nb_iteration; i++) convert(to, from, 65536);
    gettimeofday(&NOW, NULL);
    double time = NOW.tv_sec - BEGIN.tv_sec + (NOW.tv_usec - BEGIN.tv_usec)/1000000.;
    cerr << __PRETTY_FUNCTION__ << " : " << nb_iteration << " itérations en " << time << "s" << endl;

  }

  void chrono() {
    chrono<uint8_t,float>();
    chrono<float,uint8_t>();
    chrono<uint8_t,uint8_t>();
  }




  void uint8_to_float() {
    double t = 0;
    
    uint8_t FROM8[32768];
    float  FLOAT[32768];
    uint8_t TO8[32768];
    for(int i = 0; i < 32768; i++) FROM8[i] = i;
    for(int i = 0; i < 1000; i++) {
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

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitConvert );

