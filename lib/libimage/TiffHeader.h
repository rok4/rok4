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

namespace TiffHeader {

static const size_t headerSize = 142;

static const uint8_t TIFF_HEADER_RAW_INT8_RGB[142]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142
static const uint8_t TIFF_HEADER_RAW_INT8_RGBA[142]  = { //FIXME
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) | 1      | 1 (Int8)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142

static const uint8_t TIFF_HEADER_RAW_INT8_GRAY[142]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers un bloc mémoire 8
    3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142

static const uint8_t TIFF_HEADER_RAW_FLOAT32_GRAY[142]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   32, 0, 0, 0,     // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers 1 bloc mémoire 32
    3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 3 (Float)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142



static const uint8_t TIFF_HEADER_LZW_FLOAT32_GRAY[142]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   32, 0, 0, 0,     // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers 1 bloc mémoire 32
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 3 (Float)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142

static const uint8_t TIFF_HEADER_LZW_INT8_GRAY[142]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers un bloc mémoire 8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 1, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 1
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142

static const uint8_t TIFF_HEADER_LZW_INT8_RGB[142]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142

static const uint8_t TIFF_HEADER_LZW_INT8_RGBA[142]  = { //FIXME
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   5, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142


static const uint8_t TIFF_HEADER_DEFLATE_FLOAT32_GRAY[142]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   32, 0, 0, 0,     // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers 1 bloc mémoire 32
    3, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 8 (AdobeDEFLATE)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 3 (Float)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142

static const uint8_t TIFF_HEADER_DEFLATE_INT8_GRAY[142]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers un bloc mémoire 8
    3, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 8 (AdobeDEFLATE)
    6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 1, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 1
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142

static const uint8_t TIFF_HEADER_DEFLATE_INT8_RGB[142]  = {
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 8 (AdobeDEFLATE)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142

static const uint8_t TIFF_HEADER_DEFLATE_INT8_RGBA[142]  = { //FIXME
    73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
    10, 0,                                         // 8  | nombre de tags sur 16 bits (10)
    // ..                                                | TIFFTAG              | DATA TYPE | NUMBER | VALUE
    0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
    1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
    2, 1,   3, 0,   3, 0, 0, 0,   134,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
    3, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 8 (AdobeDEFLATE)
    6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
    17,1,   4, 0,   1 ,0, 0, 0,   142,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 142
    21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
    22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
    23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
    83,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 118| SAMPLEFORMAT    (339)| SHORT (3) |        | 1 (Int8)
    0, 0, 0, 0,                                    // 130| fin de l'IFD
    8, 0,   8, 0,   8, 0,   8, 0
};                         // 134| 4x 8 sur 16 bits (pointés par les samplesperpixels)
// 142
}
#endif
