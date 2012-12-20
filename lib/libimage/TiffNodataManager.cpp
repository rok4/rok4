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
 * \file TiffNodataManager.cpp
 ** \~french
 * \brief Implémentation de la classe TiffNodataManager, permettant de modifier la couleur de nodata des images à canal entier
 ** \~english
 * \brief Implement classe TiffNodataManager, allowing to modify nodata color for byte sample image
 */

#include "TiffNodataManager.h"
#include "Logger.h"
#include "tiffio.h"
#include <cstdlib>
#include <stdint.h>
#include <iostream>
#include <string.h>
#include <queue>

using namespace std;

TiffNodataManager::TiffNodataManager(uint16 channels, uint8_t *targetValue, uint8_t *dataValue, uint8_t *nodataValue) :
        channels(channels), targetValue(targetValue), dataValue(dataValue), nodataValue(nodataValue)
{
    if (memcmp(targetValue,nodataValue,channels)) {
        newNodataValue = true;
    } else {
        // La nouvelle valeur de non-donnée est la même que la couleur cible : on ne change pas la couleur de non-donnée
        newNodataValue = false;
    }
    
    if (memcmp(targetValue,dataValue,channels)) {
        // Pour changer la couleur des données contenant la couleur cible, on doit la supprimer complètement puis remettre le nodata à la couleur initiale
        removeTargetValue = true;
        newNodataValue = true;
    } else {
        // La nouvelle valeur de donnée est la même que la couleur cible : on ne supprime donc pas la couleur cible des données
        removeTargetValue = false;
    }

    if (! newNodataValue && ! removeTargetValue) {
        LOGGER_INFO("TiffNodataManger have nothing to do !");
    }
}

bool TiffNodataManager::treatNodata(char* input, char* output)
{
    if (! newNodataValue && ! removeTargetValue) {
        LOGGER_INFO("Have nothing to do !");
        return true;
    }

    uint32 width , height, rowsperstrip;
    uint16 bitspersample, samplesperpixel, photometric, compression , planarconfig, nb_extrasamples;
    
    TIFF *TIFF_FILE = 0;
    
    TIFF_FILE = TIFFOpen(input, "r");
    if(!TIFF_FILE) {
        LOGGER_ERROR("Unable to open file for reading: " << input);
        return false;
    }
    if( ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, &width)                       ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, &height)                     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, &photometric)                ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel)||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_COMPRESSION, &compression)                ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, &rowsperstrip))
    {
        LOGGER_ERROR("Error reading file: " << input);
        return false;
    }

    if (planarconfig != 1)  {
        LOGGER_ERROR("Sorry : only single image plane as planar configuration is handled");
        return false;
    }
    if (bitspersample != 8)  {
        LOGGER_ERROR("Sorry : only byte sample are handled");
        return false;
    }
    
    if (samplesperpixel > channels)  {
        LOGGER_ERROR("The nodata manager is not adapted (samplesperpixel have to be " << channels <<
                        " or less) for the image " << input << " (" << samplesperpixel << ")");
        return false;
    }
    
    uint8_t *IM  = new uint8_t[width * height * samplesperpixel];

    for(int h = 0; h < height; h++) {
        if(TIFFReadScanline(TIFF_FILE, IM + width*samplesperpixel*h, h) == -1) {
            LOGGER_ERROR("Unable to read line to " + string(input));
            return false;
        }
    }
    
    TIFFClose(TIFF_FILE);
    
    // 'targetValue' pixels are replaced by 'dataValue' pixels
    if (removeTargetValue) {
        for(int i = 0; i < width * height; i++) {
            if(! memcmp(IM+i*samplesperpixel,targetValue,samplesperpixel)) {
                memcpy(IM+i*samplesperpixel,dataValue,samplesperpixel);
            }
        }
    }
    
    // 'dataValue' pixels which touch edges are replaced by 'nodataValue' pixels
    if (newNodataValue) {
        changeNodataValue(IM, width , height, samplesperpixel);
    }
    
    uint16_t extrasample = EXTRASAMPLE_ASSOCALPHA;
    TIFF_FILE = TIFFOpen(output, "w");
    if(!TIFF_FILE) {
        LOGGER_ERROR("Unable to open file for writting: " + string(output));
        return false;
    }
    if( ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, width)               ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, height)             ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, bitspersample)    ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel) ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, photometric)        ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, rowsperstrip)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_PLANARCONFIG, planarconfig)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_COMPRESSION, compression))
    {
        LOGGER_ERROR("Error writting file: " +  string(output));
        return false;
    }

    if (samplesperpixel == 4) {
        if (! TIFFSetField(TIFF_FILE, TIFFTAG_EXTRASAMPLES,1,&extrasample))
            LOGGER_ERROR("Error writting file: " +  std::string(output));
    }
    
    uint8_t *LINE = new uint8_t[width * samplesperpixel];
    
    // output image is written
    for(int h = 0; h < height; h++) {
        memcpy(LINE, IM+h*width*samplesperpixel, width * samplesperpixel);
        if(TIFFWriteScanline(TIFF_FILE, LINE, h) == -1) {
            LOGGER_ERROR("Unable to write line to " + string(output));
            return false;
        }
    }
    
    TIFFClose(TIFF_FILE);
    delete[] IM;
    delete[] LINE;
    
    return true;
}

