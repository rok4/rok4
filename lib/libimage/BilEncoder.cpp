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

#include <iostream>

#include "BilEncoder.h"
#include <boost/log/trivial.hpp>

size_t BilEncoder::read ( uint8_t *buffer, size_t size ) {
    size_t offset = 0;

    // Hypothese 1 : tous les bil produits sont en float
    int linesize = image->getWidth() * image->getChannels() * sizeof ( float );

    // Hypothese 2 : le pixel de l'image source est de type float
    float* buf_f=new float[image->getWidth() *image->getChannels()];
    for ( ; line < image->getHeight() && offset + linesize <= size; line++ ) {

        //	image->getline(buffer + offset, line);
        image->getline ( buf_f,line );

        // Hypothese 3 : image->getChannels()=1
        // On n'utilise pas la fonction convert qui caste un float en uint8_t
        // On copie simplement les octets des floats dans des uint8_t
        for ( int i=0; i<image->getWidth(); i++ ) {
            for ( int j=0; j<sizeof ( float ); j++ )
                buffer[offset+sizeof ( float ) *i+j]=* ( ( uint8_t* ) ( &buf_f[i] ) +j );
        }
        offset += linesize;
    }

    delete[] buf_f;

    return offset;
}

BilEncoder::~BilEncoder() {
    delete image;
}

bool BilEncoder::eof() {
    return line >= image->getHeight();
}
