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
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*++++ NE FAIT RIEN POUR LE MOMENT : NE PAS UTILISER ++++*/
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdio.h>
#include "tiffio.h"
#include <iostream>
#include <stdint.h>
#include <string>
#include <sstream>

#define STRIP_OFFSETS      273
#define ROWS_PER_STRIP     278
#define STRIP_BYTE_COUNTS  279

using namespace std;

struct Entry {
  uint16_t tag;
  uint16_t type;
  uint32_t count;
  uint32_t value; 
};

inline void error(string message) {
    cerr << message << endl;
}

uint8_t *IM;

uint32 width , height,tilewidth , tileheight, rowsperstrip;
uint16 bitspersample, sampleperpixel, photometric, compression , planarconfig, nb_extrasamples;
uint16 *extrasamples;


int main(int argc, char **argv) {
    
    char *input_file = 0, *output_file = 0;

    if (argc == 1){
        cout << std::endl << "cache2work: uncompress and merge tiles in a tiff" << std::endl; 
        cout << "usage: cache2work <inputfile> <outputfile>" << std::endl << std::endl;
        return(0);
    }
    for(int i = 1; i < argc; i++) {
        if(!input_file) input_file = argv[i];
        else if(!output_file) output_file = argv[i];
        else {
            error("Error : argument must specify exactly one input file and one output file");
        }
    }
    if(!output_file || !input_file) error("Error : argument must specify exactly one input file and one output file");
  
    TIFF *TIFF_FILE = 0;
    
    TIFF_FILE = TIFFOpen(input_file, "r");
    if(!TIFF_FILE) error("Unable to open file for reading: " + string(input_file));
    
    if( ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGEWIDTH, &width)                       ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_IMAGELENGTH, &height)                     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_PHOTOMETRIC, &photometric)                ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel)||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_TILELENGTH, &tileheight)                  ||
        ! TIFFGetField(TIFF_FILE, TIFFTAG_TILEWIDTH, &tilewidth)                    ||
         ! TIFFGetField(TIFF_FILE, TIFFTAG_COMPRESSION, &compression)               ||
        ! TIFFGetFieldDefaulted(TIFF_FILE, TIFFTAG_EXTRASAMPLES, &nb_extrasamples, &extrasamples))
    {
        error("Error reading file components: " +  string(input_file));
        return false;
    }

    if (planarconfig != 1)  error("Sorry : only planarconfig = 1 is supported");
    if (bitspersample != 8)  error("Sorry : only bitspersample = 8 is supported");
    if (sampleperpixel != 3)  error("Sorry : tool white manager is just available for sampleperpixel = 3");

}  
