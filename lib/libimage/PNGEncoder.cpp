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

#include <iostream>

#include "PNGEncoder.h"
#include "byteswap.h"
#include <boost/log/trivial.hpp>
#include <string.h> // Pour memcpy


// IEND chunck
static const uint8_t IEND[12] = {
    0, 0, 0, 0, 'I', 'E', 'N', 'D',    // 8  | taille et type du chunck IHDR
    0xae, 0x42, 0x60, 0x82
};           // crc32

static const uint8_t PNG_HEADER_GRAY[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                                    // 16 | width
    0, 0, 1, 0,                                    // 20 | height
    8,                                             // 24 | bit depth
    0,                                             // 25 | Colour type
    0,                                             // 26 | Compression method
    0,                                             // 27 | Filter method
    0,                                             // 28 | Interlace method
    0xd3, 0x10, 0x3f, 0x31
};                       // 29 | crc32
// 33

static const uint8_t PNG_HEADER_RGB[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                                    // 16 | width
    0, 0, 1, 0,                                    // 20 | height
    8,                                             // 24 | bit depth
    2,                                             // 25 | Colour type
    0,                                             // 26 | Compression method
    0,                                             // 27 | Filter method
    0,                                             // 28 | Interlace method
    0xd3, 0x10, 0x3f, 0x31
};                       // 29 | crc32
// 33

static const uint8_t PNG_HEADER_RGBA[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                                    // 16 | width
    0, 0, 1, 0,                                    // 20 | height
    8,                                             // 24 | bit depth
    6,                                             // 25 | Colour type
    0,                                             // 26 | Compression method
    0,                                             // 27 | Filter method
    0,                                             // 28 | Interlace method
    0xd3, 0x10, 0x3f, 0x31
};                       // 29 | crc32
// 33

static const uint8_t PNG_HEADER_PALETTE[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                                    // 16 | width
    0, 0, 1, 0,                                    // 20 | height
    8,                                             // 24 | bit depth
    3,                                             // 25 | Colour type
    0,                                             // 26 | Compression method
    0,                                             // 27 | Filter method
    0,                                             // 28 | Interlace method
    0xd3, 0x10, 0x3f, 0x31
};


//For PNG compression method 0, the zlib compression method/flags code shall specify method code 8 (deflate compression) and an LZ77 window size of not more than 32768 bytes. The zlib compression method number is not the same as the PNG compression method number in the IHDR chunk (see 11.2.2 IHDR Image header). The additional flags shall not specify a preset dictionary.
//

void PNGEncoder::addCRC ( uint8_t *buffer, uint32_t length ) {
    * ( ( uint32_t* ) buffer ) = bswap_32 ( length );
    uint32_t crc = crc32 ( 0, Z_NULL, 0 );
    crc = crc32 ( crc, buffer + 4, length + 4 );
    * ( ( uint32_t* ) ( buffer+8+length ) ) = bswap_32 ( crc );
}

