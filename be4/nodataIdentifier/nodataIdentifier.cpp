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
    cerr << "Usage: nodataIdentifier [-n nodataInit] input.tiff output.tiff";
}

void error(string message) {
    cerr << message << endl;
    exit(2);
}

TIFF *SRC_TIFF = 0;
TIFF *DST_TIFF  = 0;

char* src_tiff = 0;
char* dst_tiff = 0;
char* strnodata = 0;
uint8_t nodata = 255;
uint8_t color[3] = {255,255,255};
uint8_t *IM ;
bool *MASK;
int C;
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
    if(!memcmp(IM + newpos*C, color, C) && !MASK[newpos]) {
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
            if (src_tiff == 0) src_tiff = argv[i];
            else if (dst_tiff  == 0) dst_tiff  = argv[i];
            else error("too many parameters");
        }
    }
    if(dst_tiff == 0) error("Missing output file");
    if(src_tiff == 0) error("Missing input file");

    SRC_TIFF = TIFFOpen(src_tiff, "r");

    if( ! TIFFGetField(SRC_TIFF, TIFFTAG_IMAGEWIDTH, &width)                       ||
        ! TIFFGetField(SRC_TIFF, TIFFTAG_IMAGELENGTH, &height)                     ||
        ! TIFFGetField(SRC_TIFF, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
        ! TIFFGetFieldDefaulted(SRC_TIFF, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
        ! TIFFGetField(SRC_TIFF, TIFFTAG_PHOTOMETRIC, &photometric)                ||
        ! TIFFGetFieldDefaulted(SRC_TIFF, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel)||
        ! TIFFGetField(SRC_TIFF, TIFFTAG_COMPRESSION, &compression)                ||
        ! TIFFGetField(SRC_TIFF, TIFFTAG_ROWSPERSTRIP, &rowsperstrip)              ||
        ! TIFFGetFieldDefaulted(SRC_TIFF, TIFFTAG_EXTRASAMPLES, &nb_extrasamples, &extrasamples))
            error("Error reading input file: " +  string(src_tiff));

    if(planarconfig != 1)  error("Sorry : only planarconfig = 1 is supported");
    if(bitspersample != 8)  error("Sorry : only bitspersample = 8 is supported");
    
    if (strnodata != 0) {
        int a1 = h2i(strnodata[0]);
        int a0 = h2i(strnodata[1]);
        if (a1 < 0 || a0 < 0) error("invalid parameter in -n argument for image");
        nodata = 16*a1+a0;
        color = {nodata,nodata,nodata};
    }
    
    C = sampleperpixel;
    IM  = new uint8_t[width * height * C];
    MASK = new bool[width * height];
    memset(MASK, false, width * height);

    for(int h = 0; h < height; h++) 
        if(TIFFReadScanline(SRC_TIFF, IM + width*C*h, h) == -1) 
            error("Unable to read data");

    for(int pos = 0; pos < width; pos++) 
        if(!memcmp(IM + C * pos, color, C)) {Q.push(pos); MASK[pos] = true;}
    for(int pos = width*(height-1); pos < width*height; pos++) 
        if(!memcmp(IM + C * pos, color, C)) {Q.push(pos); MASK[pos] = true;}
    for(int pos = 0; pos < width*height; pos += width)
        if(!memcmp(IM + C * pos, color, C)) {Q.push(pos); MASK[pos] = true;}
    for(int pos = width -1; pos < width*height; pos+= width)
        if(!memcmp(IM + C * pos, color, C)) {Q.push(pos); MASK[pos] = true;}
    
    if(Q.empty() && strcmp(src_tiff, dst_tiff) == 0) {
        delete[] IM;
        delete[] MASK;
        cout << "Il n'y a pas de nodata qui touche le bord" << endl;
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

    uint8_t *LINE = new uint8_t[width * C];

    DST_TIFF = TIFFOpen(dst_tiff, "w");
    if(!DST_TIFF) error("Unable to open output file: " + string(dst_tiff));
    if( ! TIFFSetField(DST_TIFF, TIFFTAG_IMAGEWIDTH, width)               ||
        ! TIFFSetField(DST_TIFF, TIFFTAG_IMAGELENGTH, height)             ||
        ! TIFFSetField(DST_TIFF, TIFFTAG_BITSPERSAMPLE, bitspersample)    ||
        ! TIFFSetField(DST_TIFF, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel) ||
        ! TIFFSetField(DST_TIFF, TIFFTAG_PHOTOMETRIC, photometric)        ||
        ! TIFFSetField(DST_TIFF, TIFFTAG_ROWSPERSTRIP, rowsperstrip)      ||
        ! TIFFSetField(DST_TIFF, TIFFTAG_PLANARCONFIG, planarconfig)      ||
        ! TIFFSetField(DST_TIFF, TIFFTAG_COMPRESSION, compression)        ||
        (nb_extrasamples && ! TIFFSetField(DST_TIFF, TIFFTAG_EXTRASAMPLES, nb_extrasamples, extrasamples)))
            error("Error writting output file: " +  string(dst_tiff));
    
    for(int h = 0; h < height; h++) {
        
        memcpy(LINE, IM+h*width*C, width * C);

        for(int w = 0; w < width; w++) {
            if(MASK[h*width+w]) {
                for(int c = 0; c < C; c++) {
                    LINE[C*w + c] = (2-c)*127;
                }
            }
        }
        if(TIFFWriteScanline(DST_TIFF, LINE, h) == -1) error("Unable to write line to " + string(dst_tiff));
    }

    TIFFClose(SRC_TIFF);
    TIFFClose(DST_TIFF);
    delete[] IM;
    delete[] MASK;
    delete[] LINE;
    return 0;
    
}


