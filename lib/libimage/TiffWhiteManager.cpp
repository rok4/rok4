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

#include "TiffWhiteManager.h"
#include "tiffio.h"
#include <cstdlib>
#include <stdint.h>
#include <iostream>
#include <string.h>
#include <queue>

using namespace std;

uint8_t white[3] = {255,255,255};
uint8_t whiteForData[3] = {254,254,254};
uint8_t whiteForNodata[3] = {255,255,255};


TiffWhiteManager::TiffWhiteManager(char* input, char* output, bool bRemoveWhite, bool bAddNodataWhite) :
        input(input), output(output), bRemoveWhite(bRemoveWhite), bAddNodataWhite(bAddNodataWhite) {}

/**
 * @fn treatWhite
 * @brief White managment. We need to have no white pixel in images. This color have to be reserved for nodata. In this way, white could be replaced by transparent in a 3D displayer.
 * @author IGN
*
*/

inline void error(string message) {
    cerr << message << endl;
}

bool TiffWhiteManager::treatWhite() {
    
    TIFF *TIFF_FILE = 0;
    
    TIFF_FILE = TIFFOpen(input, "r");
    if(!TIFF_FILE) {
        error("Unable to open file for reading: " + string(input));
        return false;
    }
    if( ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, &width)                       ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, &height)                     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, &photometric)                ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel)||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_COMPRESSION, &compression)                ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, &rowsperstrip)              ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_EXTRASAMPLES, &nb_extrasamples, &extrasamples))
    {
        error("Error reading file: " +  string(input));
        return false;
    }

    if (planarconfig != 1)  {
        error("Sorry : only planarconfig = 1 is supported");
        return false;
    }
    if (bitspersample != 8)  {
        error("Sorry : only bitspersample = 8 is supported");
        return false;
    }
    if (sampleperpixel != 3)  {
        error("Sorry : tool white manager is just available for sampleperpixel = 3");
        return false;
    }
    
    IM  = new uint8_t[width * height * sampleperpixel];

    for(int h = 0; h < height; h++) {
        if(TIFFReadScanline(TIFF_FILE, IM + width*sampleperpixel*h, h) == -1) {
            error("Unable to read line to " + string(input));
            return false;
        }
    }
    
    TIFFClose(TIFF_FILE);
    
    // 'white' pixels are replaced by 'whiteForData' pixels
    if (bRemoveWhite) {
        for(int i = 0; i < width * height * sampleperpixel; i += sampleperpixel) {
            if(! memcmp(IM+i,white,sampleperpixel)) {
                memcpy(IM+i,whiteForData,sampleperpixel);
            }
        }
    }
    
    // 'whiteForData' pixels which touch edges are replaced by 'whiteForNodata' pixels
    if (bAddNodataWhite) {
        addNodataWhite();
    }
    
    
    TIFF_FILE = TIFFOpen(output, "w");
    if(!TIFF_FILE) {
        error("Unable to open file for writting: " + string(output));
        return false;
    }
    if( ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, width)               ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, height)             ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, bitspersample)    ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel) ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, photometric)        ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, rowsperstrip)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_PLANARCONFIG, planarconfig)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_COMPRESSION, compression)        ||
        (nb_extrasamples && ! TIFFSetField(TIFF_FILE, TIFFTAG_EXTRASAMPLES, nb_extrasamples, extrasamples)))
    {
        error("Error writting file: " +  string(output));
        return false;
    }
    
    uint8_t *LINE = new uint8_t[width * sampleperpixel];
    
    // output image is written
    for(int h = 0; h < height; h++) {
        memcpy(LINE, IM+h*width*sampleperpixel, width * sampleperpixel);
        if(TIFFWriteScanline(TIFF_FILE, LINE, h) == -1) {
            error("Unable to write line to " + string(output));
            return false;
        }
    }
    
    TIFFClose(TIFF_FILE);
    delete[] IM; 
    
    return true;

}

/**
 * @fn addNodataWhite
 * @brief We begin with front pixels which contain 'whiteForData' (254,254,254 by default) and we replace them with whiteForNodata pixels (255,255,255 by default). Then we propagate white in the image. This pixels are considered to be nodata.
 * @author IGN
*
*/

void TiffWhiteManager::addNodataWhite() {
    
    queue<int> Q;
    bool* MASK = new bool[width * height];
    memset(MASK, false, width * height);
    
    // Initialisation : we identify front pixels which are lightGray
    for(int pos = 0; pos < width; pos++) { // top
        if(!memcmp(IM + sampleperpixel * pos, whiteForData, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    }
    for(int pos = width*(height-1); pos < width*height; pos++) { // bottom
        if(!memcmp(IM + sampleperpixel * pos, whiteForData, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    }
    for(int pos = 0; pos < width*height; pos += width) { // left
        if(!memcmp(IM + sampleperpixel * pos, whiteForData, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    }
    for(int pos = width -1; pos < width*height; pos+= width) { // right
        if(!memcmp(IM + sampleperpixel * pos, whiteForData, sampleperpixel)) {Q.push(pos); MASK[pos] = true;}
    }
    
    if(Q.empty()) {
        // No nodata pixel identified, nothing to do
        delete[] MASK;
        return ;
    }
    
    // while there are 'whiteForData' pixels which can propagate, we do it
    while(!Q.empty()) {
        int pos = Q.front();
        Q.pop();
        int newpos;
        if (pos % width > 0) {
            newpos = pos - 1;
            if(!memcmp(IM + newpos*sampleperpixel, whiteForData, sampleperpixel) && !MASK[newpos]) {
                MASK[newpos] = true;
                Q.push(newpos);
            }
        }
        if (pos % width < width - 1) {
            newpos = pos + 1;
            if(!memcmp(IM + newpos*sampleperpixel, whiteForData, sampleperpixel) && !MASK[newpos]) {
                MASK[newpos] = true;
                Q.push(newpos);
            }
        }
        if (pos / width > 0) {
            newpos = pos - width;
            if(!memcmp(IM + newpos*sampleperpixel, whiteForData, sampleperpixel) && !MASK[newpos]) {
                MASK[newpos] = true;
                Q.push(newpos);
            }
        }
        if (pos / width < height - 1) {
            newpos = pos + width;
            if(!memcmp(IM + newpos*sampleperpixel, whiteForData, sampleperpixel) && !MASK[newpos]) {
                MASK[newpos] = true;
                Q.push(newpos);
            }
        }
    }
    
    for(int i = 0; i < width * height * sampleperpixel; i += sampleperpixel) {
        if(MASK[i/sampleperpixel]) {
            memcpy(IM+i,whiteForNodata,sampleperpixel);
        }
    }
    
    delete[] MASK;
}

