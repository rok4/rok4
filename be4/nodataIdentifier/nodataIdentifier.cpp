
/**
 * @file nodataIdentifier.cpp
 * @brief Modification de la valeur de nodata en une autre, en faisant attention de ne pas modifier les pixels légitimes de l'image, dont la valeur est celle du nodata. On modifie les pixels en partant des bords.
 * @author IGN
*
*/

#include "tiffio.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <queue>
#include <stdint.h>
#include <fstream>
#include <string.h>

using namespace std;

void usage() {
    cout << "Usage: nodataIdentifier -n1 nodata1 [-n2 nodata2] inout.tiff" << endl;
    cout << "Pixels in file 'inout.tiff' which touch edges and contains the value 'nodata1' will be changed in 'nodata2' (white by default). Values are in hexadecimal format." << endl;
}

void error(string message) {
    cerr << message << endl;
    exit(2);
}

TIFF *TIFF_FILE = 0;

char* tiff_file = 0;
uint8_t* nodataColor1;
uint8_t* nodataColor2;
uint8_t *IM ;
bool *MASK;
queue<int> Q;

uint32 width, height, rowsperstrip = -1;
uint16 bitspersample, sampleperpixel, photometric, compression = -1, planarconfig, nb_extrasamples;
uint16 *extrasamples;

/**
*@fn int h2i(char s)
* Hexadecimal -> int
* @return valeur décimale positive, -1 en cas d'erreur
*/

int h2i(char s)
{
    if('0' <= s && s <= '9')
        return (s - '0');
    if('a' <= s && s <= 'f')
        return (s - 'a' + 10);
    if('A' <= s && s <= 'F')
        return (10 + s - 'A');
    else
        return -1; /* invalid input! */
}

/**
*@fn void propagate(int newpos)
* Ajoute cette nouvelle position en tant que pixel de nodata si celui ci contient la valeur de nodata
*/

inline void propagate(int newpos) {
    if(!memcmp(IM + newpos*sampleperpixel, nodataColor1, sampleperpixel) && !MASK[newpos]) {
        MASK[newpos] = true;
        Q.push(newpos);
    }
}

/**
*@fn int main(int argc, char* argv[])
* Implémentation de la commande nodataIdentifier
* 
* Usage : nodataIdentifier [-n1 nodataIn] [-n2 nodataOut] inout.tiff \n
* nodataOut is 255 (FF) for each sample by default.
*/

