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
 * \file merge4tiff.cpp
 * \brief Sous echantillonage de 4 images 
 * \author IGN
*
*/

#include "tiffio.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string.h>
#include <stdint.h>
#include "../be4version.h"


void usage() {
  std::cerr << "merge4tiff version "<< BE4_VERSION << std::endl;
  std::cerr << "Usage : merge4tiff -g gamma_correction -n nodata -c compression -r rowsperstrip -b background_image -i1 image1 -i2 image2 -i3 image3 -i4 image4 imageOut" << std::endl;
  std::cerr << "-n : one integer per samples, separated by comma, in decimal format, mandatory. Examples :"<< std::endl;
  std::cerr << "       - for images (u_int8) : 255,255,0"<< std::endl;
  std::cerr << "       - for DTM (float) : -99999"<< std::endl;
  std::cerr << "-b : the background image, optional" << std::endl;
  std::cerr << "-g : default gamma is 1.0 (have no effect)" << std::endl;
  std::cerr << "-c : work compression" << std::endl;
  std::cerr << "-r : should not be used in be4 context" << std::endl;
  std::cerr << std::endl << "Images spatial distribution :" << std::endl;
  std::cerr << "   image1 | image2" << std::endl;
  std::cerr << "   -------+-------" << std::endl;
  std::cerr << "   image3 | image4" << std::endl;
}

void error(std::string message) {
    std::cerr << message << std::endl;
    usage();
    exit(1);
}

double gammaM4t;
char* strnodata;
float* nodataFloat32;
uint8_t* nodataUInt8;

// Chemins des images
char* backgroundImage;
char* backgroundMask;
char* inputImages[4];
char* inputMasks[4];
char* outputImage;
char* outputMask;

uint32_t width,height,rowsperstrip;
uint16_t compression,bitspersample,samplesperpixel,sampleformat,photometric,planarconfig;

void parseCommandLine(int argc, char* argv[])
{
    // Initialisation
    gammaM4t = 1.;
    strnodata = 0;
    compression = -1;
    rowsperstrip = -1;
    backgroundImage = 0;
    backgroundMask = 0;
    for (int i=0;i<4;i++) {inputImages[i] = 0; inputMasks[i] = 0;}
    outputImage = 0;
    outputMask = 0;

    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'g': // gamma
                    if(++i == argc) error("Missing parameter in -g argument");
                    gammaM4t = atof(argv[i]);
                    if (gammaM4t <= 0.) error("invalid parameter in -g argument");
                    break;
                case 'n': // nodata
                    if(++i == argc) error("Missing parameter in -n argument");
                    strnodata = argv[i];
                    break;
                case 'h': // help
                    usage();
                    exit(0);
                case 'c': // compression
                    if(++i == argc) error("Error in -c option");
                    if(strncmp(argv[i], "none",4) == 0 || strncmp(argv[i], "raw",3) == 0) compression = COMPRESSION_NONE;
                    else if(strncmp(argv[i], "zip",3) == 0) compression = COMPRESSION_ADOBE_DEFLATE;
                    else if(strncmp(argv[i], "pkb",3) == 0) compression = COMPRESSION_PACKBITS;
                    else if(strncmp(argv[i], "jpg",3) == 0) compression = COMPRESSION_JPEG;
                    else if(strncmp(argv[i], "lzw",3) == 0) compression = COMPRESSION_LZW;
                    else compression = COMPRESSION_NONE;
                    break;
                case 'r':
                    if(++i == argc) error("Missing parameter in -r argument");
                    rowsperstrip = atoi(argv[i]);
                    break;
                case 'b': // background image
                    if(++i == argc) error("Missing parameter in -b argument");
                    backgroundImage = argv[i];
                    break;
                case 'i': // images to merge
                    if(++i == argc) error("Missing parameter in -i argument");
                    switch(argv[i-1][2]){
                        case '1':
                            inputImages[0] = argv[i];
                            break;
                        case '2':
                            inputImages[1] = argv[i];
                            break;
                        case '3':
                            inputImages[2] = argv[i];
                            break;
                        case '4':
                            inputImages[3] = argv[i];
                            break;
                        default:
                            break;
                    }
                    break;
                case 'm': // associated masks
                    if(++i == argc) error("Missing parameter in -m argument");
                    switch(argv[i-1][2]){
                        case '1':
                            inputMasks[0] = argv[i];
                            break;
                        case '2':
                            inputMasks[1] = argv[i];
                            break;
                        case '3':
                            inputMasks[2] = argv[i];
                            break;
                        case '4':
                            inputMasks[3] = argv[i];
                            break;
                        case 'b':
                            backgroundMask = argv[i];
                            break;
                        case 'o':
                            outputMask = argv[i];
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    error("Unknown option");
            }
        } else {
            if (outputImage!=0) error ("ONE output file");
            outputImage = argv[i];
        }
    }
    if (strnodata == 0) {
        error ("Missing nodata value");
    }
    if (outputImage==0) error ("Missing output file");

}

