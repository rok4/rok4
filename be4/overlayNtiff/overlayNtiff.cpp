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
 * \file overlayNtiff.cpp
 * \brief Superposition de n images avec transparence
 * \author IGN
 *
 * Ce programme est destine a etre utilise dans la chaine de generation de cache joinCache.
 * Il est appele pour calculer les dalles avec plusieurs sources.
 *
 * Parametres d'entree :
 * 1. Une couleur qui sera considérée comme transparente
 * 2. Une couleur qui sera utilisée si on veut supprimer le canal alpha
 * 3. Le nombre de canaux de l'image de sortie
 * 4. Une liste d'images source à superposer
 * 5. Un fichier de sortie
 *
 * En sortie, un fichier TIFF au format dit de travail brut non compressé entrelace sur 1,3 ou 4 canaux entiers.
 *
 * Toutes les images ont la même taille et sont sur 4 canaux entiers.
 * Le canal alpha est considéré comme l'opacité (0 = transparent)
 */

#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include "tiffio.h"
#include "tiff.h"
#include "Logger.h"
#include "LibtiffImage.h"
#include "math.h"
#include "../be4version.h"

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

#define     COMPOSEMETHOD_TRANSPARENCY  1
#define     COMPOSEMETHOD_MULTIPLY      2

/**
* @fn void usage()
* Usage de la ligne de commande
*/
void usage() {
    LOGGER_INFO("overlayNtiff version "<< BE4_VERSION);
    LOGGER_INFO(" Usage : overlayNtiff -mode multiply -transparent 255,255,255 -opaque 0,0,0 -channels 4"
        "-input source1.tif source2.tif source3.tif -output result.tif\n"
        "-mode : transparency/multiply\n"
        "-transparent : color which will be considered to be transparent\n"
        "-opaque : background color in case we have to remove alpha channel\n"
        "-channels : precise number of samples per pixel in the output image\n"
        "-input : list of source images\n"
        "-output : output file path\n");
}

/**
* @fn void error()
* Sortie d'erreur de la commande
*/
void error(std::string message) {
    LOGGER_ERROR(message);
    exit(1);
}

uint8_t transparent[3];
int outputChannels = 0;
int composeMethod = 0;
uint8_t opaque[4];
std::vector<char*> input;
char* output = 0;

/**
* @fn parseCommandLine(int argc, char** argv)
* Lecture des parametres de la ligne de commande
*/

int parseCommandLine(int argc, char** argv) {

    char* strTransparent;
    char* strOpaque;

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i],"-transparent")) {
            if(i++ >= argc) error("Error with option -transparent");
            strTransparent = argv[i];
            continue;
        }
        else if(!strcmp(argv[i],"-mode")) {
            if(i++ >= argc) error("Error with option -mode");
            if(strncmp(argv[i], "multiply",8) == 0) composeMethod = COMPOSEMETHOD_MULTIPLY; // = 2
            else if(strncmp(argv[i], "transparency",12) == 0) composeMethod = COMPOSEMETHOD_TRANSPARENCY; // = 1
            else error("Error with option -mode ");
            continue;
        }
        else if(!strcmp(argv[i],"-channels")) {
            if(i++ >= argc) error("Error with option -channels");
            outputChannels = atoi(argv[i]);
            continue;
        }
        else if(!strcmp(argv[i],"-opaque")) {
            if(i++ >= argc) error("Error with option -opaque");
            strOpaque = argv[i];
            continue;
        }
        else if (!strcmp(argv[i],"-input")) {
            if(i++ >= argc) error("Error with option -input");
            while (argv[i][0] != '-') {
                input.push_back(argv[i]);
                i++;
            }
            i--;
            continue;
        }
        else if (!strcmp(argv[i],"-output")) {
            if(i++ >= argc) error("Error with option -output");
            output = argv[i];
            continue;
        }
        else {
            usage();
            return -1;
        }
    }
    
    // Mode control
    if (composeMethod == 0) {
        error("We need to know the compoistion method");
    }
    
    // Input/output control
    if (input.size() == 0 || !output) {
        LOGGER_ERROR("We need to have one or more sources and one output");
        return -1;
    }
    
    // Channels control
    if (outputChannels != 1 && outputChannels != 3 && outputChannels != 4) {
        LOGGER_ERROR("Samples per pixel can be 1,3 or 4.");
        return -1;
    }
    
    // Transparent interpretation
    char* charValue = strtok(strTransparent,",");
    if(charValue == NULL) {
        LOGGER_ERROR("Error with option -transparent : 3 integer values (between 0 and 255) seperated by comma");
        return -1;
    }
    int value = atoi(charValue);
    if(value < 0 || value > 255) {
        LOGGER_ERROR("Error with option -transparent : 3 integer values (between 0 and 255) seperated by comma");
        return -1;
    }
    transparent[0] = value;
    for(int i = 1; i < 3; i++) {
        charValue = strtok (NULL, ",");
        if(charValue == NULL) {
            LOGGER_ERROR("Error with option -transparent : 3 integer values (between 0 and 255) seperated by comma");
            return -1;
        }
        value = atoi(charValue);
        if(value < 0 || value > 255) {
            LOGGER_ERROR("Error with option -transparent : 3 integer values (between 0 and 255) seperated by comma");
            return -1;
        }
        transparent[i] = value;
    }
    
    // Opaque interpretation
    opaque[3] = 255;
    charValue = strtok(strOpaque,",");
    if(charValue == NULL) {
        LOGGER_ERROR("Error with option -opaque : 3 integer values (between 0 and 255) seperated by comma");
        return -1;
    }
    value = atoi(charValue);
    if(value < 0 || value > 255) {
        LOGGER_ERROR("Error with option -opaque : 3 integer values (between 0 and 255) seperated by comma");
        return -1;
    }
    opaque[0] = value;
    for(int i = 1; i < 3; i++) {
        charValue = strtok (NULL, ",");
        if(charValue == NULL) {
            LOGGER_ERROR("Error with option -opaque : 3 integer values (between 0 and 255) seperated by comma");
            return -1;
        }
        value = atoi(charValue);
        if(value < 0 || value > 255) {
            LOGGER_ERROR("Error with option -opaque : 3 integer values (between 0 and 255) seperated by comma");
            return -1;
        }
        opaque[i] = value;
    }
    
    return 0;
}

