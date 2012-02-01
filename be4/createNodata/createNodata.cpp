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
 * \file createNodata.cpp
 * \brief création d'une image avec une seule valeur, pour la tuile de nodata
 * \author IGN
*
*/

#include "TiledTiffWriter.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "tiffio.h"
#include <jpeglib.h>

void usage() {
    std::cerr << "createNodata -n nodata -c [none/png/jpeg/lzw] -p [gray/rgb] -t [sizex] [sizey] -b [8/32] -a [uint/float] -s [1/3] output_file"<< std::endl;
}

void error(std::string message) {
    std::cerr << message << std::endl;
    usage();
    exit(1);
}


/**
*@fn int h2i(char s)
* Hexadecimal -> int
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

int main(int argc, char* argv[]) {
    char* output = 0;
    uint32_t imagewidth = 256, imageheight = 256;
    uint16_t compression = COMPRESSION_NONE;
    uint16_t photometric = PHOTOMETRIC_RGB;
    char* strnodata;
    int nodata;
    uint32_t bitspersample = 8;
    uint16_t sampleformat = SAMPLEFORMAT_UINT; // Autre possibilite : SAMPLEFORMAT_IEEEFP
    uint16_t sampleperpixel = 3;
    int quality = -1;
    
//  Nodata image dimensions are precised in parameters and image contains a single tile

    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'c': // compression
                    if(++i == argc) {std::cerr << "Error in -c option" << std::endl; exit(2);}
                    if(strncmp(argv[i], "none",4) == 0) compression = COMPRESSION_NONE;
                    else if(strncmp(argv[i], "png",3) == 0) {
                        compression = COMPRESSION_PNG;
                        if(argv[i][3] == ':') quality = atoi(argv[i]+4);
                    }
                    else if(strncmp(argv[i], "jpeg",4) == 0) {
                        compression = COMPRESSION_JPEG;
                        if(argv[i][4] == ':') quality = atoi(argv[i]+5);
                    }
                    else if(strncmp(argv[i], "lzw",3) == 0) {
                        compression = COMPRESSION_LZW;
                    }
                    else compression = COMPRESSION_NONE;
                    break;
                case 'n': // nodata
                    if(++i == argc) error("missing parameter in -n argument");
                    strnodata = argv[i];
                    break;
                case 'p': // photometric
                    if(++i == argc) {std::cerr << "Error in -p option" << std::endl; exit(2);}          
                    if(strncmp(argv[i], "gray",4) == 0) photometric = PHOTOMETRIC_MINISBLACK;
                    else if(strncmp(argv[i], "rgb",3) == 0) photometric = PHOTOMETRIC_RGB;
                    else photometric = PHOTOMETRIC_RGB;
                    break;
                case 't': // dimension de la tuile de nodata
                    if(i+2 >= argc) {std::cerr << "Error in -t option" << std::endl; exit(2);}
                    imagewidth = atoi(argv[++i]);
                    imageheight = atoi(argv[++i]);
                    break;
                case 'a':
                    if(++i == argc) {std::cerr << "Error in -a option" << std::endl; exit(2);}
                    if (strncmp(argv[i],"uint",4)==0) {sampleformat = SAMPLEFORMAT_UINT;}
                    else if (strncmp(argv[i],"float",5)==0) {sampleformat = SAMPLEFORMAT_IEEEFP;}
                    break;
                case 'b':
                    if(i+1 >= argc) {std::cerr << "Error in -b option" << std::endl; exit(2);}
                    bitspersample = atoi(argv[++i]);
                    break;
                case 's':
                    if(i+1 >= argc) {std::cerr << "Error in -s option" << std::endl; exit(2);}
                    sampleperpixel = atoi(argv[++i]);
                    break;
                default: usage();
            }
        }
        else {
            if(output == 0) output = argv[i];
            else {std::cerr << "argument must specify one output file" << std::endl; usage(); exit(2);}
        }
    }

    if(output == 0) {std::cerr << "argument must specify one output file" << std::endl; exit(2);}
    if(photometric == PHOTOMETRIC_MINISBLACK && compression == COMPRESSION_JPEG) {std::cerr << "Gray jpeg not supported" << std::endl; exit(2);}

    // nodata treatment
    // input data creation : the same value (nodata) everywhere
    int bytesperpixel = sampleperpixel*bitspersample/8;
    uint8_t data[imageheight*imagewidth*bytesperpixel], *pdata = data;
    
    // Case float32
    if (sampleformat == SAMPLEFORMAT_IEEEFP && bitspersample == 32) {
        if (strnodata != 0) {
            nodata = atoi(strnodata);
            if (nodata == 0 && strcmp(strnodata,"0")!=0) error("invalid nodata value for this sampleformat/bitspersample couple : it must be a decimal format number");
        } else {
            nodata = -99999;
        }
        
        for (int i = 0; i<imageheight*imagewidth*sampleperpixel; i++) {
            *((float*) (pdata)) = (float) nodata;
            pdata += 4;
        }
    }
    // Case int8
    else if (sampleformat == SAMPLEFORMAT_UINT && bitspersample == 8) {
        if (strnodata != 0) {
            int a1 = h2i(strnodata[0]);
            int a0 = h2i(strnodata[1]);
            if (a1 < 0 || a0 < 0) error("invalid nodata value for this sampleformat/bitspersample couple : it must be hexadecimal format number");
            nodata = 16*a1+a0;
        } else {
            nodata = 255;
        }
        
        for (int i = 0; i<imageheight*imagewidth*sampleperpixel; i++) {
            *((uint8_t*) (pdata)) = nodata;
            pdata++;
        }
    }
    else
        error("sampleformat/bitspersample not supported (float/32 or uint/8)");
        
    TiledTiffWriter W(output, imagewidth, imageheight, photometric, compression, quality, imagewidth, imageheight,bitspersample,sampleformat);


    if(W.WriteTile(0, 0, data) < 0) {std::cerr << "Error while writting tile of nodata" << std::endl; return 2;}

    if(W.close() < 0) {std::cerr << "Error while writting index" << std::endl; return 2;}
    
    return 0;
}