void checkComponents(TIFF* image, bool isMask)
{
    uint32_t _width,_height,_rowsperstrip;
    uint16_t _bitspersample,_samplesperpixel,_sampleformat,_photometric,_compression,_planarconfig;

    if(width == 0) { // read the parameters of the first input file
        if( ! TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width)                   ||
        ! TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height)                     ||
        ! TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
        ! TIFFGetFieldDefaulted(image, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
        ! TIFFGetField(image, TIFFTAG_PHOTOMETRIC, &photometric)                ||
        ! TIFFGetFieldDefaulted(image, TIFFTAG_SAMPLEFORMAT, &sampleformat)     ||
        ! TIFFGetFieldDefaulted(image, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel))
        error(std::string("Error reading input file: ") + TIFFFileName(image));

        if (compression == (uint16)(-1)) TIFFGetField(image, TIFFTAG_COMPRESSION, &compression);
        if (rowsperstrip == (uint32)(-1)) TIFFGetField(image, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
        if (planarconfig != 1) error("Sorry : only planarconfig = 1 is supported");
        if (width%2 || height%2) error ("Sorry : only even dimensions for input images are supported");

        return;
    }
    
    if( ! TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &_width)                         ||
        ! TIFFGetField(image, TIFFTAG_IMAGELENGTH, &_height)                       ||
        ! TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &_bitspersample)              ||
        ! TIFFGetFieldDefaulted(image, TIFFTAG_PLANARCONFIG, &_planarconfig)       ||
        ! TIFFGetFieldDefaulted(image, TIFFTAG_SAMPLESPERPIXEL, &_samplesperpixel) ||
        ! TIFFGetField(image, TIFFTAG_PHOTOMETRIC, &_photometric)                  ||
        ! TIFFGetFieldDefaulted(image, TIFFTAG_SAMPLEFORMAT, &_sampleformat) )
        error(std::string("Error reading file ") + TIFFFileName(image));

    if (isMask) {
        if (! (_width == width && _height == height && _bitspersample == 8 && _planarconfig == planarconfig &&
                _photometric == PHOTOMETRIC_MINISBLACK && _samplesperpixel == 1)) {
            error(std::string("Error : all input masks must have the same parameters (width, height, etc...) : ")
                + TIFFFileName(image));
        }
    } else {
        if (! (_width == width && _height == height && _bitspersample == bitspersample &&
                _planarconfig == planarconfig && _photometric == photometric && _samplesperpixel == samplesperpixel)) {
            
            error(std::string("Error : all input images must have the same parameters (width, height, etc...) : ")
                + TIFFFileName(image));
        }
    }
}

