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

#include "AspectImage.h"

#include "Logger.h"

#include "Utils.h"
#include <cstring>
#include <cmath>
#define DEG_TO_RAD      .0174532925199432958
#include <string>

int AspectImage::getline ( float* buffer, int line ) {
    if ( !aspect ) {
        generate();
    }
    return _getline ( buffer, line );
}

int AspectImage::getline ( uint16_t* buffer, int line ) {
    if ( !aspect ) {
        generate();
    }
    return _getline ( buffer, line );
}

int AspectImage::getline ( uint8_t* buffer, int line ) {
    if ( !aspect ) {
        generate();
    }
    return _getline ( buffer, line );
}

//definition des variables
AspectImage::AspectImage (int width, int height, int channels, BoundingBox<double> bbox, Image* image,  float resolution, std::string algo, float minSlope) :
    Image ( width, height, channels, bbox ),
    origImage ( image ), aspect ( NULL ), resolution (resolution), algo (algo), minSlope (minSlope)
    {

    matrix[0] = 1 / (8.0*resolution) ;
    matrix[1] = 2 / (8.0*resolution) ;
    matrix[2] = 1 / (8.0*resolution) ;

    matrix[3] = 2 / (8.0*resolution) ;
    matrix[4] = 0 ;
    matrix[5] = 2 / (8.0*resolution) ;

    matrix[6] = 1 / (8.0*resolution) ;
    matrix[7] = 2 / (8.0*resolution) ;
    matrix[8] = 1 / (8.0*resolution) ;


}

AspectImage::~AspectImage() {
    delete origImage;
    if ( aspect ) delete[] aspect;
}


int AspectImage::_getline ( float* buffer, int line ) {
    convert ( buffer, aspect + line * width, width );
    return width;
}

int AspectImage::_getline ( uint16_t* buffer, int line ) {
    convert ( buffer, aspect + line * width, width );
    return width;
}

int AspectImage::_getline ( uint8_t* buffer, int line ) {
    memcpy ( buffer, aspect + line * width, width );
    return width;
}


int AspectImage::getOrigLine ( uint8_t* buffer, int line ) {
    return origImage->getline ( buffer,line );
}

int AspectImage::getOrigLine ( uint16_t* buffer, int line ) {
    return origImage->getline ( buffer,line );
}

int AspectImage::getOrigLine ( float* buffer, int line ) {
    return origImage->getline ( buffer,line );
}


void AspectImage::generate() {
    aspect = new float[width * height];
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

void AspectImage::generateLine ( int line, float* line1, float* line2, float* line3) {
    float* currentLine =aspect + line * width;
    //on commence a la premiere colonne
    int columnOrig = 1;
    int column = 0;
    //creation de la variable sur laquelle on travaille pour trouver le seuil
    double value,value1,value2,slope;

    //calcul de la variable sur toutes les autres colonnes
    while ( column < width ) {

        value1 = (matrix[2] * ( * ( line1+columnOrig+1 ) ) + matrix[5] * ( * ( line2+columnOrig+1 ) ) + matrix[8] * ( * ( line3+columnOrig+1 ) ) - matrix[0] * ( * ( line1+columnOrig-1 ) ) - matrix[3] * ( * ( line2+columnOrig-1 ) ) - matrix[6] * ( * ( line3+columnOrig-1 ) ));
        value2 = (matrix[0] * ( * ( line1+columnOrig-1 ) ) + matrix[1] * ( * ( line1+columnOrig ) ) + matrix[2] * ( * ( line1+columnOrig+1 ) ) - matrix[6] * ( * ( line3+columnOrig-1 ) ) - matrix[7] * ( * ( line3+columnOrig ) ) - matrix[8] * ( * ( line3+columnOrig+1 ) ));

        //calcul de la pente pour ne pas afficher l'exposition en dessous d'une certaine valeur de pente
        slope = sqrt(pow(value1,2.0)+pow(value2,2.0));
        if (slope < minSlope) {
            value = -1.0;
        } else {
            value = (atan2(value1,value2) + M_PI) * 180 / M_PI;
        }

        * ( currentLine+ ( column++ ) ) = ( value );
        columnOrig++;

    }

}


