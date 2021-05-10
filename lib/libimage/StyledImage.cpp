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

#include "StyledImage.h"

#include <boost/log/trivial.hpp>

int StyledImage::getline ( float* buffer, int line ) {
    //Styled image do not translate to float
    return origImage->getline ( buffer, line );
}

int StyledImage::getline ( uint16_t* buffer, int line ) {
    //Styled image do not translate to uint16_t
    return origImage->getline ( buffer, line );
}

int StyledImage::getline ( uint8_t* buffer, int line ) {
    if ( origImage->getChannels()==1 && palette->getColoursMap() && !palette->getColoursMap()->empty() ) {
        return _getline ( buffer, line );
    }

    return origImage->getline ( buffer, line );
}

StyledImage::StyledImage ( Image* image, int expectedChannels, Palette* palette ) : Image ( image->getWidth(), image->getHeight(), expectedChannels, image->getBbox() ), origImage ( image ), palette ( palette ) {
    if ( !this->palette->getColoursMap()->empty() ) {
        channels = expectedChannels;
    } else {
        channels = image->getChannels();
    }
}

StyledImage::~StyledImage() {
    delete origImage;
}


int StyledImage::_getline ( uint8_t* buffer, int line ) {
    float* source = new float[origImage->getWidth() *origImage->getChannels()];
    origImage->getline ( source, line );
    //TODO Optimize It
    int i = 0;
    switch ( channels ) {
    case 4:
        for ( ; i < origImage->getWidth() ; i++ ) {
            Colour iColour = palette->getColour ( * ( source+i ) );
            * ( buffer+i*4 ) = iColour.r;
            * ( buffer+i*4+1 ) = iColour.g;
            * ( buffer+i*4+2 ) = iColour.b;
            * ( buffer+i*4+3 ) = iColour.a;
        }
    case 3:
        for ( ; i < origImage->getWidth() ; i++ ) {
            Colour iColour = palette->getColour ( * ( source+i ) );
            * ( buffer+i*3 ) = iColour.r;
            * ( buffer+i*3+1 ) = iColour.g;
            * ( buffer+i*3+2 ) = iColour.b;
        }
    }

    delete[] source;
    return i*sizeof ( uint8_t ) *channels;

}