void checkImages(TIFF* INPUTI[2][2],TIFF* INPUTM[2][2],
                 TIFF*& BGI,TIFF*& BGM,
                 TIFF*& OUTPUTI,TIFF*& OUTPUTM)
{
    width=0;    

    for(int i = 0; i < 4; i++) {
        INPUTM[i/2][i%2] = NULL;
        if (inputImages[i] == 0){
            INPUTI[i/2][i%2] = NULL;
            continue;
        }
        
        TIFF *inputi = TIFFOpen(inputImages[i], "r");
        if(inputi == NULL) error("Unable to open input image: " + std::string(inputImages[i]));
        INPUTI[i/2][i%2] = inputi;

        checkComponents(inputi, false);

        if (inputMasks[i] != 0){
            TIFF *inputm = TIFFOpen(inputMasks[i], "r");
            if(inputm == NULL) error("Unable to open input mask: " + std::string(inputMasks[i]));
            INPUTM[i/2][i%2] = inputm;

            checkComponents(inputm, true);
        }
        
    }

    BGI = 0;
    BGM = 0;

    // Si on a quatre image et pas de masque (images considérées comme pleines), le fond est inutile
    if (inputImages[0] && inputImages[1] && inputImages[2] && inputImages[3] &&
        ! inputMasks[0] && ! inputMasks[1] && ! inputMasks[2] && ! inputMasks[3])
        backgroundImage=0;

    if (backgroundImage) {
        BGI=TIFFOpen(backgroundImage, "r");
        if (BGI == NULL) error("Unable to open background image: " + std::string(backgroundImage));
        checkComponents(BGI,false);
        
        if (backgroundMask) {
            BGM = TIFFOpen(backgroundMask, "r");
            if (BGM == NULL) error("Unable to open background mask: " + std::string(backgroundMask));
            checkComponents(BGM,true);
        }
    }

    OUTPUTI = 0;
    OUTPUTM = 0;

    OUTPUTI = TIFFOpen(outputImage, "w");
    if(OUTPUTI == NULL) error("Unable to open output image: " + std::string(outputImage));
    if(! TIFFSetField(OUTPUTI, TIFFTAG_IMAGEWIDTH, width) ||
         ! TIFFSetField(OUTPUTI, TIFFTAG_IMAGELENGTH, height) ||
         ! TIFFSetField(OUTPUTI, TIFFTAG_BITSPERSAMPLE, bitspersample) ||
         ! TIFFSetField(OUTPUTI, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel) ||
         ! TIFFSetField(OUTPUTI, TIFFTAG_PHOTOMETRIC, photometric) ||
         ! TIFFSetField(OUTPUTI, TIFFTAG_ROWSPERSTRIP, rowsperstrip) ||
         ! TIFFSetField(OUTPUTI, TIFFTAG_PLANARCONFIG, planarconfig) ||
         ! TIFFSetField(OUTPUTI, TIFFTAG_COMPRESSION, compression) ||
         ! TIFFSetField(OUTPUTI, TIFFTAG_SAMPLEFORMAT, sampleformat))
        error("Error writting output image: " + std::string(outputImage));

    if (outputMask) {
        OUTPUTM = TIFFOpen(outputMask, "w");
        if(OUTPUTM == NULL) error("Unable to open output mask: " + std::string(outputImage));
        if(! TIFFSetField(OUTPUTM, TIFFTAG_IMAGEWIDTH, width) ||
             ! TIFFSetField(OUTPUTM, TIFFTAG_IMAGELENGTH, height) ||
             ! TIFFSetField(OUTPUTM, TIFFTAG_BITSPERSAMPLE, 8) ||
             ! TIFFSetField(OUTPUTM, TIFFTAG_SAMPLESPERPIXEL, 1) ||
             ! TIFFSetField(OUTPUTM, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK) ||
             ! TIFFSetField(OUTPUTM, TIFFTAG_ROWSPERSTRIP, rowsperstrip) ||
             ! TIFFSetField(OUTPUTM, TIFFTAG_PLANARCONFIG, planarconfig) ||
             ! TIFFSetField(OUTPUTM, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS) ||
             ! TIFFSetField(OUTPUTM, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT))
            error("Error writting output mask: " + std::string(outputMask));
    }
}


int fillLine(float* image, TIFF* IMAGE, uint8_t* mask, TIFF* MASK, int line, int width) {

    if (TIFFReadScanline(IMAGE, image,line) == -1) return 1;
    
    if (MASK) {
        if (TIFFReadScanline(MASK, mask,line) == -1) return 1;
        for (int w = 0; w < width; w++) {
            if (mask[w] < 127) {
                memcpy(image + w*samplesperpixel,nodataFloat32,samplesperpixel*sizeof(float));
            }
        }
    } else {
        memset(mask,255,width);
    }

    return 0;
}

