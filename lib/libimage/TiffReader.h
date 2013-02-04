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

#ifndef _TIFFREADER_
#define _TIFFREADER_

#include <stdint.h>
#include "tiffio.h"

class TiffReader {
  public:
    TIFF *input;           // libtiff object

    uint32_t width;        // TIFFTAG_IMAGEWIDTH
    uint32_t length;       // TIFFTAG_IMAGELENGTH
    uint16_t photometric;  // 

    uint32_t tileWidth;    // TIFFTAG_IMAGELENGTH
    uint32_t tileLength;   // TIFFTAG_IMAGEWIDTH
    uint16_t bitspersample;

    uint8_t *LineBuffer;   // temporary buffer

    int sampleSize;        // Taille en octets d'un pixel (RGB = 3, Gray = 1)

    int BufferSize;     // Nombre de lignes(tuiles) que peut contenir le Buffer.
    uint8_t **_Buffer;  // Contient un cache des lignes ou tuiles
    uint8_t **Buffer;   // Contient pour chaque ligne(tuile) un pointer vers un cache 0 si pas en cache
    int *BIndex;        // Contient la ligne(tuile) du cache à cette position dans Buffer.
    int Buffer_pos;     // pointeur roulant de la position courrant dans Buffer.
    
    uint8_t* getRawLine(uint32_t line);
    uint8_t* getRawTile(uint32_t tile);


  public:    
    uint8_t* getEncodedTile(uint32_t tile);
    uint8_t* getLine(uint32_t line, uint32_t offset = 0, uint32_t size = -1);

    TiffReader(const char* filename);
    void close();
    int getWindow(int offsetx, int offsety, int width, int length, uint8_t *buffer);

    uint32_t getWidth() {return width;}
    uint32_t getLength() {return length;}
    uint16_t getPhotometric() {return photometric;}
    int getSampleSize() {return sampleSize;}
    uint32_t getBitsPerSample() {return bitspersample;}
};

#endif // _TIFFREADER_
