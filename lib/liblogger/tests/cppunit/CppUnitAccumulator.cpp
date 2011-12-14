#include <cppunit/extensions/HelperMacros.h>
#include "Accumulator.h"
#include <sys/time.h>
#include <sstream>
#include <map>

class CppUnitAccumulator : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitAccumulator );
  // enregistrement des methodes de tests à jouer :
  CPPUNIT_TEST( test_mono_thread );
  CPPUNIT_TEST( test_multi_thread );
  CPPUNIT_TEST( test_rollingfile );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() {};

protected:

  static void* fill_accumulator(void* arg) {
    Accumulator* A = (Accumulator*) arg;

    for(int i = 0; i < 100; i++) {
      std::stringstream S;
      S << i << " " << pthread_self() << std::endl;
      A->addMessage(S.str());
    }


  }



  void test_mono_thread() {
    std::stringstream out;
    Accumulator* A = new StreamAccumulator(out);
    fill_accumulator((void*) A);
    delete A;

    for(int i = 0; i < 100; i++) {
      int n;
      pthread_t id;
      out >> n >> id;
      CPPUNIT_ASSERT_EQUAL(i, n);
    }
  }


  void test_multi_thread() {
    std::stringstream out;
    Accumulator* A = new StreamAccumulator(out);

    pthread_t T[64];

    for(int i = 0; i < 64; i++) 
      pthread_create(&T[i], NULL, fill_accumulator, (void*) A);
    for(int i = 0; i < 64; i++) 
      pthread_join(T[i], 0);
    delete A;

    std::map<pthread_t, int> M;
    for(int i =  0; i < 64; i++) M[T[i]] == 0;

    // On vérifie que les 100 lignes de chaque thread ont bien été écrites dans le bon ordre.
    for(int i = 0; i < 64*100; i++) {
      int n;
      pthread_t id;
      out >> n >> id;
      CPPUNIT_ASSERT_EQUAL(n, M[id]);
      M[id]++;
    }
    for(int i =  0; i < 64; i++) CPPUNIT_ASSERT_EQUAL(M[T[i]], 100);
  }

  void test_rollingfile() {
    Accumulator* A = new RollingFileAccumulator("bubu",3600);
    fill_accumulator((void*) A);
    delete A;
  }




};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitAccumulator );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CppUnitAccumulator, "CppUnitAccumulator" );
