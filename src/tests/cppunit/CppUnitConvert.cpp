#include <cppunit/extensions/HelperMacros.h>
#include "Utils.h"
#include <sys/time.h>
#include <cstdlib>

#include <iostream>
using namespace std;

template<typename T> inline const char* name();
template<> inline const char* name<uint8_t>() {return "uint8";}
template<> inline const char* name<float>() {return "float";}


class CppUnitConvert : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitConvert );
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
    int nb_iteration = 1000000;
    int length = 1000;

    F from[2048]  __attribute__ ((aligned (32)));
    T to[2048]    __attribute__ ((aligned (32)));
    memset(from, 0, sizeof(from));

    cerr << " -= Conversion " << name<F>() << " -> " << name<T>() << " =-" << endl;

    double t = chrono(to, from, length, nb_iteration); 
    cerr << t << "s : " << nb_iteration << " (x" << length << ") conversions " << name<F>() << " (aligned) -> " << name<T>() << " (aligned)" << endl;

    t = chrono(to, from+1, length, nb_iteration);
    cerr << t << "s : " << nb_iteration << " (x" << length << ") conversions " << name<F>() << " (non aligned) -> " << name<T>() << " (aligned)" << endl;

    t = chrono(to+1, from, length, nb_iteration);
    cerr << t << "s : " << nb_iteration << " (x" << length << ") conversions " << name<F>() << " (aligned) -> " << name<T>() << " (non aligned)" << endl;

    t = chrono(to+2, from+1, length, nb_iteration);
    cerr << t << "s : " << nb_iteration << " (x" << length << ") conversions " << name<F>() << " (non aligned) -> " << name<T>() << " (non aligned)" << endl;
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

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitConvert );

