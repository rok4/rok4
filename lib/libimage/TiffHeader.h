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

#ifndef _TIFFHEADER_
#define _TIFFHEADER_
#include "Format.h"
#include "Image.h"



namespace TiffHeader {

static const size_t headerSize ( int channel ) {
    switch ( channel ) {
    case 1:
        return 134;
    case 3:
        return 146;
    case 4:
        return 162;
    }

}

static const uint8_t TIFF_HEADER_RAW_INT8_GRAY[134]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers un bloc mémoire 8
    3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   134,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 134
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0                                     // 130| fin de l'IFD
};                         // 134

static const uint8_t TIFF_HEADER_RAW_FLOAT32_GRAY[134]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   32, 0, 0, 0,     // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers 1 bloc mémoire 32
    3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   134,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 134
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 3 (Float)
    0, 0, 0, 0                                     // 130| fin de l'IFD
};                         // 134

static const uint8_t TIFF_HEADER_RAW_INT8_RGB[146]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   146,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 146
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   3, 0, 0, 0,   140, 0, 0, 0,    // 118| SAMPLEFORMAT    (339)| SHORT (3) | 3      | pointeur vers un bloc mémoire 1,1,1
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,                          // 134| 3x 8 sur 16 bits (pointés par les samplesperpixels)
    1, 0,   1, 0,   1, 0                           // 140| 3x 8 sur 16 bits (pointés par les samplesperpixels)
};                                                 // 146

static const uint8_t TIFF_HEADER_RAW_INT8_RGBA[162]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    11, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   4, 0, 0, 0,   146,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   162,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 162
    21,1,   3, 0,   1, 0, 0, 0,   4, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 4
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 4, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 4
    82,1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 118| EXTRASAMPLES    (338)| SHORT (3) |        | 2 (UNASSOCALPHA)
    83,1,   3, 0,   4, 0, 0, 0,   154, 0, 0, 0,    // 130| SAMPLEFORMAT    (339)| SHORT (3) | 4      | 1 (Int8)
    0, 0, 0, 0,                                    // 142| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0,                  // 146| 4x 8 sur 16 bits (pointés par les samplesperpixels)
    1, 0,   1, 0,   1, 0,   1, 0                   // 154| 4x 8 sur 16 bits (pointés par les samplesperpixels)
};
// 162


static const uint8_t TIFF_HEADER_LZW_FLOAT32_GRAY[134]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   32, 0, 0, 0,     // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers 1 bloc mémoire 32
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   134,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 134
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 3 (Float)
    0, 0, 0, 0                                     // 130| fin de l'IFD
};                                                 // 134


static const uint8_t TIFF_HEADER_LZW_INT8_GRAY[134]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers un bloc mémoire 8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   134,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 134
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 1, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 1
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0                                     // 130| fin de l'IFD
};                                                 // 134

static const uint8_t TIFF_HEADER_LZW_INT8_RGB[146]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   146,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 146
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   3, 0, 0, 0,   140, 0, 0, 0,    // 118| SAMPLEFORMAT    (339)| SHORT (3) | 3      | pointeur vers un bloc mémoire 1,1,1
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,                          // 134| 3x 8 sur 16 bits (pointés par les samplesperpixels)
    1, 0,   1, 0,   1, 0                           // 140| 3x 8 sur 16 bits (pointés par les samplesperpixels)
};                                                 // 146

static const uint8_t TIFF_HEADER_LZW_INT8_RGBA[162]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    11, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   4, 0, 0, 0,   146,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   162,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 162
    21,1,   3, 0,   1, 0, 0, 0,   4, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 4
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 4, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 4
    82,1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 118| EXTRASAMPLES    (338)| SHORT (3) |        | 2 (UNASSOCALPHA)
    83,1,   3, 0,   4, 0, 0, 0,   154, 0, 0, 0,    // 130| SAMPLEFORMAT    (339)| SHORT (3) | 4      | 1 (Int8)
    0, 0, 0, 0,                                    // 142| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0,                  // 146| 4x 8 sur 16 bits (pointés par les samplesperpixels)
    1, 0,   1, 0,   1, 0,   1, 0                   // 154| 4x 8 sur 16 bits (pointés par les samplesperpixels)
};                                                 // 162

static const uint8_t TIFF_HEADER_ZIP_FLOAT32_GRAY[134]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   32, 0, 0, 0,     // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers 1 bloc mémoire 32
    3, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 8 (AdobeDEFLATE)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   134,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 134
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 3 (Float)
    0, 0, 0, 0                                     // 130| fin de l'IFD
};                                                 // 134

static const uint8_t TIFF_HEADER_ZIP_INT8_GRAY[134]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers un bloc mémoire 8
    3, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 8 (AdobeDEFLATE)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   134,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 134
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 1, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 1
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0                                     // 130| fin de l'IFD
};                                                 // 134

static const uint8_t TIFF_HEADER_ZIP_INT8_RGB[146]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 8 (AdobeDEFLATE)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   146,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 146
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   3, 0, 0, 0,   140, 0, 0, 0,    // 118| SAMPLEFORMAT    (339)| SHORT (3) | 3      | pointeur vers un bloc mémoire 1,1,1
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,                          // 134| 3x 8 sur 16 bits (pointés par les samplesperpixels)
    1, 0,   1, 0,   1, 0                           // 140| 3x 8 sur 16 bits (pointés par les samplesperpixels)
};                                                 // 146                                               // 150

static const uint8_t TIFF_HEADER_ZIP_INT8_RGBA[162]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    11, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   4, 0, 0, 0,   146,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 8 (AdobeDEFLATE)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   162,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 162
    21,1,   3, 0,   1, 0, 0, 0,   4, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 4
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 4, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 4
    82,1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 118| EXTRASAMPLES    (338)| SHORT (3) |        | 2 (UNASSOCALPHA)
    83,1,   3, 0,   4, 0, 0, 0,   154, 0, 0, 0,    // 130| SAMPLEFORMAT    (339)| SHORT (3) | 4      | 1 (Int8)
    0, 0, 0, 0,                                    // 142| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0,                  // 146| 4x 8 sur 16 bits (pointés par les samplesperpixels)
    1, 0,   1, 0,   1, 0,   1, 0                   // 154| 4x 8 sur 16 bits (pointés par les samplesperpixels)
};                                                 // 162

static const uint8_t TIFF_HEADER_PKB_FLOAT32_GRAY[134]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   32, 0, 0, 0,     // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers 1 bloc mémoire 32
    3, 1,   3, 0,   1, 0, 0, 0,   5, 128, 0, 0,    // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 32773 (Packbits)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   134,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 134
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 3 (Float)
    0, 0, 0, 0                                     // 130| fin de l'IFD
};                                                 // 134

static const uint8_t TIFF_HEADER_PKB_INT8_GRAY[134]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers un bloc mémoire 8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 128, 0, 0,    // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 32773 (Packbits)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   134,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 134
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 1, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 1
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0                                     // 130| fin de l'IFD
};                                                 // 134

static const uint8_t TIFF_HEADER_PKB_INT8_RGB[146]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 128, 0, 0,    // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 32773 (Packbits)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   146,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 146
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   3, 0, 0, 0,   140, 0, 0, 0,    // 118| SAMPLEFORMAT    (339)| SHORT (3) | 3      | pointeur vers un bloc mémoire 1,1,1
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,                          // 134| 3x 8 sur 16 bits (pointés par les samplesperpixels)
    1, 0,   1, 0,   1, 0                           // 140| 3x 8 sur 16 bits (pointés par les samplesperpixels)
};                                                 // 146

static const uint8_t TIFF_HEADER_PKB_INT8_RGBA[162]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    11, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   4, 0, 0, 0,   146,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 128, 0, 0,    // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 32773 (Packbits)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   162,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 162
    21,1,   3, 0,   1, 0, 0, 0,   4, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 4
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 4, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 4
    82,1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 118| EXTRASAMPLES    (338)| SHORT (3) |        | 2 (UNASSOCALPHA)
    83,1,   3, 0,   4, 0, 0, 0,   154, 0, 0, 0,    // 130| SAMPLEFORMAT    (339)| SHORT (3) | 4      | pointeur vers un bloc mémoire 1,1,1,1
    0, 0, 0, 0,                                    // 142| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0,                  // 146| 4x 8 sur 16 bits (pointés par les samplesperpixels)
    1, 0,   1, 0,   1, 0,   1, 0                   // 154| 4x 8 sur 16 bits (pointés par les samplesperpixels)
};                                                 // 162

static const uint8_t GEOTIFF_HEADER_PART[60]  = {
    // ..                                                     | TIFFTAG                    | DATA TYPE    | NUMBER | VALUE
    14,  131,   12, 0,   3, 0, 0, 0,   0, 0, 0, 0,      // 0  | ModelPixelScaleTag  (33550)| DOUBLE  (12) | 3      | pointeur
    130, 132,   12, 0,   6, 0, 0, 0,   0, 0, 0, 0,      // 12 | ModelTiepointTag    (33922)| DOUBLE  (12) | 6      | pointeur
    175, 135,   3,  0,   0, 0, 0, 0,   0, 0, 0, 0,      // 24 | GeoKeyDirectoryTag  (34735)| SHORT    (3) | ?      | pointeur
    176, 135,   12, 0,   0, 0, 0, 0,   0, 0, 0, 0,      // 36 | GeoDoubleParamsTag  (34736)| DOUBLE  (12) | ?      | pointeur
    177, 135,   2,  0,   0, 0, 0, 0,   0, 0, 0, 0,      // 48 | GeoAsciiParamsTag   (34737)| ASCII    (2) | ?      | pointeur
                                                        // 60  
};

static const size_t GEOTIFF_HEADER_PART_SIZE = 60 ;

static const void appendToGeoKeyDirectory( uint16_t* geoKeyDirectory, size_t* geoKeyDirectorySize, uint16_t arg1, uint16_t arg2, uint16_t arg3, uint16_t arg4){
    *(geoKeyDirectory + (*geoKeyDirectorySize)*4 ) = arg1;
    *(geoKeyDirectory + (*geoKeyDirectorySize)*4 +1 ) = arg2;
    *(geoKeyDirectory + (*geoKeyDirectorySize)*4 +2 ) = arg3;
    *(geoKeyDirectory + (*geoKeyDirectorySize)*4 +3 ) = arg4;
    (*geoKeyDirectorySize)+=1;
};

static const void appendToGeoKeyDirectory( uint16_t* geoKeyDirectory, size_t* geoKeyDirectorySize, int arg1, int arg2, int arg3, int arg4){
    appendToGeoKeyDirectory(geoKeyDirectory,geoKeyDirectorySize,(uint16_t) arg1,(uint16_t) arg2,(uint16_t) arg3,(uint16_t) arg4);
};

static const void appendToGeoDoubleParams( double* geoDoubleParams, size_t* geoDoubleParamsSize, double arg1){
    *(geoDoubleParams + (*geoDoubleParamsSize) ) = arg1;
    (*geoDoubleParamsSize)+=1;
};


struct param {
    std::string proj;
    int geotifftag;
};

struct projParams {
    int projcoordtransgeokey;
    size_t nbparam;
    param listparam[15];
};

static const projParams LCC_1SP = {     9,
                                        5,
                                        {
                                            {"lon_0", 3080},
                                            {"lat_1", 3081},
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"k_0", 3092}
                                        }
                                    };
                                    
static const projParams LCC_2SP = {     8,
                                        6,
                                        {
                                            {"lat_1", 3078},
                                            {"lat_2", 3079},
                                            {"lon_0", 3084},
                                            {"lat_0", 3085},
                                            {"x_0", 3086},
                                            {"y_0", 3087}
                                        }
                                    };
                                    
static const projParams MERC_1SP = {    7,
                                        4,
                                        {
                                            {"lon_0", 3080},
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"k_0", 3092}
                                        }
                                    };
static const projParams AEA = {         11,
                                        6,
                                        {
                                            {"lat_1", 3078},
                                            {"lat_2", 3079},
                                            {"lon_0", 3080},
                                            {"lat_0", 3081},
                                            {"x_0", 3082},
                                            {"y_0", 3083}
                                        }
                                    };

static const projParams AEQD = {        12,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088},
                                            {"lat_0", 3089}
                                        }
                                    };

static const projParams CASS = {        18,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3080},
                                            {"lat_0", 3081}
                                        }
                                    };

static const projParams CEA = {         28,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3080},
                                            {"lat_ts", 3078}
                                        }
                                    };

static const projParams EQDC = {        13,
                                        6,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lat_1", 3078},
                                            {"lat_2", 3079},
                                            {"lon_0", 3080},
                                            {"lat_0", 3081}
                                        }
                                    };

static const projParams EQC = {         17,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088},
                                            {"lat_ts", 3089}
                                        }
                                    };
static const projParams TMERC = {       1,
                                        5,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3080},
                                            {"lat_0", 3081},
                                            {"k", 3092}
                                        }
                                    };
static const projParams GNOM = {        19,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088},
                                            {"lat_0", 3089}
                                        }
                                    };
                                    
static const projParams OMERC = {       3,
                                        6,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lonc", 3088},
                                            {"lat_0", 3089},
                                            {"k_0", 3093},
                                            {"alpha", 3094}
                                        }
                                    };
                                    
static const projParams LAEA = {        10,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088},
                                            {"lat_0", 3089}
                                        }
                                    };

                                    
static const projParams MILL = {        20,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088},
                                            {"lat_0", 3089}
                                        }
                                    };
                                    
static const projParams STEREA = {      16,
                                        5,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3080},
                                            {"lat_0", 3081},
                                            {"k_0", 3092}
                                        }
                                    };

static const projParams ORTHO = {       21,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088},
                                            {"lat_0", 3089}
                                        }
                                    };

static const projParams POLY = {        22,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3080},
                                            {"lat_0", 3081}
                                        }
                                    };

static const projParams ROBIN = {       23,
                                        3,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088}
                                        }
                                    };

static const projParams SINU = {        24,
                                        3,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088}
                                        }
                                    };

static const projParams STERE = {       14,
                                        4,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088},
                                            {"lat_0", 3089}
                                        }
                                    };

static const projParams VANDG = {       25,
                                        3,
                                        {
                                            {"x_0", 3082},
                                            {"y_0", 3083},
                                            {"lon_0", 3088}
                                        }
                                    };

static uint8_t* insertGeoTags ( Image* image, uint8_t* header, size_t* sizeHeader ) {
    CRS crs = image->getCRS();
    
    if ( &crs == NULL ){
        LOGGER_ERROR("L'objet image n'a pas de géoréférencement.");
        return header;
    }
    
    std::string projName = crs.getProj4Param("proj");
    if ( projName == "" ) {
      LOGGER_ERROR("La projection de l'image est vide.");
      return header;
    }
    
    
    double doubletmp;
    uint16_t* GeoKeyDirectory;
    GeoKeyDirectory = new uint16_t[120]; //max size = 120 uint16_t
    size_t GeoKeyDirectorySize = 0;
    
    double* GeoDoubleParams;
    GeoDoubleParams = new double[26]; //max size = 26 double
    size_t GeoDoubleParamsSize = 0;
    
    std::string GeoAsciiParams = ""; 
   
    
    //initialisation
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,1,1,0,0);
    
    
    //GeoTIFF Configuration Keys
    //GTModelTypeGeoKey
    if ( projName == "longlat" )  {
      appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,1024,0,1,2);
    } else {
      appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,1024,0,1,1);
    }
    
    //GTRasterTypeGeoKey : RasterPixelIsArea
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,1025,0,1,1);
    
    //GTCitationGeoKey : into GeoAsciiParamsTag(34737)
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,1026,34737,crs.getProj4Code().size()+1,GeoAsciiParams.size());
    GeoAsciiParams.append(crs.getProj4Code()+"|");
    //End of  GeoTIFF Configuration Keys
    
    //Geographic CS Parameter Keys
    //GeographicTypeGeoKey : user-defined(32767)
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2048,0,1,32767);
    //GeogGeodeticDatumGeoKey : user-defined(32767)
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2050,0,1,32767);
    //GeogPrimeMeridianGeoKey : user-defined(32767)
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2051,0,1,32767);
    //GeogLinearUnitsGeoKey : meter(9001)
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2052,0,1,9001);
    //GeogAngularUnitsGeoKey : degree(9102)
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2054,0,1,9102);
    //GeogEllipsoidGeoKey : user-defined(32767)
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2056,0,1,32767);
    //GeogSemiMajorAxisGeoKey : into GeoDoubleParams(34736)
    if ( crs.getProj4Param("a") != "" ){
      if ( !sscanf ( crs.getProj4Param("a").c_str(),"%lf",&doubletmp ) ) {
        LOGGER_ERROR("Impossible de parser le demi grand axe de la definition proj4");
      } else {
        appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2057,34736,1,GeoDoubleParamsSize);
        appendToGeoDoubleParams(GeoDoubleParams,&GeoDoubleParamsSize, doubletmp);
      }
    }
    //GeogSemiMinorAxisGeoKey : into GeoDoubleParams(34736)
    if ( crs.getProj4Param("b") != "" ){
      if ( !sscanf ( crs.getProj4Param("b").c_str(),"%lf",&doubletmp ) ) {
        LOGGER_ERROR("Impossible de parser le demi petit axe de la definition proj4");
      } else {
        appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2058,34736,1,GeoDoubleParamsSize);
        appendToGeoDoubleParams(GeoDoubleParams,&GeoDoubleParamsSize, doubletmp);
      }
    }
    //GeogInvFlatteningGeoKey : into GeoDoubleParams(34736)
    if ( crs.getProj4Param("rf") != "" ){
      if ( !sscanf ( crs.getProj4Param("rf").c_str(),"%lf",&doubletmp ) ) {
        LOGGER_ERROR("Impossible de parser l'inverse du coefficient d'applatissement de la definition proj4");
      } else {
        appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2059,34736,1,GeoDoubleParamsSize);
        appendToGeoDoubleParams(GeoDoubleParams,&GeoDoubleParamsSize, doubletmp);
      }
    }
    //GeogPrimeMeridianLongGeoKey : into GeoDoubleParams(34736)
    if ( crs.getProj4Param("pm") != "" ){
      if ( !sscanf ( crs.getProj4Param("pm").c_str(),"%lf",&doubletmp ) ) {
        LOGGER_ERROR("Impossible de parser le meridien d'origine de la definition proj4");
      } else {
        appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2061,34736,1,GeoDoubleParamsSize);
        appendToGeoDoubleParams(GeoDoubleParams,&GeoDoubleParamsSize, doubletmp);
      }
    }
    //GeogTOWGS84GeoKey : into GeoDoubleParams(34736)
    if ( crs.getProj4Param("towgs84") != "" ){
      std::string towgs84 = crs.getProj4Param("towgs84");
      
      //Split of the string
      std::vector< std::string > towgs84V;
      size_t pos = 0;
      size_t find = 0;
      while (find != std::string::npos ) {
        find = towgs84.find(",", pos);
        towgs84V.push_back(towgs84.substr(pos, find-pos));
        pos= find+1;
      }
      if ( towgs84V.size() != 3 && towgs84V.size() != 7 ) {
        LOGGER_ERROR("Le prarametre +towgs84 de la definition proj4 n'a pas le nombre d'elements approprie");
      }
      double towgs84Dbl[towgs84V.size()];
      for (size_t i = 0; i < towgs84V.size();i++) {
        if ( sscanf ( towgs84V.at(i).c_str(),"%lf",&(towgs84Dbl[i]) ) !=1 ) {
            LOGGER_ERROR("Impossible de parser un parametre de towgs84 de la definition proj4");
        }
      }
      
      appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,2062,34736,towgs84V.size(),GeoDoubleParamsSize);
      for (size_t i = 0; i < towgs84V.size();i++) {
        appendToGeoDoubleParams(GeoDoubleParams,&GeoDoubleParamsSize, towgs84Dbl[i]);
      }
      
    }
    //End of Geographic CS Parameter Keys
  
    if ( projName != "longlat" ) {
        //GeographicTypeGeoKey
        const projParams * myProjParams = NULL;
        if ( projName == "lcc" ) {
            if ( crs.getProj4Param("lat_2") != "" ) {
                myProjParams = &LCC_2SP;
            } else {
                myProjParams = &LCC_1SP;
            }
        } else if ( projName == "aea" ) {
            myProjParams = &AEA;
        } else if ( projName == "aeqd" ) {
            myProjParams = &AEQD;
        } else if ( projName == "cass" ) {
            myProjParams = &CASS;
        } else if ( projName == "cea" ) {
            myProjParams = &CEA;
        } else if ( projName == "eqdc" ) {
            myProjParams = &EQDC;
        } else if ( projName == "eqc" ) {
            myProjParams = &EQC;
        } else if ( projName == "tmerc" ) {
            myProjParams = &TMERC;
        } else if ( projName == "gnom" ) {
            myProjParams = &GNOM;
        } else if ( projName == "omerc" ) {
            myProjParams = &OMERC;
        } else if ( projName == "laea" ) {
            myProjParams = &LAEA;
        } else if ( projName == "merc" ) {
            myProjParams = &MERC_1SP;
        } else if ( projName == "mill" ) {
            myProjParams = &MILL;
        } else if ( projName == "sterea" ) {
            myProjParams = &STEREA;
        } else if ( projName == "ortho" ) {
            myProjParams = &ORTHO;
        } else if ( projName == "poly" ) {
            myProjParams = &POLY;
        } else if ( projName == "stere" ) {
            myProjParams = &STERE;
        } else if ( projName == "robin" ) {
            myProjParams = &ROBIN;
        } else if ( projName == "sinu" ) {
            myProjParams = &SINU;
        } else if ( projName == "vandg" ) {
            myProjParams = &VANDG;
        }
        if ( myProjParams == NULL ){
            LOGGER_ERROR("La projection de l'image n'est pas supportée.");
            return header;
        }
        
        //ProjectedCSTypeGeoKey : user-defined(32767)
        appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,3072,0,1,32767);
        //ProjectionGeoKey : user-defined(32767)
        appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,3074,0,1,32767);
        //ProjCoordTransGeoKey
        appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,3075,0,1,myProjParams->projcoordtransgeokey);
        //ProjLinearUnitsGeoKey meter(9001)
        appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,3076,0,1,9001);
        
        for (size_t i = 0; i < myProjParams->nbparam; i ++) {
            if (crs.getProj4Param(myProjParams->listparam[i].proj) != ""){
                if ( sscanf ( crs.getProj4Param(myProjParams->listparam[i].proj).c_str(),"%lf",&doubletmp ) !=1 ) {
                    LOGGER_ERROR("Impossible de parser le parametre " + myProjParams->listparam[i].proj + " de la definition proj4");
                } else {
                    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,myProjParams->listparam[i].geotifftag ,34736,1,GeoDoubleParamsSize);
                    appendToGeoDoubleParams(GeoDoubleParams,&GeoDoubleParamsSize, doubletmp);
                }
            }
        }
    }
    
    //End of Geographic CS Parameter Keys
    
    
    //Close the GeoKeyDirectory and set size
    LOGGER_DEBUG("Fermeture de GeoKeyDirectory");
    appendToGeoKeyDirectory(GeoKeyDirectory,&GeoKeyDirectorySize,0,0,0,0);
    *(GeoKeyDirectory+3)=(uint16_t) ((GeoKeyDirectorySize-2));
    
    size_t new_sizeHeader = *sizeHeader + 60 //IFD
                                        + 3*sizeof(double) //Scale
                                        + 6*sizeof(double) //TiePoints
                                        + GeoKeyDirectorySize*4*sizeof(uint16_t)
                                        + GeoDoubleParamsSize*sizeof(double)
                                        + GeoAsciiParams.size()+1;
    uint8_t* new_header;
    new_header = new uint8_t[new_sizeHeader];
    size_t new_offset;
    uint16_t old_nbTag = *(header+8);
    LOGGER_DEBUG("Copie du début de l'en-tête");
    memcpy(new_header,header,10+12*old_nbTag);
    new_offset = 10+12*old_nbTag;
    * ( (uint16_t*) (new_header+8) ) =old_nbTag+5;
    LOGGER_DEBUG("Mise à jour des pointeurs");
    for (uint16_t i = 0;i<old_nbTag;i++) {
        if (((uint32_t)*(new_header+14+i*12)) != 1 ){
            *((uint32_t*)(new_header+18+i*12)) = *((uint32_t*)(new_header+18+i*12))+60;
        }
    }
    
    LOGGER_DEBUG("Ajout de la partie Geotiff");
    mempcpy(new_header+new_offset, GEOTIFF_HEADER_PART, GEOTIFF_HEADER_PART_SIZE );
    new_offset+=GEOTIFF_HEADER_PART_SIZE;
    
    LOGGER_DEBUG("Copie IFD et Fin de l'en-tête");
    memcpy(new_header+new_offset, header+10+12*old_nbTag, *sizeHeader-10-12*old_nbTag);
    new_offset+= *sizeHeader-10-12*old_nbTag;
    
    //append ModelPixelScaleTag
    LOGGER_DEBUG("Ajout de ModelPixelScaleTag");
    *((uint32_t*)(new_header+10+12*old_nbTag+8)) = (uint32_t) new_offset;
    double* ModelPixelScaleTag;
    ModelPixelScaleTag = new double[3];
    *ModelPixelScaleTag= image->getResX();
    *(ModelPixelScaleTag+1)= image->getResY();
    *(ModelPixelScaleTag+2)= (double) 0.0;
    memcpy(new_header+new_offset, ModelPixelScaleTag, 3*sizeof(double));
    new_offset+= 3*sizeof(double);
    delete[] ModelPixelScaleTag;

    //append ModelTiepointTag
    LOGGER_DEBUG("Ajout de ModelTiepointTag");
    *((uint32_t*)(new_header+10+12*old_nbTag+20)) = (uint32_t) new_offset;
    double* ModelTiepointTag;
    ModelTiepointTag = new double[6];
    *ModelTiepointTag= (double) 0.0;
    *(ModelTiepointTag+1)= (double) 0.0;
    *(ModelTiepointTag+2)= (double) 0.0;
    *(ModelTiepointTag+3)= (double) image->getBbox().xmin;
    *(ModelTiepointTag+4)= (double) image->getBbox().ymax;
    *(ModelTiepointTag+5)= (double) 0.0;
    memcpy(new_header+new_offset, ModelTiepointTag, 6*sizeof(double));
    new_offset+= 6*sizeof(double);
    delete[] ModelTiepointTag;

    //append GeoKeyDirectoryTag
    LOGGER_DEBUG("Ajout de GeoKeyDirectoryTag");
    *((uint32_t*)(new_header+10+12*old_nbTag+28)) = (uint32_t) GeoKeyDirectorySize*4;
    *((uint32_t*)(new_header+10+12*old_nbTag+32)) = (uint32_t) new_offset;
    memcpy(new_header+new_offset, GeoKeyDirectory, GeoKeyDirectorySize*4*sizeof(uint16_t));
    new_offset+=GeoKeyDirectorySize*4*sizeof(uint16_t);
    delete[] GeoKeyDirectory;
  
    //append GeoDoubleParamsTag
    LOGGER_DEBUG("Ajout de GeoDoubleParamsTag");
    *((uint32_t*)(new_header+10+12*old_nbTag+40)) = (uint32_t) GeoDoubleParamsSize;
    *((uint32_t*)(new_header+10+12*old_nbTag+44)) = (uint32_t) new_offset;
    memcpy(new_header+new_offset, GeoDoubleParams, GeoDoubleParamsSize*sizeof(double));
    new_offset+=GeoDoubleParamsSize*sizeof(double);
    delete[] GeoDoubleParams;

    //append GeoAsciiParamsTag
    LOGGER_DEBUG("Ajout de GeoAsciiParamsTag");
    *((uint32_t*)(new_header+10+12*old_nbTag+52)) = (uint32_t) GeoAsciiParams.size()+1;
    *((uint32_t*)(new_header+10+12*old_nbTag+56)) = (uint32_t) new_offset;
    char* charAscii = const_cast<char*>(GeoAsciiParams.c_str());
    memcpy(new_header+new_offset,charAscii, GeoAsciiParams.size());
    new_offset+=GeoAsciiParams.size();   
    
    //null byte
    *(new_header+new_offset) = (uint8_t) 0;
    new_offset+=1;
    
    //Modify strip offset
    *((uint32_t*)(new_header+78)) = (uint32_t) new_offset;
    
    
    delete[] header;
    header = new uint8_t[new_offset];
    memcpy(header,new_header, new_offset);
    *sizeHeader = new_offset;
    delete[] new_header;
    
    return header;
    
};




}
#endif
