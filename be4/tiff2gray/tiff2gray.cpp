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

#include "tiffio.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "../be4version.h"
#include "OneBitConverter.h"

using namespace std;

void usage() {
    cerr << "tiff2gray version "<< BE4_VERSION << std::endl;
    cerr << endl << "usage: tiff2gray [-c (none|zip|packbits|jpeg|lzw)] <input_file> <output_file>" << endl << endl;
    cerr << "       The default compression is the same as the source image." << endl << endl;
}

void error ( string message ) {
    cerr << message << endl;
    exit ( 1 );
}


void tiff_copy ( TIFF *input, TIFF *output, bool minIsWhite ) {
    uint8_t buffer_in[TIFFStripSize ( input )];
    uint8_t buffer_out[TIFFStripSize ( output )];
    int nb_strip = TIFFNumberOfStrips ( input );
    for ( int n = 0; n < nb_strip; n++ ) {
        int size = TIFFReadEncodedStrip ( input, n, buffer_in, -1 );
        cerr << "read size = " << size << std::endl;
        if ( size == -1 ) error ( "Error reading data" );
        if (minIsWhite) {
            OneBitConverter::minwhiteToGray ( buffer_out, buffer_in, size );
        } else {
            OneBitConverter::minblackToGray ( buffer_out, buffer_in, size );
        }
        
        if ( TIFFWriteEncodedStrip ( output, n, buffer_out, 8*size ) == -1 ) error ( "Error writing data" );
    }
}


#define pack(a,b,c,d)	((long)(((a)<<24)|((b)<<16)|((c)<<8)|(d)))

int main ( int argc, char* argv[] ) {
    TIFF *INPUT, *OUTPUT;
    char *input_file = 0, *output_file = 0;
    uint32 width, height, rowsperstrip;
    uint16 bitspersample, samplesperpixel, photometric, compression = -1, planarconfig;

    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'c': // compression
                if ( ++i == argc ) error ( "Error in -c option" );
                if ( strncmp ( argv[i], "none",4 ) == 0 ) compression = COMPRESSION_NONE;
                else if ( strncmp ( argv[i], "zip",3 ) == 0 ) compression = COMPRESSION_ADOBE_DEFLATE;
                else if ( strncmp ( argv[i], "packbits",8 ) == 0 ) compression = COMPRESSION_PACKBITS;
                else if ( strncmp ( argv[i], "jpeg",4 ) == 0 ) compression = COMPRESSION_JPEG;
                else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) compression = COMPRESSION_LZW;
                else compression = COMPRESSION_NONE;
                break;
            default :
                usage();
                exit ( 0 );
            }
        } else {
            if ( !input_file ) input_file = argv[i];
            else if ( !output_file ) output_file = argv[i];
            else error ( "Error : argument must specify exactly one input file and one output file" );
        }
    }
    if ( !output_file ) error ( "Error : argument must specify exactly one input file and one output file" );

    INPUT = TIFFOpen ( input_file, "r" );
    if ( INPUT == NULL ) error ( "Unable to open input file: " + string ( input_file ) );

    if ( compression == ( uint16 ) ( -1 ) ) TIFFGetField ( INPUT, TIFFTAG_COMPRESSION, &compression );

    if ( ! TIFFGetField ( INPUT, TIFFTAG_IMAGEWIDTH, &width )                   ||
            ! TIFFGetField ( INPUT, TIFFTAG_IMAGELENGTH, &height )                 ||
            ! TIFFGetField ( INPUT, TIFFTAG_BITSPERSAMPLE, &bitspersample )        ||
            ! TIFFGetFieldDefaulted ( INPUT, TIFFTAG_PLANARCONFIG, &planarconfig ) ||
            ! TIFFGetField ( INPUT, TIFFTAG_PHOTOMETRIC, &photometric )            ||
            ! TIFFGetField ( INPUT, TIFFTAG_ROWSPERSTRIP, &rowsperstrip )          ||
            ! TIFFGetFieldDefaulted ( INPUT, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel ) )
        error ( "Error reading input file: " + string ( input_file ) );

    if ( planarconfig != 1 ) error ( "Sorry : only planarconfig = 1 is supported" );

    OUTPUT = TIFFOpen ( output_file, "w" );
    if ( OUTPUT == NULL ) error ( "Unable to open output file: " + string ( output_file ) );
    if ( ! TIFFSetField ( OUTPUT, TIFFTAG_IMAGEWIDTH, width )                   ||
            ! TIFFSetField ( OUTPUT, TIFFTAG_IMAGELENGTH, height )                 ||
            ! TIFFSetField ( OUTPUT, TIFFTAG_BITSPERSAMPLE, 8 )                    ||
            ! TIFFSetField ( OUTPUT, TIFFTAG_SAMPLESPERPIXEL, 1 )                  ||
            ! TIFFSetField ( OUTPUT, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK ) ||
            ! TIFFSetField ( OUTPUT, TIFFTAG_ROWSPERSTRIP, rowsperstrip )          ||
            ! TIFFSetField ( OUTPUT, TIFFTAG_COMPRESSION, compression ) )
        error ( "Error writing output file: " + string ( output_file ) );

    switch ( pack ( planarconfig,photometric,samplesperpixel,bitspersample ) ) {
    case pack ( PLANARCONFIG_CONTIG,PHOTOMETRIC_MINISWHITE,1,1 ) :
        tiff_copy ( INPUT, OUTPUT, true );
        break;
    case pack ( PLANARCONFIG_CONTIG,PHOTOMETRIC_MINISBLACK,1,1 ) :
        tiff_copy ( INPUT, OUTPUT, false );
        break;
    default :
        error ( "Unsupported input format" );
    }

    TIFFClose ( INPUT );
    TIFFClose ( OUTPUT );
    return 0;
}