void TiffNodataManager::changeNodataValue(uint8_t* IM, uint32 width , uint32 height,uint16 samplesperpixel) {
    
    queue<int> Q;
    uint8_t MASK[width * height];
    memset(MASK, 0, width * height);
    
    // Initialisation : we identify front pixels which are lightGray
    for(int pos = 0; pos < width; pos++) { // top
        if(! memcmp(IM + samplesperpixel * pos, dataValue, samplesperpixel)) {Q.push(pos); MASK[pos] = 255;}
    }
    for(int pos = width*(height-1); pos < width*height; pos++) { // bottom
        if(! memcmp(IM + samplesperpixel * pos, dataValue, samplesperpixel)) {Q.push(pos); MASK[pos] = 255;}
    }
    for(int pos = 0; pos < width*height; pos += width) { // left
        if(! memcmp(IM + samplesperpixel * pos, dataValue, samplesperpixel)) {Q.push(pos); MASK[pos] = 255;}
    }
    for(int pos = width -1; pos < width*height; pos+= width) { // right
        if(! memcmp(IM + samplesperpixel * pos, dataValue, samplesperpixel)) {Q.push(pos); MASK[pos] = 255;}
    }
    
    if(Q.empty()) {
        // No nodata pixel identified, nothing to do
        return ;
    }
    
    // while there are 'whiteForData' pixels which can propagate, we do it
    while(!Q.empty()) {
        int pos = Q.front();
        Q.pop();
        int newpos;
        if (pos % width > 0) {
            newpos = pos - 1;
            if(! MASK[newpos] && ! memcmp(IM + newpos*samplesperpixel, dataValue, samplesperpixel)) {
                MASK[newpos] = 255;
                Q.push(newpos);
            }
        }
        if (pos % width < width - 1) {
            newpos = pos + 1;
            if(! MASK[newpos] && ! memcmp(IM + newpos*samplesperpixel, dataValue, samplesperpixel)) {
                MASK[newpos] = 255;
                Q.push(newpos);
            }
        }
        if (pos / width > 0) {
            newpos = pos - width;
            if(! MASK[newpos] && ! memcmp(IM + newpos*samplesperpixel, dataValue, samplesperpixel)) {
                MASK[newpos] = 255;
                Q.push(newpos);
            }
        }
        if (pos / width < height - 1) {
            newpos = pos + width;
            if(! MASK[newpos] && ! memcmp(IM + newpos*samplesperpixel, dataValue, samplesperpixel)) {
                MASK[newpos] = 255;
                Q.push(newpos);
            }
        }
    }
    
    for(int i = 0; i < width * height; i ++) {
        if (MASK[i]) {
            memcpy(IM+i*samplesperpixel,nodataValue,samplesperpixel);
        }
    }
}

