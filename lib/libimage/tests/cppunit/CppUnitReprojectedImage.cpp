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

#include "ReprojectedImage.h"
#include "EmptyImage.h"
#include <sys/time.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

class CppUnitReprojectedImage : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitReprojectedImage );
    // enregistrement des methodes de tests à jouer :
//  CPPUNIT_TEST( testReproject );
    CPPUNIT_TEST ( performance );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() {};

protected:
    /*
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
    */

    string name ( int kernel_type ) {
        switch ( kernel_type ) {
        case Interpolation::UNKNOWN:
            return "(Default) Lanczos 3";
        case Interpolation::NEAREST_NEIGHBOUR:
            return "Nearest Neighbour";
        case Interpolation::LINEAR:
            return "Linear";
        case Interpolation::CUBIC:
            return "Cubic";
        case Interpolation::LANCZOS_2:
            return "Lanczos 2";
        case Interpolation::LANCZOS_3:
            return "Lanczos 3";
        case Interpolation::LANCZOS_4:
            return "Lanczos 4";
        }
    }

    void _chrono ( int channels, int kernel_type ) {
        int color[4];
        float buffer[800*4] __attribute__ ( ( aligned ( 32 ) ) );
        int nb_iteration = 2;

        timeval BEGIN, NOW;
        gettimeofday ( &BEGIN, NULL );

        for ( int i = 0; i < nb_iteration; i++ ) {
            BoundingBox<double> bbox ( 500000, 6500000, 501000, 6501000 );
            Grid* grid = new Grid ( 800, 600, bbox );
            grid->reproject ( "IGNF:LAMBE", "IGNF:LAMB93" );

            Image* image = new EmptyImage ( 1024, 768, channels, color );
            /*
                image->getbbox().xmin = grid->bbox.xmin - 100;
                image->getbbox().ymin = grid->bbox.ymin - 100;
                image->getbbox().xmax = grid->bbox.xmax + 100;
                image->getbbox().ymax = grid->bbox.ymax + 100;
            */
            image->setBbox ( BoundingBox<double> ( grid->bbox.xmin - 100, grid->bbox.ymin - 100, grid->bbox.xmax + 100, grid->bbox.ymax + 100 ) );

            grid->affine_transform ( 1./image->getResX(), -image->getBbox().xmin/image->getResX(),
                             -1./image->getResY(), image->getBbox().ymax/image->getResY() );

            ReprojectedImage* R = new ReprojectedImage ( image,  bbox, grid, Interpolation::KernelType ( kernel_type ) );
            for ( int l = 0; l < 600; l++ ) R->getline ( buffer, l );
            delete R;
        }
        gettimeofday ( &NOW, NULL );
        double time = NOW.tv_sec - BEGIN.tv_sec + ( NOW.tv_usec - BEGIN.tv_usec ) /1000000.;
        cerr << time << "s : " << nb_iteration << " reprojection 800x600, " << channels << " canaux, " << name ( kernel_type ) << endl;
    }

    void performance() {
        for ( int i = 1; i <= 4; i++ ) {
            // i=(i==2?++i:i);
            for ( int j = 0; j < 7; j++ )
                _chrono ( i, j );
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitReprojectedImage );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitReprojectedImage, "CppUnitReprojectedImage" );

