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

#include "PenteImage.h"

#include "Logger.h"

#include "Utils.h"
#include <cstring>
#include <cmath>
#define DEG_TO_RAD      .0174532925199432958
#include <string>
 


int PenteImage::getline ( float* buffer, int line ) {
    if ( !pente ) {
        generate();
    }
    return _getline ( buffer, line );
}

int PenteImage::getline ( uint16_t* buffer, int line ) {
    if ( !pente ) {
        generate();
    }
    return _getline ( buffer, line );
}

int PenteImage::getline ( uint8_t* buffer, int line ) {
    if ( !pente ) {
        generate();
    }
    return _getline ( buffer, line );
}

//definition des variables
PenteImage::PenteImage (int width, int height, int channels, BoundingBox<double> bbox, Image* image, float resolutionx, float resolutiony, std::string algo, std::string unit, int slopend, float imgnd, int mxSlope) :
    Image ( width, height, channels, bbox ),
    origImage ( image ), pente ( NULL ), resolutionX (resolutionx), resolutionY (resolutiony),algo (algo),unit (unit), slopeNoData (slopend), imgNoData (imgnd), maxSlope (mxSlope)
    {}


PenteImage::~PenteImage() {
    delete origImage;
    if ( pente ) delete[] pente;
}


int PenteImage::_getline ( float* buffer, int line ) {
    convert ( buffer, pente + line * width, width );
    return width;
}

int PenteImage::_getline ( uint16_t* buffer, int line ) {
    convert ( buffer, pente + line * width, width );
    return width;
}

int PenteImage::_getline ( uint8_t* buffer, int line ) {
    memcpy ( buffer, pente + line * width, width );
    return width;
}


int PenteImage::getOrigLine ( uint8_t* buffer, int line ) {
    return origImage->getline ( buffer,line );
}

int PenteImage::getOrigLine ( uint16_t* buffer, int line ) {
    return origImage->getline ( buffer,line );
}

int PenteImage::getOrigLine ( float* buffer, int line ) {
    return origImage->getline ( buffer,line );
}


void PenteImage::generate() {
    pente = new uint8_t[width * height];
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

void PenteImage::generateLine ( int line, float* line1, float* line2, float* line3) {
    uint8_t* currentLine =pente + line * width;
	//on commence a la premiere colonne
    int columnOrig = 1;
    int column = 0;
	//creation de la variable sur laquelle on travaille pour trouver le seuil
    double dzdx,dzdy,rise,slope;
    float a,b,c,d,e,f,g,h,i;
    float resx,resy;

    if (algo == "H") {
        resx = 8.0 * resolutionX;
        resy = 8.0 * resolutionY;
    } else if (algo == "Z") {
        resx = 2.0 * resolutionX;
        resy = 2.0 * resolutionY;
    } else {

    }


	//calcul de la variable sur toutes les autres colonnes
    while ( column < width  ) {

        a = ( * ( line1+columnOrig-1 ) );
        b = ( * ( line1+columnOrig ) );
        c = ( * ( line1+columnOrig+1 ) );
        d = ( * ( line2+columnOrig-1 ) );
        e = ( * ( line2+columnOrig ) );
        f = ( * ( line2+columnOrig+1 ) );
        g = ( * ( line3+columnOrig-1 ) );
        h = ( * ( line3+columnOrig ) );
        i = ( * ( line3+columnOrig+1 ) );

        if (a == imgNoData || b == imgNoData || c == imgNoData || d == imgNoData || e == imgNoData ||
                f == imgNoData || g == imgNoData || h == imgNoData || i == imgNoData) {
            slope = slopeNoData;
        } else {

            if (algo == "H") {
                dzdx = (( c + 2.0 * f + i) - (a + 2.0 *  d + g)) / resx;
                dzdy = (( g + 2.0 * h + i) - (a + 2.0 *  b + c)) / resy;
            } else if (algo == "Z" ) {
                dzdx = (f - d) / resx;
                dzdy = (h - b) / resy;
            } else {

            }


            if (unit == "pourcent") {
                slope = sqrt(pow(dzdx,2.0) + pow(dzdy,2.0)) * 100.0;
            } else if (unit == "degree") {
                rise = sqrt(pow(dzdx,2.0) + pow(dzdy,2.0));

                slope = atan(rise) * 180.0 / M_PI;
                if (slope>90.0){slope = 180.0-slope;}
            } else {
                slope = 0;
            }

            if (slope>maxSlope){slope = maxSlope;}

        }

        * ( currentLine+ ( column++ ) ) = ( int ) ( slope );
        columnOrig++;

    }

}
