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

#include "EstompageImage.h"

#include <boost/log/trivial.hpp>

#include "Utils.h"
#include <cstring>
#include <cmath>
#define DEG_TO_RAD      .0174532925199432958


int EstompageImage::getline ( float* buffer, int line ) {
    if ( !estompage ) {
        generate();
    }
    return _getline ( buffer, line );
}

int EstompageImage::getline ( uint16_t* buffer, int line ) {
    if ( !estompage ) {
        generate();
    }
    return _getline ( buffer, line );
}

int EstompageImage::getline ( uint8_t* buffer, int line ) {
    if ( !estompage ) {
        generate();
    }
    return _getline ( buffer, line );
}

EstompageImage::EstompageImage (int width, int height, int channels, BoundingBox<double> bbox, Image *image, float zenithDeg, float azimuthDeg, float zFactor , float resx, float resy) :
    Image ( width, height, channels, bbox ),
    origImage ( image ), estompage ( NULL ), zFactor (zFactor), resx (resx), resy (resy) {

    zenith = 90.0 - zenithDeg * DEG_TO_RAD;
    azimuth = (360.0 - azimuthDeg ) * DEG_TO_RAD;

}

EstompageImage::~EstompageImage() {
    delete origImage;
    if ( estompage ) delete[] estompage;
}


int EstompageImage::_getline ( float* buffer, int line ) {
    convert ( buffer, estompage + line * width, width );
    return width;
}

int EstompageImage::_getline ( uint16_t* buffer, int line ) {
    convert ( buffer, estompage + line * width, width );
    return width;
}

int EstompageImage::_getline ( uint8_t* buffer, int line ) {
    memcpy ( buffer, estompage + line * width, width );
    return width;
}


int EstompageImage::getOrigLine ( uint8_t* buffer, int line ) {
    return origImage->getline ( buffer,line );
}

int EstompageImage::getOrigLine ( uint16_t* buffer, int line ) {
    return origImage->getline ( buffer,line );
}

int EstompageImage::getOrigLine ( float* buffer, int line ) {
    return origImage->getline ( buffer,line );
}


void EstompageImage::generate() {
    estompage = new uint8_t[width * height];
    bufferTmp = new float[origImage->getWidth() * 3];
    float* lineBuffer[3];
    lineBuffer[0]= bufferTmp;
    lineBuffer[1]= lineBuffer[0]+origImage->getWidth();
    lineBuffer[2]= lineBuffer[1]+origImage->getWidth();

    int line = 0;
    int nextBuffer = 0;
    int lineOrig = 0;
    getOrigLine ( lineBuffer[0], lineOrig++ );
    getOrigLine ( lineBuffer[1], lineOrig++ );
    getOrigLine ( lineBuffer[2], lineOrig++ );
    generateLine ( line++, lineBuffer[0],lineBuffer[1],lineBuffer[2] );

    while ( line < height ) {
        getOrigLine ( lineBuffer[nextBuffer], lineOrig++ );
        generateLine ( line++, lineBuffer[ ( nextBuffer+1 ) %3],lineBuffer[ ( nextBuffer+2 ) %3],lineBuffer[nextBuffer] );
        nextBuffer = ( nextBuffer+1 ) %3;
    }
    delete[] bufferTmp;
}

void EstompageImage::generateLine ( int line, float* line1, float* line2, float* line3 ) {
    uint8_t* currentLine = estompage + line * width;
    int columnOrig = 1;
    int column = 0;
    double value;
    float dzdx,dzdy,slope,aspect;
    float a,b,c,d,e,f,g,h,i;

    while ( column < width ) {

        a = ( * ( line1+columnOrig-1 ) );
        b = ( * ( line1+columnOrig ) );
        c = ( * ( line1+columnOrig+1 ) );
        d = ( * ( line2+columnOrig-1 ) );
        e = ( * ( line2+columnOrig ) );
        f = ( * ( line2+columnOrig+1 ) );
        g = ( * ( line3+columnOrig-1 ) );
        h = ( * ( line3+columnOrig ) );
        i = ( * ( line3+columnOrig+1 ) );

        dzdx = ((c + 2*f + i) - (a + 2*d + g)) / (8 * resx);
        dzdy = ((g + 2*h + i) - (a + 2*b + c)) / (8 * resy);

        slope = atan(zFactor * sqrt(dzdx*dzdx+dzdy*dzdy));

        if (dzdx != 0) {
            aspect = atan2(dzdy,-dzdx);
            if (aspect < 0) {
                aspect = 2 * M_PI + aspect;
            } else {

            }
        } else {
            if (dzdy > 0) {
                aspect = M_PI_2;
            } else {
                aspect = 2 * M_PI - M_PI_2;
            }
        }

        value = 255.0 * ((cos(zenith) * cos(slope)) + (sin(zenith) * sin(slope) * cos(azimuth - aspect)));
        if (value<0) {value = 0;}

        * ( currentLine+ ( column++ ) ) = ( int ) ( value );
        columnOrig++;
    }

}