int merge4float32(TIFF* BGI, TIFF* BGM, TIFF* INPUTI[2][2], TIFF* INPUTM[2][2], TIFF* OUTPUTI, TIFF* OUTPUTM) {
int nbsamples = width * samplesperpixel;
    int left,right;

    float line_bgI[nbsamples];
    uint8_t line_bgM[width];

    int nbData;
    float pix[samplesperpixel];

    float line_1I[2*nbsamples];
    uint8_t line_1M[2*width];

    float line_2I[2*nbsamples];
    uint8_t line_2M[2*width];

    float line_outI[nbsamples];
    uint8_t line_outM[width];

    for (int i = 0; i < nbsamples ; i++) {
        line_bgI[i] = nodataFloat32[i%samplesperpixel];
    }
    memset(line_bgM,0,width);

    for(int y = 0; y < 2; y++){
        if (INPUTI[y][0]) left = 0; else left = width;
        if (INPUTI[y][1]) right = 2*width; else right = width;

        for(uint32 h = 0; h < height/2; h++) {

            int line = y*height/2 + h;

            // ------------ le fond -----------
            if (BGI)
                if (fillLine(line_bgI,BGI,line_bgM,BGM,line,width))
                    error("Unable to read background line");

            if (left == right) {
                // On n'a pas d'image en entrée pour cette ligne, on stocke le fond et on passe à la suivante
                if (TIFFWriteScanline(OUTPUTI, line_bgI, line) == -1) error("Unable to write image");
                if (OUTPUTM) if (TIFFWriteScanline(OUTPUTM, line_bgM, line) == -1) error("Unable to write mask");
                continue;
            }

            memcpy(line_outI,line_bgI,sizeof(float)*nbsamples);
            memcpy(line_outM,line_bgM,width);

            // ---------- les images ----------
            if (INPUTI[y][0]) {
                if (TIFFReadScanline(INPUTI[y][0], line_1I, 2*h) == -1) error("Unable to read data line");
                if (TIFFReadScanline(INPUTI[y][0], line_2I, 2*h+1) == -1) error("Unable to read data line");
            }

            if (INPUTI[y][1]) {
                if (TIFFReadScanline(INPUTI[y][1], line_1I + nbsamples, 2*h) == -1) error("Unable to read data line");
                if (TIFFReadScanline(INPUTI[y][1], line_2I + nbsamples, 2*h+1) == -1) error("Unable to read data line");
            }

            // ---------- les masques ---------
            memset(line_1M,255,2*width);
            memset(line_2M,255,2*width);

            if (INPUTM[y][0]) {
                if (TIFFReadScanline(INPUTM[y][0], line_1M, 2*h) == -1) error("Unable to read data line");
                if (TIFFReadScanline(INPUTM[y][0], line_2M, 2*h+1) == -1) error("Unable to read data line");
            }

            if (INPUTM[y][1]) {
                if (TIFFReadScanline(INPUTM[y][1], line_1M + width, 2*h) == -1) error("Unable to read data line");
                if (TIFFReadScanline(INPUTM[y][1], line_2M + width, 2*h+1) == -1) error("Unable to read data line");
            }

            // ---------- la moyenne ---------
            for (int pixIn = left, sampleIn = left * samplesperpixel; pixIn < right;
                 pixIn += 2, sampleIn += 2*samplesperpixel) {

                memset(pix,0,samplesperpixel*sizeof(float));
                nbData = 0;

                if (line_1M[pixIn] >= 127) {
                    nbData++;
                    for (int c = 0; c < samplesperpixel; c++) pix[c] += line_1I[sampleIn+c];
                }

                if (line_1M[pixIn+1] >= 127) {
                    nbData++;
                    for (int c = 0; c < samplesperpixel; c++) pix[c] += line_1I[sampleIn+samplesperpixel+c];
                }

                if (line_2M[pixIn] >= 127) {
                    nbData++;
                    for (int c = 0; c < samplesperpixel; c++) pix[c] += line_2I[sampleIn+c];
                }

                if (line_2M[pixIn+1] >= 127) {
                    nbData++;
                    for (int c = 0; c < samplesperpixel; c++) pix[c] += line_2I[sampleIn+samplesperpixel+c];
                }

                if (nbData > 1) {
                    line_outM[pixIn/2] = 255;
                    for (int c = 0; c < samplesperpixel; c++) line_outI[sampleIn/2+c] = pix[c]/(float)nbData;
                }
            }

            if(TIFFWriteScanline(OUTPUTI, line_outI, line) == -1) error("Unable to write image");
            if (OUTPUTM) if(TIFFWriteScanline(OUTPUTM, line_outM, line) == -1) error("Unable to write mask");

        }
    }

    if (BGI) TIFFClose(BGI);
    if (BGM) TIFFClose(BGM);

    for(int i = 0; i < 2; i++) for(int j = 0; j < 2; j++) {
        if (INPUTI[i][j]) TIFFClose(INPUTI[i][j]);
        if (INPUTM[i][j]) TIFFClose(INPUTM[i][j]);
    }

    TIFFClose(OUTPUTI);
    if (OUTPUTM) TIFFClose(OUTPUTM);

    return 0;
};