/**
* @fn int compose(uint8_t PixA,uint8_t PixB)
* @brief Calcule la valeur du pixel (4 canaux) résultant de la superposition des 2 pixels passés en paramètres
*/
void compose(uint8_t* PixA,uint8_t* PixB,uint8_t* PixR, bool rmAlpha) {

    if (composeMethod == COMPOSEMETHOD_TRANSPARENCY || rmAlpha) {
        PixR[3] = PixA[3] + PixB[3]*(255-PixA[3])/255;
        PixR[0] = (PixA[0]*PixA[3] + PixB[0]*PixB[3]*(255-PixA[3])/255)/PixR[3];
        PixR[1] = (PixA[1]*PixA[3] + PixB[1]*PixB[3]*(255-PixA[3])/255)/PixR[3];
        PixR[2] = (PixA[2]*PixA[3] + PixB[2]*PixB[3]*(255-PixA[3])/255)/PixR[3];
    }
    else if (composeMethod == COMPOSEMETHOD_MULTIPLY) {
        PixR[0] = PixA[0]*PixB[0]/255;
        PixR[1] = PixA[1]*PixB[1]/255;
        PixR[2] = PixA[2]*PixB[2]/255;
        PixR[3] = PixA[3]*PixB[3]/255;
    }

}

/**
* @fn int treatImages()
* @brief Controle des images et lecture du contenu
*/

