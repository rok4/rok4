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
#include "Kernel.h"
#include "Interpolation.h"
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
      
      const Kernel &K = Kernel::getInstance(Interpolation::KernelType(i%6)); // Il y a 6 Noyaux défini dans KernelType


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