size_t PNGEncoder::write_IHDR ( uint8_t *buffer, size_t size, uint8_t colortype ) {
    if ( colortype==0 ) {
        if ( sizeof ( PNG_HEADER_GRAY ) > size ) return 0;
        memcpy ( buffer, PNG_HEADER_GRAY, sizeof ( PNG_HEADER_GRAY ) ); // cf: http://www.w3.org/TR/PNG/#11IHDR
        if ( palette && palette->getPalettePNGSize() !=0 ) {
            colortype=3;
        }
    } else if ( colortype==2 ) {
        if ( sizeof ( PNG_HEADER_RGB ) > size ) return 0;
        memcpy ( buffer, PNG_HEADER_RGB, sizeof ( PNG_HEADER_RGB ) ); // cf: http://www.w3.org/TR/PNG/#11IHDR
    } else if ( colortype==6 ) {
        if ( sizeof ( PNG_HEADER_RGBA ) > size ) return 0;
        memcpy ( buffer, PNG_HEADER_RGBA, sizeof ( PNG_HEADER_RGBA ) ); // cf: http://www.w3.org/TR/PNG/#11IHDR
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Type de couleur non gere : " << colortype ;
        return 0;
    }
    * ( ( uint32_t* ) ( buffer+16 ) ) = bswap_32 ( image->getWidth() ); // ajoute le champs width
    * ( ( uint32_t* ) ( buffer+20 ) ) = bswap_32 ( image->getHeight() ); // ajoute le champs height
    buffer[25] = colortype;                               // ajoute le champs colortype
    addCRC ( buffer+8, 13 );                              // signe le chunck avca un CRC32
    line++;
    if ( colortype==3 ) {
        if ( sizeof ( PNG_HEADER_PALETTE ) + palette->getPalettePNGSize() > size ) return 0;
//                      BOOST_LOG_TRIVIAL(debug) << "Ajout de la palette, size : " << size << "size final : " << (sizeof(PNG_HEADER_PALETTE) + palette->getPalettePNGSize());
        memcpy ( buffer+ sizeof ( PNG_HEADER_PALETTE ), palette->getPalettePNG(), palette->getPalettePNGSize() );
        return ( sizeof ( PNG_HEADER_PALETTE ) + palette->getPalettePNGSize() );
    }
    if ( colortype==0 )
        return sizeof ( PNG_HEADER_GRAY );
    return sizeof ( PNG_HEADER_RGB );
}

size_t PNGEncoder::write_IEND ( uint8_t *buffer, size_t size ) {
    if ( sizeof ( IEND ) > size ) return 0;
    memcpy ( buffer, IEND, sizeof ( IEND ) );
    line++;
    return sizeof ( IEND );
}

size_t PNGEncoder::write_IDAT ( uint8_t *buffer, size_t size ) {
    if ( size <= 12 ) return 0;
    buffer[4] = 'I';
    buffer[5] = 'D';
    buffer[6] = 'A';
    buffer[7] = 'T';

    zstream.next_out  = buffer + 8; // laisser 8 octets au debut pour le header du chunck
    zstream.avail_out = size - 12;  // et 4 octets à la fin pour crc32

    while ( line >= 0 && line < image->getHeight() && zstream.avail_out > 0 ) { // compresser les données dans des chunck idat
        if ( zstream.avail_in == 0 ) {                                    // si plus de donnée en entrée de la zlib, on lit une nouvelle ligne
            image->getline ( linebuffer+1, line++ );
            zstream.next_in  = ( ( uint8_t* ) ( linebuffer+1 ) ) - 1;
            zstream.avail_in = image->getWidth() * image->getChannels() + 1;
        }
        if ( deflate ( &zstream, Z_NO_FLUSH ) != Z_OK ) return 0;         // return 0 en cas d'erreur.
    }

    if ( line == image->getHeight() && zstream.avail_out > 6 ) { // plus d'entrée : il faut finaliser la compression
        int r = deflate ( &zstream, Z_FINISH );
        if ( r == Z_STREAM_END ) line++;                   // on indique que l'on a compressé fini en passant line ) height+1
        else if ( r != Z_OK ) return 0;                    // une erreur
    }
    uint32_t length = size - zstream.avail_out;   // taille des données écritres
    addCRC ( buffer, length - 12 );               // signature du chunck
    return length;
}


size_t PNGEncoder::read ( uint8_t *buffer, size_t size ) {
    size_t pos = 0;
    uint8_t colortype=2;
    // On traite 2 cas : 'Greyscale' et 'Truecolor'
    // cf: http://www.w3.org/TR/PNG/#11IHDR

    if ( image->getChannels()==1 )
        colortype=0;
    else if ( image->getChannels()==3 )
        colortype=2;
    else if ( image->getChannels()==4 )
        colortype=6;
    if ( line == -1 ) pos += write_IHDR ( buffer, size, colortype );
    if ( line >= 0 && line <= image->getHeight() ) pos += write_IDAT ( buffer + pos, size - pos );
    if ( line == image->getHeight() +1 ) pos += write_IEND ( buffer + pos, size - pos );
    return pos;
}

