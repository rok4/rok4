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

#ifndef _TIFFRAWENCODER_
#define _TIFFRAWENCODER_

#include "Data.h"
#include "Image.h"
#include "TiffHeader.h"
#include "TiffEncoder.h"

#include <cstring>

template <typename T>
class TiffRawEncoder : public TiffEncoder {
protected:
    virtual void prepareHeader(){
	LOGGER_DEBUG("TiffRawEncoder : préparation de l'en-tete");
	sizeHeader = TiffHeader::headerSize ( image->channels );
	header = new uint8_t[sizeHeader];
	if ( image->channels==1 )
	    if ( sizeof ( T ) == sizeof ( float ) ) {
		memcpy( header, TiffHeader::TIFF_HEADER_RAW_FLOAT32_GRAY, sizeHeader);
	    } else {
		memcpy( header, TiffHeader::TIFF_HEADER_RAW_INT8_GRAY, sizeHeader);
	    }
	else if ( image->channels==3 )
	    memcpy( header, TiffHeader::TIFF_HEADER_RAW_INT8_RGB, sizeHeader);
	else if ( image->channels==4 )
	    memcpy( header, TiffHeader::TIFF_HEADER_RAW_INT8_RGBA, sizeHeader);
	* ( ( uint32_t* ) ( header+18 ) )  = image->getWidth();
	* ( ( uint32_t* ) ( header+30 ) )  = image->getHeight();
	* ( ( uint32_t* ) ( header+102 ) ) = image->getHeight();
	* ( ( uint32_t* ) ( header+114 ) ) = tmpBufferSize ;
    }
  
    virtual void prepareBuffer(){
	LOGGER_DEBUG("TiffRawEncoder : préparation du buffer d'image");
	tmpBuffer = new uint8_t[image->getHeight()*image->getWidth()*image->channels*sizeof ( T )];
	int lRead = 0;
	tmpBufferSize = 0;
	int linesize = image->getWidth()*image->channels;
	int linesizetmp = linesize * sizeof ( T );
	for ( ; lRead < image->getHeight() ; lRead++ ) {
	    image->getline ( ( T* ) (tmpBuffer+tmpBufferSize), lRead );
	    tmpBufferSize+=linesizetmp;
	}
    }

public:
    TiffRawEncoder ( Image *image ) : TiffEncoder( image, -1 ) {}
    ~TiffRawEncoder() {
    }
   
};

#endif