int treatImages()
{
    
    TIFF *TIFF_FILE = 0;

    uint32 width, height, rowsperstrip = -1;
    uint16 bitspersample, sampleperpixel, photometric, compression = -1, planarconfig;
    
    // FIRST IMAGE READING
    // we determine image's characteristics from the first image
    TIFF_FILE = TIFFOpen(input.at(0), "r");
    if(!TIFF_FILE) error("Unable to open file for reading: " + std::string(input.at(0)));
    if( ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, &width)                       ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, &height)                     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, &photometric)                ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel)||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_COMPRESSION, &compression)                ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, &rowsperstrip))
    {    
        error("Error reading file: " +  std::string(input.at(0)));
    }

    if (planarconfig != 1) error("Sorry : only planarconfig = 1 is supported");
    if (sampleperpixel != 4) error("Sorry : only sampleperpixel = 4 is supported");
    if (bitspersample != 8) error("Sorry : only bitspersample = 8 is supported");
    if (compression != 1) error("Sorry : compression not accepted");
    
    uint8_t *IM = new uint8_t[width * height * 4];
    uint8_t *LINE = new uint8_t[width * 4];
    uint8_t *PIXEL = new uint8_t[4];
    
    for(int h = 0; h < height; h++) {
        if(TIFFReadScanline(TIFF_FILE,LINE, h) == -1) error("Unable to read data");
        for(int w = 0; w < width; w++) {
            if (! memcmp(LINE+w*4,&transparent,3)) {
                memset(LINE+w*4,0,4);
            }
        }
        memcpy(&IM[h*width*4],LINE,width*4);
    }
    TIFFClose(TIFF_FILE);
    
    // FOLLOWING IMAGE READING
    for (int i = 1; i < input.size(); i++) {
        
        uint32 widthp, heightp;
        uint16 bitspersamplep, sampleperpixelp, compressionp = -1, planarconfigp;
        
        TIFF_FILE = TIFFOpen(input.at(i), "r");
        if(!TIFF_FILE) error("Unable to open file for reading: " + std::string(input.at(i)));
        if( ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, &widthp)                       ||
            ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, &heightp)                     ||
            ! TIFFGetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, &bitspersamplep)            ||
            ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_PLANARCONFIG, &planarconfigp)     ||
            ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixelp)||
            ! TIFFGetField(TIFF_FILE, TIFFTAG_COMPRESSION, &compressionp))
        {    
            error("Error reading file: " +  std::string(input.at(i)));
        }

        if (planarconfigp != 1) error("Sorry : only planarconfig = 1 is supported (" + std::string(input.at(i)) +")");
        if (sampleperpixelp != 4) error("Sorry : only sampleperpixel = 4 is supported (" + std::string(input.at(i)) +")");
        if (bitspersamplep != 8) error("Sorry : only bitspersample = 8 is supported (" + std::string(input.at(i)) +")");
        if (compressionp != 1) error("Sorry : compression not accepted (" + std::string(input.at(i)) +")");
        
        if (widthp != width || heightp != height) error("All images must have same size (width and height)");
        
        for(int h = 0; h < height; h++) {
            if(TIFFReadScanline(TIFF_FILE,LINE, h) == -1) error("Unable to read data");
            for(int w = 0; w < width; w++) {
                if ( LINE[w*4+3] == 0 || ! memcmp(LINE+w*4,&transparent,3)) {
                    continue;
                }
                compose(LINE+w*4,IM+(h*width+w)*4,PIXEL,false);
                memcpy(&IM[h*width*4+w*4],PIXEL,4);
            }
        }
        TIFFClose(TIFF_FILE);
    }
    
    // OUTPUT IMAGE WRITTING
    uint16_t extrasample = EXTRASAMPLE_ASSOCALPHA;
    TIFF_FILE = TIFFOpen(output, "w");
    if(!TIFF_FILE) error("Unable to open file for writting: " + std::string(output));
    
    if( ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, width)               ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, height)             ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, 8)                ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, outputChannels) ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_ROWSPERSTRIP, rowsperstrip)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_PLANARCONFIG, planarconfig)      ||
        ! TIFFSetField(TIFF_FILE, TIFFTAG_COMPRESSION, compression))
            error("Error writting file: " +  std::string(output));
    
    if (outputChannels == 4) {
        if (! TIFFSetField(TIFF_FILE, TIFFTAG_EXTRASAMPLES,1,&extrasample))
            error("Error writting file: " +  std::string(output));
    }
    
    
    if (outputChannels == 4) {
        if (! TIFFSetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB) ||
            ! TIFFSetField(TIFF_FILE, TIFFTAG_EXTRASAMPLES,1,&extrasample))
            error("Error writting file: " +  std::string(output));
        
        for(int h = 0; h < height; h++) {
            memcpy(LINE, IM+h*width*outputChannels, width * outputChannels);
            if(TIFFWriteScanline(TIFF_FILE, LINE, h) == -1) error("Unable to write line to " + std::string(output));
        }
    }
    else if (outputChannels == 3) {
        if (!TIFFSetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB))
            error("Error writting file: " +  std::string(output));
        
        for(int h = 0; h < height; h++) {
            for(int i = 0; i < width; i++) {
                compose(IM+h*width*4+i*4,opaque,PIXEL,true);
                memcpy(LINE+i*3,PIXEL,3);
            }
            if(TIFFWriteScanline(TIFF_FILE, LINE, h) == -1) error("Unable to write line to " + std::string(output));
        }
    }
    else {
        if (!TIFFSetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK))
            error("Error writting file: " +  std::string(output));

        for(int h = 0; h < height; h++) {
            for(int i = 0; i < width; i++) {
                compose(IM+h*width*4+i*4,opaque,PIXEL,true);
                LINE[i] = 0.2125*PIXEL[0] + 0.7154*PIXEL[1] + 0.0721*PIXEL[2];
            }
            if(TIFFWriteScanline(TIFF_FILE, LINE, h) == -1) error("Unable to write line to " + std::string(output));
        }
        
    }

    TIFFClose(TIFF_FILE);
    
    delete[] IM;
    delete[] LINE;
    delete[] PIXEL;
    
    return 0;
}



/**
* @fn int main(int argc, char **argv)
* @brief Fonction principale
*/

int main(int argc, char **argv) {

    /* Initialisation des Loggers */
    Logger::setOutput(STANDARD_OUTPUT_STREAM_FOR_ERRORS);

    Accumulator* acc = new StreamAccumulator();
    //Logger::setAccumulator(DEBUG, acc);
    Logger::setAccumulator(INFO , acc);
    Logger::setAccumulator(WARN , acc);
    Logger::setAccumulator(ERROR, acc);
    Logger::setAccumulator(FATAL, acc);

    std::ostream &logd = LOGGER(DEBUG);
          logd.precision(16);
    logd.setf(std::ios::fixed,std::ios::floatfield);

    std::ostream &logw = LOGGER(WARN);
    logw.precision(16);
    logw.setf(std::ios::fixed,std::ios::floatfield);

    LOGGER_DEBUG("Read parameters");
    // Lecture des parametres de la ligne de commande
    if (parseCommandLine(argc,argv)) {
        LOGGER_ERROR("Echec lecture ligne de commande");
        sleep(1);
        return -1;
    }

    LOGGER_DEBUG("Treat");
    // Controle des images
    if (treatImages()<0){
        LOGGER_ERROR("Echec controle des images");
        sleep(1);
        return -1;
    }
    
    delete acc;

    return 0;
}
