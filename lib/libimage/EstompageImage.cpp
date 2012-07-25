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

#include "EstompageImage.h"

#include "Logger.h"

#include "Utils.h"
#include <cstring>
#include <cmath>
#define DEG_TO_RAD      .0174532925199432958


int EstompageImage::getline ( float* buffer, int line ) {
    if (!estompage) {
        generate();
    }
    return _getline( buffer, line );
}

int EstompageImage::getline ( uint8_t* buffer, int line ) {
    if (!estompage) {
        generate();
    }
    return _getline( buffer, line );
}

EstompageImage::EstompageImage ( Image* image, int angle, float exaggeration, uint8_t center) : 
                Image ( image->width, image->height, 1, image->getbbox() ),
                origImage ( image ), estompage(NULL), exaggeration(exaggeration), center(center) {
    
    //   Sun direction
    float angleRad = angle * DEG_TO_RAD;
    matrix[0] = sin(angleRad - M_PI_4);
    matrix[1] = sin(angleRad);
    matrix[2] = sin(angleRad + M_PI_4);
    
    matrix[3] = sin(angleRad - M_PI_2);
    matrix[4] =  0.   ;
    matrix[5] = sin(angleRad + M_PI_2);
    
    matrix[6] = sin(angleRad - M_PI_4 - M_PI_2);
    matrix[7] = sin(angleRad + M_PI);
    matrix[8] = sin(angleRad + M_PI_4 + M_PI_2);
    
    
    //Add Zenithal Light
    matrix[0] +=  (-1) ;
    matrix[1] +=  (-1) ;
    matrix[2] +=  (-1) ;
    
    matrix[3] +=  (-1) ;
    matrix[4] +=    8  ;
    matrix[5] +=  (-1) ;
    
    matrix[6] +=  (-1) ;
    matrix[7] +=  (-1) ;
    matrix[8] +=  (-1) ;
    

}

EstompageImage::~EstompageImage() {
    delete origImage;
    if (estompage) delete estompage;
}

int EstompageImage::_getline ( float* buffer, int line ) {
    convert(buffer, estompage + line * width, width);
    return width;
}

int EstompageImage::_getline ( uint8_t* buffer, int line ) {
    memcpy(buffer, estompage + line * width, width);
    return width;
}

void EstompageImage::generate() {
    estompage = new uint8_t[origImage->width * origImage->height];
    buffer = new float[origImage->width * 3];
    float* lineBuffer[3];
    lineBuffer[0]= buffer;
    lineBuffer[1]= lineBuffer[0]+origImage->width;
    lineBuffer[2]= lineBuffer[1]+origImage->width;
    
    int line = 0;
    int nextBuffer = 0;
    origImage->getline(lineBuffer[0], line);
    origImage->getline(lineBuffer[0], line+1);
    origImage->getline(lineBuffer[2], line+2);
    generateLine(line++, lineBuffer[0],lineBuffer[0],lineBuffer[1]);
    generateLine(line++, lineBuffer[0],lineBuffer[1],lineBuffer[2]);
    while (line < origImage->height -1) {
        origImage->getline(lineBuffer[nextBuffer], line+1);
        generateLine(line++, lineBuffer[(nextBuffer+1)%3],lineBuffer[(nextBuffer+2)%3],lineBuffer[nextBuffer]);
        nextBuffer = (nextBuffer+1)%3;
    }
    generateLine(line,lineBuffer[nextBuffer], lineBuffer[(nextBuffer+1)%3],lineBuffer[(nextBuffer+1)%3]);
    delete buffer;
}

void EstompageImage::generateLine ( int line, float* line1, float* line2, float* line3 ) {
    uint8_t* currentLine = estompage + line * width;
    int column = 1;
    double value;
    value = matrix[0] * (*line1) + matrix[1] * (*line1) + matrix[2] * (*(line1+1))
          + matrix[3] * (*line2) + matrix[4] * (*line2) + matrix[5] * (*(line2+1))
          + matrix[6] * (*line3) + matrix[7] * (*line3) + matrix[8] * (*(line3+1));
    value*=exaggeration;
    value+=center;
    if (value < 0 ) value = 0;
    if (value > 255 ) value = 255;
    *currentLine = (int) value;
    
    while (column < width - 1 ) {
        value = matrix[0] * (*(line1+column-1)) + matrix[1] * (*(line1+column)) + matrix[2] * (*(line1+column+1))
               + matrix[3] * (*(line2+column-1)) + matrix[4] * (*(line2+column)) + matrix[5] * (*(line2+column+1))
               + matrix[6] * (*(line3+column-1)) + matrix[7] * (*(line3+column)) + matrix[8] * (*(line3+column+1));
        value*=exaggeration;
        value+=center;
        if (value < 0 ) value = 0;
        if (value > 255 ) value = 255;
        *(currentLine+(column++)) = (int) (value);
    
    }
    value = matrix[0] * (*(line1+column-1)) + matrix[1] * (*(line1+column)) + matrix[2] * (*(line1+column))
          + matrix[3] * (*(line2+column-1)) + matrix[4] * (*(line2+column)) + matrix[5] * (*(line2+column))
          + matrix[6] * (*(line3+column-1)) + matrix[7] * (*(line3+column)) + matrix[8] * (*(line3+column));
    value*=exaggeration;
    value+=center;
    if (value < 0 ) value = 0;
    if (value > 255 ) value = 255;
    *(currentLine+column)=(int) (value);
}