int main(int argc, char* argv[]) {
    
    char* strnodata1 = 0;
    char* strnodata2 = 0;

    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'h': 
                    usage();
                    exit(0);
                case 'n': 
                    if(++i == argc) error("Missing parameter in -n argument");
                    switch(argv[i-1][2]){
                        case '1':
                            strnodata1 = argv[i];
                            break;
                        case '2':
                            strnodata2 = argv[i];
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
        else {
            if (tiff_file == 0) tiff_file = argv[i];
            else error("Too many parameters");
        }
    }
    if(tiff_file == 0) error("Missing input file");

    TIFF_FILE = TIFFOpen(tiff_file, "r");
    if(!TIFF_FILE) error("Unable to open file for reading: " + string(tiff_file));
    if( ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, &width)                       ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, &height)                     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, &photometric)                ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel)||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_COMPRESSION, &compression)                ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, &rowsperstrip)              ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_EXTRASAMPLES, &nb_extrasamples, &extrasamples))
            error("Error reading file: " +  string(tiff_file));

    if (planarconfig != 1)  error("Sorry : only planarconfig = 1 is supported");
    if (bitspersample != 8)  error("Sorry : only bitspersample = 8 is supported");
    if (compression != 1)  error("Sorry : compression not accepted");
    
    if (strnodata1 != 0 && strlen(strnodata1) < 2*sampleperpixel || strnodata2 != 0 && strlen(strnodata2) < 2*sampleperpixel) {
        error("A nodata parameter is too short");
    }
    
    nodataColor1 = new uint8_t[sampleperpixel];
    nodataColor2 = new uint8_t[sampleperpixel];
    
    if (strnodata1 != 0) {
        uint8_t nodata = 255;
        for (int i=0; i<sampleperpixel; i++) {
            int a1 = h2i(strnodata1[i*2]);
            int a0 = h2i(strnodata1[i*2+1]);
            if (a1 < 0 || a0 < 0) error("Invalid caracter in the input nodata value");
            nodata = 16*a1+a0;
            nodataColor1[i] = nodata;
        }
    } else {
        error("No input nodata value, impossible to continue");
    }
    
    if (strnodata2 != 0) {
        uint8_t nodata = 255;
        for (int i=0; i<sampleperpixel; i++) {
            int a1 = h2i(strnodata2[i*2]);
            int a0 = h2i(strnodata2[i*2+1]);
            if (a1 < 0 || a0 < 0) error("Invalid caracter in the output nodata value");
            nodata = 16*a1+a0;
            nodataColor2[i] = nodata;
        }
    } // else 255 (default)
    
    IM  = new uint8_t[width * height * sampleperpixel];
    MASK = new bool[width * height];
    memset(MASK, false, width * height);

    for(int h = 0; h < height; h++) 
        if(TIFFReadScanline(TIFF_FILE, IM + width*sampleperpixel*h, h) == -1) 
            error("Unable to read data");
    
    TIFFClose(TIFF_FILE);

    for(int pos = 0; pos < width; pos++) 
        if(!memcmp(IM + sampleperpixel * pos, nodataColor1, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    for(int pos = width*(height-1); pos < width*height; pos++) 
        if(!memcmp(IM + sampleperpixel * pos, nodataColor1, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    for(int pos = 0; pos < width*height; pos += width)
        if(!memcmp(IM + sampleperpixel * pos, nodataColor1, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    for(int pos = width -1; pos < width*height; pos+= width)
        if(!memcmp(IM + sampleperpixel * pos, nodataColor1, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    
    if(Q.empty()) {
        // No nodata pixel identified, nothing to do
        delete[] IM;
        delete[] MASK;
        return 0;
    }

    while(!Q.empty()) {
        int pos = Q.front();
        Q.pop();
        if (pos % width > 0) {
            propagate(pos - 1);
        }
        if (pos % width < width - 1) {
            propagate(pos + 1);
        }
        if (pos / width > 0) {
            propagate(pos - width);
        }
        if (pos / width < height - 1) {
            propagate(pos + width);
        }
    }

    uint8_t *LINE = new uint8_t[width * sampleperpixel];

    TIFF_FILE = TIFFOpen(tiff_file, "w");
    if(!TIFF_FILE) error("Unable to open file for writting: " + string(tiff_file));
    if( ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, width)               ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, height)             ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, bitspersample)    ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel) ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, photometric)        ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, rowsperstrip)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_PLANARCONFIG, planarconfig)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_COMPRESSION, compression)        ||
        (nb_extrasamples && ! TIFFSetField(TIFF_FILE, TIFFTAG_EXTRASAMPLES, nb_extrasamples, extrasamples)))
            error("Error writting file: " +  string(tiff_file));
    
    for(int h = 0; h < height; h++) {
        
        memcpy(LINE, IM+h*width*sampleperpixel, width * sampleperpixel);

        for(int w = 0; w < width; w++) {
            if(MASK[h*width+w]) {
                for(int c = 0; c < sampleperpixel; c++) {
                    LINE[sampleperpixel*w + c] = nodataColor2[c];
                }
            }
        }
        if(TIFFWriteScanline(TIFF_FILE, LINE, h) == -1) error("Unable to write line to " + string(tiff_file));
    }

    TIFFClose(TIFF_FILE);
    delete[] IM;
    delete[] MASK;
    delete[] LINE;
    return 0;
    
}


