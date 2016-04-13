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

#include "PenteImage.h"

#include "Logger.h"

#include "Utils.h"
#include <cstring>
#include <cmath>
#define DEG_TO_RAD      .0174532925199432958
#include <string>

 
using namespace std;
 


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
PenteImage::PenteImage ( Image* image,  float resolution, string algo) :
    Image ( image->getWidth(), image->getHeight(), 1, image->getBbox() ),
    origImage ( image ), pente ( NULL ), resolution (resolution), algo (algo)//, center ( center )
	{


	if (algo == "H"){
		//   matrice de la PenteImage
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

	else if (algo == NULL || algo =="Z"){//if no defined algo, algo = Z
		//   matrice de la PenteImage
		matrix[0] = 0 ;
		matrix[1] = -1 / (2.0*resolution) ;
		matrix[2] = 0 ;

		matrix[3] = -1 / (2.0*resolution) ;
		matrix[4] = 0 ;
		matrix[5] = 1 / (2.0*resolution) ;

		matrix[6] = 0 ;
		matrix[7] = 1 / (2.0*resolution) ;
		matrix[8] = 0 ;
	}

}

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
    pente = new uint8_t[origImage->getWidth() * origImage->getHeight()];
    bufferTmp = new float[origImage->getWidth() * 3];
    float* lineBuffer[3];
    lineBuffer[0]= bufferTmp;
    lineBuffer[1]= lineBuffer[0]+origImage->getWidth();
    lineBuffer[2]= lineBuffer[1]+origImage->getWidth();

    int line = 0;
    int nextBuffer = 0;
    getOrigLine ( lineBuffer[0], line );
    getOrigLine ( lineBuffer[1], line+1 );
    getOrigLine ( lineBuffer[2], line+2 );
    generateLine ( line++, lineBuffer[0],lineBuffer[0],lineBuffer[1] );
    generateLine ( line++, lineBuffer[0],lineBuffer[1],lineBuffer[2] );
    while ( line < origImage->getHeight() -1 ) {
        getOrigLine ( lineBuffer[nextBuffer], line+1 );
        generateLine ( line++, lineBuffer[ ( nextBuffer+1 ) %3],lineBuffer[ ( nextBuffer+2 ) %3],lineBuffer[nextBuffer] );
        nextBuffer = ( nextBuffer+1 ) %3;
    }
    generateLine ( line,lineBuffer[nextBuffer], lineBuffer[ ( nextBuffer+1 ) %3],lineBuffer[ ( nextBuffer+1 ) %3] );
    delete[] bufferTmp;
}

void PenteImage::generateLine ( int line, float* line1, float* line2, float* line3, std::string algo ) {
    uint8_t* currentLine =pente + line * width;
	//on commence a la premiere colonne
    int column = 1;
	//creation de la variable sur laquelle on travaille pour trouver le seuil
    double value;

		//calcul de la variable sur la premiere colonne
	if(algo=="H")
		{value = (matrix[2] * ( * ( line1+column+1 ) ) + matrix[5] * ( * ( line2+column+1 ) ) + matrix[8] * ( * ( line3+column+1 ) ) - matrix[0] * ( * ( line1+column ) ) - matrix[3] * ( * ( line2+column ) ) - matrix[6] * ( * ( line3+column ) ))^2
		+ (matrix[0] * ( * ( line1+column ) ) + matrix[1] * ( * ( line1+column ) ) + matrix[2] * ( * ( line1+column+1 ) ) - matrix[6] * ( * ( line3+column ) ) - matrix[7] * ( * ( line3+column ) ) - matrix[7] * ( * ( line3+column+1 ) ))^2;

		value = sqrt(value);}


	else if (algo == NULL || algo =="Z")
	{value = sqrt((matrix[1] * ( * ( line1+column ) ) - matrix[7] * ( * ( line3+column ) ))^2 + (matrix[3] * ( * ( line2+column ) ) - matrix[5] * ( * ( line2+column+1 ) ))^2);}

	value = atan(value) * 180 / PI;
	//verification valeur non superieure a 90
	if (value>90){value = 180-value;}

    *currentLine = ( int ) value;

	//calcul de la variable sur toutes les autres colonnes
    while ( column < width - 1 ) {
		if (algo == "H"){
			value = (matrix[2] * ( * ( line1+column+1 ) ) + matrix[5] * ( * ( line2+column+1 ) ) + matrix[8] * ( * ( line3+column+1 ) ) - matrix[0] * ( * ( line1+column-1 ) ) - matrix[3] * ( * ( line2+column-1 ) ) - matrix[6] * ( * ( line3+column-1 ) ))^2
			+ (matrix[0] * ( * ( line1+column-1 ) ) + matrix[1] * ( * ( line1+column ) ) + matrix[2] * ( * ( line1+column+1 ) ) - matrix[6] * ( * ( line3+column-1 ) ) - matrix[7] * ( * ( line3+column ) ) - matrix[7] * ( * ( line3+column+1 ) ))^2;

			value = sqrt(value);
		}
		else if (algo == NULL || algo =="Z")
			{value = sqrt((matrix[1] * ( * ( line1+column ) ) - matrix[7] * ( * ( line3+column ) ))^2 + (matrix[3] * ( * ( line2+column-1 ) ) - matrix[5] * ( * ( line2+column+1 ) ))^2);}

		value = atan(value) * 180 / PI;

        * ( currentLine+ ( column++ ) ) = ( int ) ( value );

    }


	//calcul de la variable sur la dernière colonne
		if (algo == "H")
			{value = (matrix[2] * ( * ( line1+column ) ) + matrix[5] * ( * ( line2+column ) ) + matrix[8] * ( * ( line3+column ) ) - matrix[0] * ( * ( line1+column-1 ) ) - matrix[3] * ( * ( line2+column-1 ) ) - matrix[6] * ( * ( line3+column-1 ) ))^2
			+ (matrix[0] * ( * ( line1+column-1 ) ) + matrix[1] * ( * ( line1+column ) ) + matrix[2] * ( * ( line1+column ) ) - matrix[6] * ( * ( line3+column-1 ) ) - matrix[7] * ( * ( line3+column ) ) - matrix[7] * ( * ( line3+column ) ))^2;

			value = sqrt(value);
			}
		else if (algo == NULL || algo =="Z")
			{value = sqrt((matrix[1] * ( * ( line1+column ) ) - matrix[7] * ( * ( line3+column ) ))^2 + (matrix[3] * ( * ( line2+column-1 ) ) - matrix[5] * ( * ( line2+column ) ))^2);}
		value = atan(value) * 180 / PI;
		//verification valeur non superieure a 90
		if (value>90){value = 180-value;}

    * ( currentLine+column ) = ( int ) ( value );
}
