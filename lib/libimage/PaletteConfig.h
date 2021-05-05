/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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

#ifndef PALETTECONFIG_H
#define PALETTECONFIG_H
#include <stdint.h>
#include "zlib.h"
#include <string.h>
#include "byteswap.h"

static const uint8_t BLACK[3] = {0, 255, 0};

static const uint8_t tRNS[256+12] = {
    0,  0,  1,  0,                  // 256
    't','R','N','S',                  // tag
    255,254,253,252,251,250,249,248,247,246,245,244,243,242,241,240,239,238,237,236,235,234,233,232,231,230,229,228,227,226,225,224,
    223,222,221,220,219,218,217,216,215,214,213,212,211,210,209,208,207,206,205,204,203,202,201,200,199,198,197,196,195,194,193,192,
    191,190,189,188,187,186,185,184,183,182,181,180,179,178,177,176,175,174,173,172,171,170,169,168,167,166,165,164,163,162,161,160,
    159,158,157,156,155,154,153,152,151,150,149,148,147,146,145,144,143,142,141,140,139,138,137,136,135,134,133,132,131,130,129,128,
    127,126,125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100, 99, 98, 97, 96,
    95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64,
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
    31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
    0x5a, 0x66, 0xe1, 0x83
};                     //crc

static void buildPLTE ( uint8_t pLTE[780],bool transparent, const uint8_t rgb[3] ) {
    // Remplissage de la palette
    pLTE[0] = 0;
    pLTE[1] = 0;
    pLTE[2] = 3;
    pLTE[3] = 0;
    pLTE[4] = 'P';
    pLTE[5] = 'L';
    pLTE[6] = 'T';
    pLTE[7] = 'E';
    if ( transparent ) {
        for ( int i = 0; i < 256; i++ ) {
            memcpy ( pLTE + 8 + 3*i, rgb, 3 );
        }
    } else for ( int i = 0; i < 256; i++ ) {
            pLTE[3*i+8]  = i + ( ( 255 - i ) *rgb[0] + 127 ) / 255;
            pLTE[3*i+9]  = i + ( ( 255 - i ) *rgb[1] + 127 ) / 255;
            pLTE[3*i+10] = i + ( ( 255 - i ) *rgb[2] + 127 ) / 255;
        }
    uint32_t crc = crc32 ( 0, Z_NULL, 0 );
    crc = crc32 ( crc, pLTE + 4, 3*256+4 );
    * ( ( uint32_t* ) ( pLTE + 256*3 + 8 ) ) = bswap_32 ( crc );
}

#endif // PALETTECONFIG_H
