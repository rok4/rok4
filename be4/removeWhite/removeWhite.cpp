
/**
 * @file removeWhite.cpp
 * @brief On ne veut pas qu'il y ait tous les canaux à 255 lorsque ceux ci sont des entiers sur 8 bits.
 * Dans les cas du rgb, on ne veut donc pas de blanc dans l'image. Cette couleur doit être réservée au nodata,
 * même si ce n'est pas celle donnée dans la configuration de be4.
 * La raison est la suivante : le blanc pourra être rendu transparent, et dans le cas du jpeg, le blanc de nodata
 * doit être pur, c'est pourquoi on remplit de blanc les blocs (16*16 pixels) qui contiennent au moins un pixel blanc.
 * Pour éviter de "trouer" les données, on supprime ce blanc légitime.
 * @author IGN
*
*/

#include "tiffio.h"
#include <cstdlib>
#include <iostream>
#include <string.h>

using namespace std;

void usage() {
    cerr << endl << "usage: removeWhite <input_file> <output_file>" << endl << endl;
    cerr << "remove white pixels in the image but not pixels which touch edges (nodata)" << endl;
}

void error(string message) {
    cerr << message << endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    TIFF *INPUT, *OUTPUT;
    char *input_file = 0, *output_file = 0;
    uint32 width, height, rowsperstrip;
    uint16 bitspersample, samplesperpixel, photometric, compression = -1, planarconfig;

    for(int i = 1; i < argc; i++) {
        if(!input_file) input_file = argv[i];
        else if(!output_file) output_file = argv[i];
        else error("Error : argument must specify exactly one input file and one output file");
    }
    if(!output_file) error("Error : argument must specify exactly one input file and one output file");

    INPUT = TIFFOpen(input_file, "r");
    if(INPUT == NULL) error("Unable to open input file: " + string(input_file));

    TIFFGetField(INPUT, TIFFTAG_COMPRESSION, &compression);
    if (compression != COMPRESSION_NONE) {
        error("This treatment is unabled for compressed images like " + string(input_file));
    }

    if(! TIFFGetField(INPUT, TIFFTAG_IMAGEWIDTH, &width)                ||
    ! TIFFGetField(INPUT, TIFFTAG_IMAGELENGTH, &height)                 ||
    ! TIFFGetField(INPUT, TIFFTAG_BITSPERSAMPLE, &bitspersample)        ||
    ! TIFFGetFieldDefaulted(INPUT, TIFFTAG_PLANARCONFIG, &planarconfig) ||
    ! TIFFGetField(INPUT, TIFFTAG_PHOTOMETRIC, &photometric)            ||
    ! TIFFGetField(INPUT, TIFFTAG_ROWSPERSTRIP, &rowsperstrip)          ||
    ! TIFFGetFieldDefaulted(INPUT, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel))
    error("Error reading input file: " + string(input_file));

    if(planarconfig != 1) error("Sorry : only planarconfig = 1 is supported");

    OUTPUT = TIFFOpen(output_file, "w");
    if(OUTPUT == NULL) error("Unable to open output file: " + string(output_file));
    if(! TIFFSetField(OUTPUT, TIFFTAG_IMAGEWIDTH, width)                   ||
    ! TIFFSetField(OUTPUT, TIFFTAG_IMAGELENGTH, height)                 ||
    ! TIFFSetField(OUTPUT, TIFFTAG_BITSPERSAMPLE, 8)                    ||
    ! TIFFSetField(OUTPUT, TIFFTAG_SAMPLESPERPIXEL, 1)                  ||
    ! TIFFSetField(OUTPUT, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK) ||
    ! TIFFSetField(OUTPUT, TIFFTAG_ROWSPERSTRIP, rowsperstrip)          ||
    ! TIFFSetField(OUTPUT, TIFFTAG_COMPRESSION, compression))
    error("Error writing output file: " + string(output_file));

    TIFFClose(INPUT);
    TIFFClose(OUTPUT);
    return 0;
}

