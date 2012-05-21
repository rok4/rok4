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


/**
 * @file removeFF.cpp
 * @brief On ne veut pas qu'il y ait des canaux à 255 lorsque ceux ci sont des entiers sur 8 bits.
 * Dans les cas du rgb, on ne veut donc pas de blanc dans l'image. Cette couleur doit être réservée au nodata,
 * même si ce n'est pas celle donnée dans la configuration de be4.
 * La raison est la suivante : le blanc pourra être rendu transparent, et dans le cas du jpeg, le blanc de nodata
 * doit être pur, c'est pourquoi on remplit de blanc les blocs (16*16 pixels) qui contiennent au moins un pixel blanc.
 * Pour éviter de "trouer" les données, on remplace ce blanc légitime par du gris très clair (FEFEFE).
 * @author IGN
*
*/

#include "tiffio.h"
#include <cstdlib>
#include <stdint.h>
#include <iostream>
#include <string.h>
#include "../be4version.h"

using namespace std;

void usage() {
    cerr << "removeFF version "<< BE4_VERSION << endl;
    cerr << endl << "usage: removeFF <input_file> <output_file>" << endl << endl;
    cerr << "remove 255 samples in pixels in the image" << endl;
}

void error(string message) {
    cerr << message << endl;
    usage();
    exit(1);
}

TIFF *TIFF_FILE = 0;

uint8_t *IM ;

uint32 width, height, rowsperstrip = -1;
uint16 bitspersample, sampleperpixel, photometric, compression = -1, planarconfig, nb_extrasamples;
uint16 *extrasamples;

int main(int argc, char* argv[]) {
    char *input_file = 0, *output_file = 0;

    for(int i = 1; i < argc; i++) {
        if(!input_file) input_file = argv[i];
        else if(!output_file) output_file = argv[i];
        else error("Error : argument must specify exactly one input file and one output file");
    }
    if(!output_file || !input_file) error("Error : argument must specify exactly one input file and one output file");

    TIFF_FILE = TIFFOpen(input_file, "r");
    if(!TIFF_FILE) error("Unable to open file for reading: " + string(input_file));
    if( ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, &width)                       ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, &height)                     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, &photometric)                ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel)||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_COMPRESSION, &compression)                ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, &rowsperstrip)              ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_EXTRASAMPLES, &nb_extrasamples, &extrasamples))
            error("Error reading file: " +  string(input_file));

    if (planarconfig != 1)  error("Sorry : only planarconfig = 1 is supported");
    if (bitspersample != 8)  error("Sorry : only bitspersample = 8 is supported");
    if (compression != 1)  error("Sorry : compression not accepted");
    
    IM  = new uint8_t[width * height * sampleperpixel];

    for(int h = 0; h < height; h++) 
        if(TIFFReadScanline(TIFF_FILE, IM + width*sampleperpixel*h, h) == -1) 
            error("Unable to read data");
    
    TIFFClose(TIFF_FILE);

    TIFF_FILE = TIFFOpen(output_file, "w");
    if(!TIFF_FILE) error("Unable to open file for writting: " + string(output_file));
    if( ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, width)               ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, height)             ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, bitspersample)    ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel) ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, photometric)        ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, rowsperstrip)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_PLANARCONFIG, planarconfig)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_COMPRESSION, compression)        ||
        (nb_extrasamples && ! TIFFSetField(TIFF_FILE, TIFFTAG_EXTRASAMPLES, nb_extrasamples, extrasamples)))
            error("Error writting file: " +  string(output_file));
    
    uint8_t *LINE = new uint8_t[width * sampleperpixel];
    
    for(int h = 0; h < height; h++) {
        
        memcpy(LINE, IM+h*width*sampleperpixel, width * sampleperpixel);

        for(int w = 0; w < width; w++) {
            if(LINE[h*width+w] == 255) {
                LINE[h*width+w] == 254;
            }
        }
        if(TIFFWriteScanline(TIFF_FILE, LINE, h) == -1) error("Unable to write line to " + string(output_file));
    }

    TIFFClose(TIFF_FILE);
    delete[] IM;
    delete[] LINE;
    return 0;
}