int fillLine(uint8_t* image, TIFF* IMAGE, uint8_t* mask, TIFF* MASK, int line, int width) {

    if (TIFFReadScanline(IMAGE, image,line) == -1) return 1;

    if (MASK) {
        if (TIFFReadScanline(MASK, mask,line) == -1) return 1;
        for (int w = 0; w < width; w++) {
            if (mask[w] < 127) {
                memcpy(image + w*samplesperpixel,nodataUInt8,samplesperpixel);
            }
        }
    } else {
        memset(mask,255,width);
    }

    return 0;
}

int merge4uint8(TIFF* BGI, TIFF* BGM, TIFF* INPUTI[2][2], TIFF* INPUTM[2][2], TIFF* OUTPUTI, TIFF* OUTPUTM)
{    
    uint8 MERGE[1024];
    for(int i = 0; i <= 1020; i++) MERGE[i] = 255 - (uint8) round(pow(double(1020 - i)/1020., gammaM4t) * 255.);

    int nbsamples = width * samplesperpixel;
    int left,right;

    uint8_t line_bgI[nbsamples];
    uint8_t line_bgM[width];

    int nbData;
    int pix[samplesperpixel];

    uint8_t line_1I[2*nbsamples];
    uint8_t line_1M[2*width];

    uint8_t line_2I[2*nbsamples];
    uint8_t line_2M[2*width];

    uint8_t line_outI[nbsamples];
    uint8_t line_outM[width];

    // ----------- initialisation du fond -----------
    for (int i = 0; i < nbsamples ; i++) {
        line_bgI[i] = nodataUInt8[i%samplesperpixel];
    }
    memset(line_bgM,0,width);

    for(int y = 0; y < 2; y++){
        if (INPUTI[y][0]) left = 0; else left = width;
        if (INPUTI[y][1]) right = 2*width; else right = width;

        for(uint32 h = 0; h < height/2; h++) {

            int line = y*height/2 + h;

            // ------------------- le fond ------------------
            if (BGI)
                if (fillLine(line_bgI,BGI,line_bgM,BGM,line,width))
                    error("Unable to read background line");

            if (left == right) {
                // On n'a pas d'image en entrée pour cette ligne, on stocke le fond et on passe à la suivante
                if (TIFFWriteScanline(OUTPUTI, line_bgI, line) == -1) error("Unable to write image");
                if (OUTPUTM) if (TIFFWriteScanline(OUTPUTM, line_bgM, line) == -1) error("Unable to write mask");

                continue;
            }

            // -- initialisation de la sortie avec le fond --
            memcpy(line_outI,line_bgI,nbsamples);
            memcpy(line_outM,line_bgM,width);

            // ----------------- les images -----------------
            if (INPUTI[y][0]) {
                if (TIFFReadScanline(INPUTI[y][0], line_1I, 2*h) == -1)
                    error("Unable to read data line");
                if (TIFFReadScanline(INPUTI[y][0], line_2I, 2*h+1) == -1)
                    error("Unable to read data line");
            }


            if (INPUTI[y][1]) {
                if (TIFFReadScanline(INPUTI[y][1], line_1I + nbsamples, 2*h) == -1)
                    error("Unable to read data line");
                if (TIFFReadScanline(INPUTI[y][1], line_2I + nbsamples, 2*h+1) == -1)
                    error("Unable to read data line");
            }

            // ----------------- les masques ----------------
            memset(line_1M,255,2*width);
            memset(line_2M,255,2*width);

            if (INPUTM[y][0]) {
                if (TIFFReadScanline(INPUTM[y][0], line_1M, 2*h) == -1) error("Unable to read data line");
                if (TIFFReadScanline(INPUTM[y][0], line_2M, 2*h+1) == -1) error("Unable to read data line");
            }

            if (INPUTM[y][1]) {
                if (TIFFReadScanline(INPUTM[y][1], line_1M + width, 2*h) == -1) error("Unable to read data line");
                if (TIFFReadScanline(INPUTM[y][1], line_2M + width, 2*h+1) == -1) error("Unable to read data line");
            }

            // ----------------- la moyenne ----------------
            for (int pixIn = left, sampleIn = left * samplesperpixel; pixIn < right;
                 pixIn += 2, sampleIn += 2*samplesperpixel) {

                memset(pix,0,samplesperpixel*sizeof(int));
                nbData = 0;

                if (line_1M[pixIn] >= 127) {
                    nbData++;
                    for (int c = 0; c < samplesperpixel; c++) pix[c] += line_1I[sampleIn+c];
                }

                if (line_1M[pixIn+1] >= 127) {
                    nbData++;
                    for (int c = 0; c < samplesperpixel; c++) pix[c] += line_1I[sampleIn+samplesperpixel+c];
                }

                if (line_2M[pixIn] >= 127) {
                    nbData++;
                    for (int c = 0; c < samplesperpixel; c++) pix[c] += line_2I[sampleIn+c];
                }

                if (line_2M[pixIn+1] >= 127) {
                    nbData++;
                    for (int c = 0; c < samplesperpixel; c++) pix[c] += line_2I[sampleIn+samplesperpixel+c];
                }

                if (nbData > 1) {
                    line_outM[pixIn/2] = 255;
                    for (int c = 0; c < samplesperpixel; c++) line_outI[sampleIn/2+c] = MERGE[pix[c]*4/nbData];
                }
            }
            
            if(TIFFWriteScanline(OUTPUTI, line_outI, line) == -1) error("Unable to write image");
            if (OUTPUTM) if(TIFFWriteScanline(OUTPUTM, line_outM, line) == -1) error("Unable to write mask");
        }
    }
    
    if (BGI) TIFFClose(BGI);
    if (BGM) TIFFClose(BGM);
    
    for(int i = 0; i < 2; i++) for(int j = 0; j < 2; j++) {
        if (INPUTI[i][j]) TIFFClose(INPUTI[i][j]);
        if (INPUTM[i][j]) TIFFClose(INPUTM[i][j]);
    }
    
    TIFFClose(OUTPUTI);
    if (OUTPUTM) TIFFClose(OUTPUTM);
            
    return 0;
}


