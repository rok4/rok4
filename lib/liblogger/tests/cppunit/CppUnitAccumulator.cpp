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
    A->stop();
    A->destroy();
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
    A->stop();
    A->destroy();
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
    A->stop();
    A->destroy();
    delete A;
  }




};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitAccumulator );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CppUnitAccumulator, "CppUnitAccumulator" );
