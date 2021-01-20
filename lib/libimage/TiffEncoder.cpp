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

#include "TiffEncoder.h"

#include "TiffRawEncoder.h"
#include "TiffLZWEncoder.h"
#include "TiffDeflateEncoder.h"
#include "TiffPackBitsEncoder.h"


TiffEncoder::TiffEncoder(Image *image, int line, bool isGeoTiff): image(image), line(line), isGeoTiff(isGeoTiff) {
    tmpBuffer = NULL;
    tmpBufferPos = 0;
    tmpBufferSize = 0;
    header = NULL;
    sizeHeader = 0;
}

TiffEncoder::TiffEncoder(Image *image, int line ): image(image), line(line), isGeoTiff(false) {
    tmpBuffer = NULL;
    tmpBufferPos = 0;
    tmpBufferSize = 0;
    header = NULL;
    sizeHeader = 0;
}

TiffEncoder::~TiffEncoder() {
    delete image;
    if ( tmpBuffer )
      delete[] tmpBuffer;
    if ( header )
      delete[] header;
}

DataStream* TiffEncoder::getTiffEncoder ( Image* image, Rok4Format::eformat_data format, bool isGeoTiff ) {
    switch ( format ) {
    case Rok4Format::TIFF_RAW_INT8 :
        return new TiffRawEncoder<uint8_t> ( image, isGeoTiff );
    case Rok4Format::TIFF_LZW_INT8 :
        return new TiffLZWEncoder<uint8_t> ( image, isGeoTiff );
    case Rok4Format::TIFF_ZIP_INT8 :
        return new TiffDeflateEncoder<uint8_t> ( image, isGeoTiff );
    case Rok4Format::TIFF_PKB_INT8 :
        return new TiffPackBitsEncoder<uint8_t> ( image, isGeoTiff );
    case Rok4Format::TIFF_RAW_FLOAT32 :
        return new TiffRawEncoder<float> ( image, isGeoTiff );
    case Rok4Format::TIFF_LZW_FLOAT32 :
        return new TiffLZWEncoder<float> ( image, isGeoTiff );
    case Rok4Format::TIFF_ZIP_FLOAT32 :
        return new TiffDeflateEncoder<float> ( image, isGeoTiff );
    case Rok4Format::TIFF_PKB_FLOAT32 :
        return new TiffPackBitsEncoder<float> ( image, isGeoTiff );
    default:
        return NULL;
    }
}

DataStream* TiffEncoder::getTiffEncoder ( Image* image, Rok4Format::eformat_data format ) {
    return getTiffEncoder( image, format, false );
}

size_t TiffEncoder::read(uint8_t* buffer, size_t size) {
    size_t offset = 0, dataToCopy=0;
    
    if ( !tmpBuffer ) {
        LOGGER_DEBUG("TiffEncoder : preparation du buffer d'image");
        prepareBuffer();
    }
    
    if ( !header ) {
        LOGGER_DEBUG("TiffEncoder : preparation de l'en-tete");
        prepareHeader();
        if ( isGeoTiff ){
            this->header = TiffHeader::insertGeoTags(image, this->header, &(this->sizeHeader) );
        }
    }
    
    // Si pas assez de place pour le header, ne rien écrire.
    if ( size < sizeHeader ) return 0;
      
    if ( line == -1 ) { // écrire le header tiff
	memcpy ( buffer, header, sizeHeader );
	offset = sizeHeader;
	line = 0;
    }

    if ( size - offset > 0 ) { // il reste de la place
	if ( tmpBufferPos <= tmpBufferSize ) { // il reste de la donnée
	    dataToCopy = std::min ( size-offset, tmpBufferSize - tmpBufferPos );
	    memcpy ( buffer+offset, tmpBuffer+tmpBufferPos, dataToCopy );
	    tmpBufferPos+=dataToCopy;
	    offset+=dataToCopy;
	}
    }

    return offset;
}

bool TiffEncoder::eof() {
    return ( tmpBufferPos>=tmpBufferSize );
}

unsigned int TiffEncoder::getLength(){
    if ( !tmpBuffer ) {
        LOGGER_DEBUG("TiffEncoder : preparation du buffer d'image");
        prepareBuffer();
    }
    
    if ( !header ) {
        LOGGER_DEBUG("TiffEncoder : preparation de l'en-tete");
        prepareHeader();
        if ( isGeoTiff ){
            this->header = TiffHeader::insertGeoTags(image, this->header, &(this->sizeHeader) );
        }
    }
    return sizeHeader + tmpBufferSize;
    
}