int main(int argc, char* argv[])
{
    TIFF* INPUTI[2][2];
    TIFF* INPUTM[2][2];
    TIFF* BGI;
    TIFF* BGM;
    TIFF* OUTPUTI;
    TIFF* OUTPUTM;

    parseCommandLine(argc, argv);
    
    checkImages(INPUTI,INPUTM,BGI,BGM,OUTPUTI,OUTPUTM);
    
    if (! ((bitspersample == 32 && sampleformat == SAMPLEFORMAT_IEEEFP) || 
        (bitspersample == 8 && sampleformat == SAMPLEFORMAT_UINT)) ){
        error("sampleformat/bitspersample not supported");
    }
    
    // Nodata interpretation
    int nodata[samplesperpixel];
    char* charValue = strtok(strnodata,",");
    if(charValue == NULL) {
        error("Error with option -n : a value for nodata is missing");
    }
    nodata[0] = atoi(charValue);
    for(int i = 1; i < samplesperpixel; i++) {
        charValue = strtok (NULL, ",");
        if(charValue == NULL) {
            error("Error with option -n : a value for nodata is missing");
        }
        nodata[i] = atoi(charValue);
    }
    
    // Cas MNT
    if (sampleformat == SAMPLEFORMAT_IEEEFP && bitspersample == 32) {
        nodataFloat32 = new float[samplesperpixel];
        for(int i = 0; i < samplesperpixel; i++) nodataFloat32[i] = (float) nodata[i];
        return merge4float32(BGI,BGM,INPUTI,INPUTM,OUTPUTI,OUTPUTM);
    }
    // Cas images
    else if (sampleformat == SAMPLEFORMAT_UINT && bitspersample == 8) {
        nodataUInt8 = new uint8_t[samplesperpixel];
        for(int i = 0; i < samplesperpixel; i++) nodataUInt8[i] = (uint8_t) nodata[i];
        return merge4uint8(BGI,BGM,INPUTI,INPUTM,OUTPUTI,OUTPUTM);
    }
}

