#include <cppunit/extensions/HelperMacros.h>

#include "ResampledImage.h"
#include "EmptyImage.h"
#include <sys/time.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

class CppUnitResampledImage : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( CppUnitResampledImage );
  // enregistrement des methodes de tests à jouer :
  CPPUNIT_TEST( testResampled );
  CPPUNIT_TEST( performance );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() {};

protected:

  void testResampled() {

    uint8_t color[4];
    for(int i = 0; i < 20; i++) {
      int width = 200 + rand()%800;
      int height = 200 + rand()%800;
      int channels = 1 + rand()%4;
      for(int c = 0; c < channels; c++) color[c] = rand()%256;

      Image* image = new EmptyImage(width, height, channels, color);
      double top    = 50 + 50 * double(rand())/double(RAND_MAX);
      double left   = 50 + 50 * double(rand())/double(RAND_MAX);
      double right  = width - 50 - 50 * double(rand())/double(RAND_MAX);
      double bottom = height - 50 - 50 * double(rand())/double(RAND_MAX);

      int rwidth = int ((right - left) * (0.5 + double(rand())/double(RAND_MAX)));
      int rheight = int ((bottom - top) * (0.5 + double(rand())/double(RAND_MAX)));
      
//      cerr << width << " " << height << " " << channels << " " << rwidth << " " << rheight << " " << top << " " << left << " " << right << " " << bottom << endl;

      ResampledImage* R = new ResampledImage(image, rwidth, rheight, left, top, (right - left)/rwidth, (bottom - top)/rheight);
      float buffer[R->width*R->channels];
      for(int i = 0; i < rheight; i++) {
        R->getline(buffer, i);
        for(int j = 0; j < rwidth; j++) 
          for(int c = 0; c < channels; c++) {
            CPPUNIT_ASSERT_DOUBLES_EQUAL(color[c], buffer[j*channels + c], 1e-4);
          }
      }

      delete R;
    }
    cerr << "Test ResampledImage OK" << endl;
  }


  string name(int kernel_type) {
    switch(kernel_type) {
      case Kernel::NEAREST_NEIGHBOUR: return "Nearest Neighbour";
      case Kernel::LINEAR: return "Linear";
      case Kernel::CUBIC: return "Cubic";
      case Kernel::LANCZOS_2: return "Lanczos 2";
      case Kernel::LANCZOS_3: return "Lanczos 3";
      case Kernel::LANCZOS_4: return "Lanczos 4";
    }
  }

  void _chrono(int channels, int kernel_type) {
    uint8_t color[4];
    float buffer[800*4] __attribute__ ((aligned (32)));
    int nb_iteration = 50;

    timeval BEGIN, NOW;
    gettimeofday(&BEGIN, NULL);
    for(int i = 0; i < nb_iteration; i++) {
      Image* image = new EmptyImage(1300, 1000, channels, color);
      ResampledImage* R = new ResampledImage(image, 800, 600, 50, 50, 1.5, 1.5, Kernel::KernelType(kernel_type));
      for(int l = 0; l < 600; l++) R->getline(buffer, l);
      delete R;
    }
    gettimeofday(&NOW, NULL);
    double time = NOW.tv_sec - BEGIN.tv_sec + (NOW.tv_usec - BEGIN.tv_usec)/1000000.;
    cerr << time << "s : " << nb_iteration << " rééchantillonages 800x600, " << channels << " canaux, " << name(kernel_type) << endl;
  }

  void performance() {
    for(int i = 1; i <= 4; i++) 
      for(int j = 0; j < 6; j++) 
        _chrono(i, j);
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION( CppUnitResampledImage );


