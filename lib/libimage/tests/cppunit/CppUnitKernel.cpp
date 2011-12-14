#include <cppunit/extensions/HelperMacros.h>
#include "Kernel.h"
#include <sys/time.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

class CppUnitKernel : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitKernel );
  // enregistrement des methodes de tests à jouer :
  CPPUNIT_TEST( testKernel );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() {};

protected:

  void testKernel() {

    float W[100];
    for(int i = 0; i < 1000; i++) {    
      
      const Kernel &K = Kernel::getInstance(Kernel::KernelType(i%6)); // Il y a 6 Noyaux défini dans KernelType


      int l = 2 + rand() % 99;
      int l2 = l;
      double x = 100 * double(rand()) / double(RAND_MAX);
      double ratio = 10 * double(rand()) / double(RAND_MAX);      
      int xmin = K.weight(W, l, x, ratio);

      double sum = 0;
      for(int i = 0; i < l; i++) sum += W[i];
      int nb_min = ceil(x - xmin);
      int nb_max = l - nb_min;

/*
      cerr << sum - 1 << " " << l << " " << l2 << " " << x << " " << xmin << " " << ratio << " | " ;
      for(int i = 0; i < l; i++) cerr << W[i] << " ";
      cerr << abs(nb_min - nb_max) << endl;
*/

      CPPUNIT_ASSERT(l <= l2);                   // On vérifie qu'il y a moins de coeff que place dans le tableau W
      CPPUNIT_ASSERT(xmin < x+1);
      CPPUNIT_ASSERT(abs(nb_min - nb_max) <= 1); // On vérifie qu'il y a autant de coeff avant et apres x (à 1 près)
      CPPUNIT_ASSERT_DOUBLES_EQUAL(1., sum, 1e-6);
    }
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitKernel );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CppUnitKernel, "CppUnitKernel" );

