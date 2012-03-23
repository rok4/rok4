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

#include <iostream>
#include <string.h> // Pour memcpy
#include <algorithm>

#include "TiffLZWEncoder.h"
#include "Logger.h"
#include "lzwEncoder.h"

const uint8_t TIFF_HEADER_LZW_FLOAT32_GRAY[128]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    9, 0,                                          // 8  | nombre de tags sur 16 bits (10)
    // .. | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   32, 0, 0, 0,     // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers 1 bloc mémoire 32
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   128,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 128
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 1, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 1
    0, 0, 0, 0,                                    // 118| fin de l'IFD
    8, 0,   8, 0,   8, 0
};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128

const uint8_t TIFF_HEADER_LZW_INT8_GRAY[128]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    9, 0,                                          // 8  | nombre de tags sur 16 bits (10)
    // .. | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers un bloc mémoire 8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   128,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 128
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 1, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 1
    0, 0, 0, 0,                                    // 118| fin de l'IFD
    8, 0,   8, 0,   8, 0
};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128

const uint8_t TIFF_HEADER_LZW_INT8_RGB[128]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    9, 0,                                          // 8  | nombre de tags sur 16 bits (10)
    // .. | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   122,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   128,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 128
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    0, 0, 0, 0,                                    // 118| fin de l'IFD
    8, 0,   8, 0,   8, 0
};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128

const uint8_t TIFF_HEADER_LZW_INT8_RGBA[128]  = { //FIXME
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    9, 0,                                          // 8  | nombre de tags sur 16 bits (10)
    // .. | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   122,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   128,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 128
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    0, 0, 0, 0,                                    // 118| fin de l'IFD
    8, 0,   8, 0,   8, 0
};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128


TiffLZWEncoder::TiffLZWEncoder(Image* image): image(image), line(-1), rawBufferSize(0), lzwBufferSize(0),lzwBufferPos(0) , lzwBuffer(NULL), rawBuffer(NULL)
{
    
}


size_t TiffLZWEncoder::read(uint8_t *buffer, size_t size_to_read) {
    size_t offset = 0, header_size=128, linesize=image->width*image->channels, dataToCopy=0;
    if (!lzwBuffer) {
        rawBuffer = new uint8_t[image->height*image->width*image->channels];
        for (int lRead = 0 ; lRead < image->height ; lRead++) {
            // Pour l'instant on ne gere que le type uint8_t
            image->getline((uint8_t*)(rawBuffer + rawBufferSize), lRead);
            rawBufferSize += linesize*sizeof(uint8_t);
        }

        lzwEncoder encoder;
        lzwBuffer = encoder.encode(rawBuffer,rawBufferSize, lzwBufferSize);
        delete[] rawBuffer;
        rawBuffer = NULL;
    }

    if (line == -1) { // écrire le header tiff
        // Si pas assez de place pour le header, ne rien écrire.
        if (size_to_read < header_size) return 0;

        // Ceci est du tiff avec une seule strip.
        if (image->channels==1)
            memcpy(buffer, TIFF_HEADER_LZW_INT8_GRAY, header_size);
        else if (image->channels==3)
            memcpy(buffer, TIFF_HEADER_LZW_INT8_RGB, header_size);
        else if (image->channels==4)
            memcpy(buffer, TIFF_HEADER_LZW_INT8_RGBA, header_size);
        *((uint32_t*)(buffer+18))  = image->width;
        *((uint32_t*)(buffer+30))  = image->height;
        *((uint32_t*)(buffer+102)) = image->height;
        *((uint32_t*)(buffer+114)) = lzwBufferSize;
        offset = header_size;
        line = 0;
    }
    
    if (size_to_read - offset > 0 ) { // il reste de la place
        if (lzwBufferPos <= lzwBufferSize){ // il reste de la donnée
             dataToCopy = std::min(size_to_read-offset, lzwBufferSize -lzwBufferPos);
             memcpy(buffer+offset,lzwBuffer+lzwBufferPos,dataToCopy);
             lzwBufferPos+=dataToCopy;
             offset+=dataToCopy;
        }
    }

    return offset;
}

bool TiffLZWEncoder::eof()
{
    return (lzwBufferPos>=lzwBufferSize);
}

TiffLZWEncoder::~TiffLZWEncoder()
{
    delete image;
}