bool PNGEncoder::eof() {
    return ( line > image->getHeight() +1 );
}

PNGEncoder::PNGEncoder ( Image* image,Palette* palette ) : image ( image ), line ( -1 ), palette ( palette ) , stubpalette ( NULL ) {
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.data_type = Z_BINARY;
    deflateInit ( &zstream, 5 ); // taux de compression zlib
    zstream.avail_in = 0;
    linebuffer = new uint8_t[image->getWidth() * image->getChannels() + 1]; // On rajoute une valeur en plus pour l'index de debut de ligne png qui sera toujours 0 dans notre cas. TODO : essayer d'aligner en memoire pour des getline plus efficace
    linebuffer[0] = 0;
    if ( ! palette ) {
        stubpalette = new Palette();
        palette = stubpalette;
    }
}

PNGEncoder::~PNGEncoder() {
    deflateEnd ( &zstream );
    if ( linebuffer ) delete[] linebuffer;
    delete image;
    if ( stubpalette )
        delete stubpalette;
}


/*
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
                0x5a, 0x66, 0xe1, 0x83};                     //crc

*/
/*
ColorizePNGEncoder::ColorizePNGEncoder(Image<pixel_gray> *image, bool transparent, const uint8_t rgb[3]) : PNGEncoder<pixel_gray>(image), transparent(transparent) {
  PLTE[0] = 0;   PLTE[1] = 0;   PLTE[2] = 3;   PLTE[3] = 0;
  PLTE[4] = 'P'; PLTE[5] = 'L'; PLTE[6] = 'T'; PLTE[7] = 'E';
  if(transparent) for(int i = 0; i < 256; i++) memcpy(PLTE + 8 + 3*i, rgb, 3);
  else for(int i = 0; i < 256; i++) {
    PLTE[3*i+8]  = i + ((255 - i)*rgb[0] + 127) / 255;
    PLTE[3*i+9]  = i + ((255 - i)*rgb[1] + 127) / 255;
    PLTE[3*i+10] = i + ((255 - i)*rgb[2] + 127) / 255;
  }
  uint32_t crc = crc32(0, Z_NULL, 0);
  crc = crc32(crc, PLTE + 4, 3*256+4);
 *((uint32_t*) (PLTE + 256*3 + 8)) = bswap_32(crc);
  line = -3;
}

ColorizePNGEncoder::~ColorizePNGEncoder() {
  std::cerr << "delete ColorizePNGEncoder" << std::endl;
}

size_t ColorizePNGEncoder::write_PLTE(uint8_t *buffer, size_t size) {
  if(sizeof(PLTE) > size) return 0;
  memcpy(buffer, PLTE, sizeof(PLTE));
  line++;
  return sizeof(PLTE);
}

size_t ColorizePNGEncoder::write_tRNS(uint8_t *buffer, size_t size) {
  if(transparent) {
    if(sizeof(tRNS) > size) return 0;
    memcpy(buffer, tRNS, sizeof(tRNS));
    line++;
    return sizeof(tRNS);
  }
  else {line++; return 0;}
}

size_t ColorizePNGEncoder::getdata(uint8_t *buffer, size_t size) {
  size_t pos = 0;
  if(line == -3) pos += write_IHDR(buffer, size, 3);
  if(line == -2) pos += write_PLTE(buffer + pos, size - pos);
  if(line == -1) pos += write_tRNS(buffer + pos, size - pos);
  if(line >= 0)  pos += PNGEncoder<pixel_gray>::getdata(buffer + pos, size - pos);
  return pos;
}

template class PNGEncoder<pixel_gray>;
template class PNGEncoder<pixel_rgb>;
template class PNGEncoder<pixel_rgba>;

 */
