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

#ifndef RAW_IMAGE_H
#define RAW_IMAGE_H

#include "Image.h"
#include "Data.h"
#include <cstring> // pour memcpy
#include "Logger.h"

class RawImage : public Image {

    DataSource *source;

public:
    /** Constructeur */
    RawImage ( int width, int height, int channels, DataSource* data_source ) : Image ( width, height, channels ) {
        // Verifier en amont que data_source n'est pas nul
        source=data_source;
    }

    virtual int getline ( uint8_t *buffer, int line ) {
        size_t size;
        const uint8_t* data=source->getData ( size );
        if ( !data ) {
            buffer=0;
            return 0;
        }
        memcpy ( buffer, ( uint8_t* ) &data[line*channels*width],width*channels*sizeof ( uint8_t ) );
        return width*channels*sizeof ( uint8_t );
    }
    virtual int getline ( float *buffer, int line ) {
        buffer = 0;
    }
    virtual int getline ( uint16_t *buffer, int line ) {
        buffer = 0;
    }

    virtual ~RawImage() {
        if (source) {
            delete source;
        }
    }
};

#endif
