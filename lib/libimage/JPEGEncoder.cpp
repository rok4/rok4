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

#include "JPEGEncoder.h"
#include <assert.h>
#include <cmath>

/** Constructeur */
JPEGEncoder::JPEGEncoder ( Image* image ) : image ( image ), status ( -1 ) {
    cinfo.err = jpeg_std_error ( &jerr );
    jpeg_create_compress ( &cinfo );
    cinfo.dest = new jpeg_destination_mgr;

    cinfo.dest->init_destination = init_destination;
    cinfo.dest->empty_output_buffer = empty_output_buffer;
    cinfo.dest->term_destination = term_destination;
    cinfo.dest->next_output_byte = 0;
    cinfo.dest->free_in_buffer = 0;

    cinfo.image_width = image->getWidth();
    cinfo.image_height = image->getHeight();
    cinfo.input_components = image->getChannels();
    if ( image->getChannels() == 3 ) cinfo.in_color_space = JCS_RGB;
    else if ( image->getChannels() == 1 ) cinfo.in_color_space = JCS_GRAYSCALE;
    else if ( image->getChannels() == 4 ) cinfo.in_color_space = JCS_EXT_RGBX;
    else cinfo.in_color_space = JCS_UNKNOWN;


    jpeg_set_defaults ( &cinfo );

    bufferLimit = std::max ( 1024, ( ( image->getWidth() * image->getChannels() ) / 2 ) );

    linebuffer = new uint8_t[image->getWidth() *image->getChannels()];
}

/**
* Lecture du flux JPEG
*/

size_t JPEGEncoder::read ( uint8_t *buffer, size_t size ) {
    if ( size < 1024 ) {
        return 0;
    }

    // On initialise le buffer d'écriture de la libjpeg
    cinfo.dest->next_output_byte = buffer;
    cinfo.dest->free_in_buffer = size;
    // Première passe : on initialise la compression (écrit déjà quelques données)
    if ( status < 0 && cinfo.dest->free_in_buffer >= bufferLimit ) {
        jpeg_start_compress ( &cinfo, true );
        status = 0;
    }
    while ( cinfo.next_scanline < cinfo.image_height && cinfo.dest->free_in_buffer >= bufferLimit ) {
        image->getline ( linebuffer, cinfo.next_scanline );
        if ( jpeg_write_scanlines ( &cinfo, &linebuffer, 1 ) < 1 ) break;
    }
    if ( status == 0 && cinfo.next_scanline >= cinfo.image_height && cinfo.dest->free_in_buffer >= bufferLimit/10 ) {
        jpeg_finish_compress ( &cinfo );
        status = 1;
    }
    return ( size - cinfo.dest->free_in_buffer );
}

/** Destructeur */
JPEGEncoder::~JPEGEncoder() {
    delete cinfo.dest;
    jpeg_destroy_compress ( &cinfo );
    delete[] linebuffer;
    delete image;
}
