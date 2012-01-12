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
    cerr << "Usage: nodataIdentifier [-n nodataInit] inout.tiff" << endl;
    cerr << "Pixels in file 'inout.tiff' which touch borders and contains the nodata value 'nodataInit' will be changed in white" << endl;
}

void error(string message) {
    cerr << message << endl;
    exit(2);
}

TIFF *TIFF_FILE = 0;

char* tiff_file = 0;
char* strnodata = 0;
uint8_t nodataColor[4] = {255,255,255,255};
uint8_t *IM ;
bool *MASK;
queue<int> Q;

uint32 width, height, rowsperstrip = -1;
uint16 bitspersample, sampleperpixel, photometric, compression = -1, planarconfig, nb_extrasamples;
uint16 *extrasamples;

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

inline void propagate(int newpos) {
    if(!memcmp(IM + newpos*sampleperpixel, nodataColor, sampleperpixel) && !MASK[newpos]) {
        MASK[newpos] = true;
        Q.push(newpos);
    }
}

int main(int argc, char* argv[]) {

    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'h': 
                    usage();
                    exit(0);
                case 'n': 
                    if(++i == argc) error("missing parameter in -n argument");
                    strnodata = argv[i];
                    break;
                break;
            }
        }
        else {
            if (tiff_file == 0) tiff_file = argv[i];
            else error("too many parameters");
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
    
    if (strlen(strnodata) != 2*sampleperpixel) {
        error("nodata parameter too short or too long");
    }
    
    if (strnodata != 0) {
        uint8_t nodata = 255;
        for (int i=0; i<sampleperpixel; i++) {
            int a1 = h2i(strnodata[i*2]);
            int a0 = h2i(strnodata[i*2+1]);
            if (a1 < 0 || a0 < 0) error("invalid caracter in the nodata value");
            nodata = 16*a1+a0;
            nodataColor[i] = nodata;
        }
    }
    
    IM  = new uint8_t[width * height * sampleperpixel];
    MASK = new bool[width * height];
    memset(MASK, false, width * height);

    for(int h = 0; h < height; h++) 
        if(TIFFReadScanline(TIFF_FILE, IM + width*sampleperpixel*h, h) == -1) 
            error("Unable to read data");
    
    TIFFClose(TIFF_FILE);

    for(int pos = 0; pos < width; pos++) 
        if(!memcmp(IM + sampleperpixel * pos, nodataColor, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    for(int pos = width*(height-1); pos < width*height; pos++) 
        if(!memcmp(IM + sampleperpixel * pos, nodataColor, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    for(int pos = 0; pos < width*height; pos += width)
        if(!memcmp(IM + sampleperpixel * pos, nodataColor, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    for(int pos = width -1; pos < width*height; pos+= width)
        if(!memcmp(IM + sampleperpixel * pos, nodataColor, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    
    if(Q.empty()) {
        cout << "No nodata pixel identified, nothing to do." << endl;
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
                    LINE[sampleperpixel*w + c] = (2-c)*127;
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


