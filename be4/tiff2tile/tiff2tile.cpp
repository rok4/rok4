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

#include <cstdlib>
#include <iostream>
#include <string.h>
#include "tiffio.h"
#include "TiffReader.h"
#include "TiledTiffWriter.h"
#include "TiffNodataManager.h"
#include "../be4version.h"

uint8_t fastWhite[4] = {254,254,254,255};
uint8_t white[4] = {255,255,255,255};

void usage() {
    std::cerr << "tiff2tile version "<< BE4_VERSION << std::endl;
    std::cerr << "usage : tiff2tile input_file -c [none/raw/png/jpg/lzw/zip/pkb] -p [gray/rgb] -t [sizex] [sizey] -b [8/32] -a [uint/float] output_file" << std::endl;
    std::cerr << "\t-crop : the blocks (used by jpeg compression) which contain a nodata pixel are fill with nodata (to keep stright nodata)" << std::endl;
}

int main(int argc, char **argv) {
    char* input = 0, *output = 0;
    uint32_t tilewidth = 256, tilelength = 256;
    uint16_t compression = COMPRESSION_NONE;
    uint16_t photometric = PHOTOMETRIC_RGB;
    uint32_t bitspersample = 8;
    uint16_t samplesperpixel = 3;
    bool crop = false;
    uint16_t sampleformat = SAMPLEFORMAT_UINT; // Autre possibilite : SAMPLEFORMAT_IEEEFP
    int quality = -1;

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i],"-crop")) {
            crop = true;
            continue;
        }
        if(argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'c': // compression
                    if(++i == argc) {std::cerr << "Error in -c option" << std::endl; exit(2);}
                    if(strncmp(argv[i], "none",4) == 0 || strncmp(argv[i], "raw",3) == 0) compression = COMPRESSION_NONE;
                    else if(strncmp(argv[i], "png",3) == 0) {
                        compression = COMPRESSION_PNG;
                        if(argv[i][3] == ':') quality = atoi(argv[i]+4);
                    }
                    else if(strncmp(argv[i], "jpg",3) == 0) {
                        compression = COMPRESSION_JPEG;
                        if(argv[i][4] == ':') quality = atoi(argv[i]+5);
                    }
                    else if(strncmp(argv[i], "lzw",3) == 0) {
                        compression = COMPRESSION_LZW;
                    }
                    else if(strncmp(argv[i], "zip",3) == 0) {
                        compression = COMPRESSION_DEFLATE;
                    }
                    else if(strncmp(argv[i], "pkb",3) == 0) {
                        compression = COMPRESSION_PACKBITS;
                    }
                    else {
                        std::cerr << "Error : unknown compression ("<< argv[i] <<")." << std::endl;
                        exit(2);
                    }
                    break;
                case 'p': // photometric
                    if(++i == argc) {std::cerr << "Error in -p option" << std::endl; exit(2);}          
                    if(strncmp(argv[i], "gray",4) == 0) photometric = PHOTOMETRIC_MINISBLACK;
                    else if(strncmp(argv[i], "rgb",3) == 0) photometric = PHOTOMETRIC_RGB;
                    else photometric = PHOTOMETRIC_RGB;
                    break;
                case 't':
                    if(i+2 >= argc) {std::cerr << "Error in -t option" << std::endl; exit(2);}
                    tilewidth = atoi(argv[++i]);
                    tilelength = atoi(argv[++i]);
                    break;
                case 'a':
                    if(++i == argc) {std::cerr << "Error in -a option" << std::endl; exit(2);}
                    if (strncmp(argv[i],"uint",4)==0) {sampleformat = SAMPLEFORMAT_UINT;}
                    else if (strncmp(argv[i],"float",5)==0) {sampleformat = SAMPLEFORMAT_IEEEFP;}
                    else {std::cerr << "Error in -a option. Possibilities are uint or float." << std::endl; exit(2);}
                    break;
                case 's': // samplesperpixel
                    if ( ++i == argc ) {std::cerr << "Error in -s option" << std::endl; exit(2);}
                    if ( strncmp ( argv[i], "1",1 ) == 0 ) samplesperpixel = 1 ;
                    else if ( strncmp ( argv[i], "3",1 ) == 0 ) samplesperpixel = 3 ;
                    else if ( strncmp ( argv[i], "4",1 ) == 0 ) samplesperpixel = 4 ;
                    else {std::cerr << "Error in -s option. Possibilities are 1,3 or 4." << std::endl; exit(2);}
                    break;
                case 'b':
                    if(i+1 >= argc) {std::cerr << "Error in -b option" << std::endl; exit(2);}
                    bitspersample = atoi(argv[++i]);
                    break;
                default: usage();
            }
        }        
        else {
            if(input == 0) input = argv[i];
            else if(output == 0) output = argv[i];
            else {
                std::cerr << "Argument must specify one input file and one output file" << std::endl;
                usage();
                exit(2);
            }
        }
    }

    if(output == 0) {
        std::cerr << "Argument must specify one input file and one output file" << std::endl; exit(2);
    }
    if(photometric == PHOTOMETRIC_MINISBLACK && compression == COMPRESSION_JPEG) {
        std::cerr << "Gray jpeg not supported" << std::endl; exit(2);
    }
    
    // For jpeg compression with crop option, we have to remove white pixel, to avoid empty bloc in data
    if (crop) {
        
        TiffNodataManager TNM(samplesperpixel,white,fastWhite,white);

        if (! TNM.treatNodata(input,input)) {
            std::cerr << "Unable to treat white pixels in this image : " << input << std::endl;
            exit(2);
        }
    }

    TiffReader R(input);

    uint32_t width = R.getWidth();
    uint32_t length = R.getLength();  
    TiledTiffWriter W(output, width, length, photometric, compression, quality, tilewidth, tilelength,bitspersample,samplesperpixel,sampleformat);

    if(width % tilewidth || length % tilelength) {
        std::cerr << "Image size must be a multiple of tile size" << std::endl;
        exit(2);
    }  
    int tilex = width / tilewidth;
    int tiley = length / tilelength;
    
    size_t dataSize = tilelength*tilewidth*R.getSampleSize();
    uint8_t* data = new uint8_t[dataSize];

    for(int y = 0; y < tiley; y++) for(int x = 0; x < tilex; x++) {
        R.getWindow(x*tilewidth, y*tilelength, tilewidth, tilelength, data);
        if(W.WriteTile(x, y, data, crop) < 0) {
            std::cerr << "Error while writting tile (" << x << "," << y << ")" << std::endl;
            return 2;
        }
    }

    R.close();
    if(W.close() < 0) {
        std::cerr << "Error while writting index" << std::endl;
        return 2;
    }
    
    return 0;
}
