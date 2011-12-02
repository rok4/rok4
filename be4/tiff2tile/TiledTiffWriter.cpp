#include "TiledTiffWriter.h"
#include "byteswap.h"
#include <string.h>
#include <iostream>


// Fonctions pour le manager de sortie de la libjpeg
void init_destination (jpeg_compress_struct *cinfo) {return;}
boolean empty_output_buffer (jpeg_compress_struct *cinfo) {return false;}
void term_destination (jpeg_compress_struct *cinfo) {return;}



static const uint8_t PNG_IEND[12] = {
  0, 0, 0, 0, 'I', 'E', 'N', 'D',    // 8  | taille et type du chunck IHDR
  0xae, 0x42, 0x60, 0x82};           // crc32           


static const uint8_t PNG_HEADER[33] = {
  137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
  0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
  0, 0, 1, 0,                                    // 16 | width
  0, 0, 1, 0,                                    // 20 | height
  8,                                             // 24 | bit depth
  0,                                             // 25 | Colour type
  0,                                             // 26 | Compression method
  0,                                             // 27 | Filter method
  0,                                             // 28 | Interlace method
  0, 0, 0, 0};                                   // 29 | crc32





TiledTiffWriter::TiledTiffWriter(const char *filename, uint32_t width, uint32_t length, uint16_t photometric = PHOTOMETRIC_RGB,
    uint16_t compression = COMPRESSION_NONE, int _quality = -1, uint32_t tilewidth = 256, uint32_t tilelength = 256, uint32_t bitspersample = 8, uint16_t sampleformat = SAMPLEFORMAT_UINT) :
    width(width), length(length), photometric(photometric), compression(compression), quality(_quality), tilewidth(tilewidth), tilelength(tilelength), bitspersample(bitspersample), sampleformat(sampleformat)
{
// input control
    if(width % tilewidth || length % tilelength) std::cerr << "Image size must be a multiple of tile size" << std::endl;
    if(photometric != PHOTOMETRIC_RGB && photometric != PHOTOMETRIC_MINISBLACK) std::cerr << "Only Gray, RGB photometric is supported" << std::endl;
    if(compression != COMPRESSION_NONE && compression != COMPRESSION_JPEG && compression != COMPRESSION_PNG && compression != COMPRESSION_LZW) std::cerr << "Compression not supported" << std::endl;

    if(photometric == PHOTOMETRIC_RGB && compression == COMPRESSION_JPEG) photometric = PHOTOMETRIC_YCBCR;

    if(photometric == PHOTOMETRIC_MINISBLACK) samplesperpixel = 1;
    else samplesperpixel = 3;

// output opening
    output.open(filename, std::ios_base::trunc | std::ios::binary);
    if(!output) std::cerr << "Unable to open output file" << std::endl;

// numbers of tiles
    tilex = width / tilewidth;
    tiley = length / tilelength;

    // Choose default quality valu according to compression
    if(compression == COMPRESSION_JPEG && (quality < 0 || quality > 100)) quality = 75;
    if(compression == COMPRESSION_PNG && (quality < 0 || quality > 10)) quality = 5;

    char header[RESERVED_SIZE], *p = header;                           // Buffer of the hedear + IFD
    memset(header, 0, sizeof(header));

    *((uint16_t*) (p))      = 0x4949;            // Little Endian
    *((uint16_t*) (p += 2)) = 42;                // Tiff specification
    *((uint32_t*) (p += 2)) = 14;                // Offset of the IFD

// write the number of entries in the IFD

    *((uint16_t*) (p += 4)) = 8;  //
    *((uint16_t*) (p += 2)) = 8;  // RBG TIFFTAG_BITSPERSAMPLE values
    *((uint16_t*) (p += 2)) = 8;  //

    if(photometric == PHOTOMETRIC_YCBCR) // Number of tags
        *((uint16_t*) (p += 2)) = 12;
    else 
        *((uint16_t*) (p += 2)) = 11;

    *((uint16_t*) (p += 2)) = TIFFTAG_IMAGEWIDTH;      //
    *((uint16_t*) (p += 2)) = TIFF_LONG;               //
    *((uint32_t*) (p += 2)) = 1;                       //
    *((uint32_t*) (p += 4)) = width;                   //

    *((uint16_t*) (p += 4)) = TIFFTAG_IMAGELENGTH;     //
    *((uint16_t*) (p += 2)) = TIFF_LONG;               //
    *((uint32_t*) (p += 2)) = 1;                       //
    *((uint32_t*) (p += 4)) = length;                  //

    *((uint16_t*) (p += 4)) = TIFFTAG_BITSPERSAMPLE;   //
    *((uint16_t*) (p += 2)) = TIFF_SHORT;              //
    *((uint32_t*) (p += 2)) = samplesperpixel; 
    *((uint32_t*) (p += 4)) = bitspersample;            // 8/32 = value for 1 sample per pixel or 8 = pointer for 3 samples per pixel

    *((uint16_t*) (p += 4)) = TIFFTAG_COMPRESSION;     //
    *((uint16_t*) (p += 2)) = TIFF_SHORT;              //
    *((uint32_t*) (p += 2)) = 1;                       //
    *((uint32_t*) (p += 4)) = compression;             //

    *((uint16_t*) (p += 4)) = TIFFTAG_PHOTOMETRIC;     //
    *((uint16_t*) (p += 2)) = TIFF_SHORT;              //
    *((uint32_t*) (p += 2)) = 1;                       //
    *((uint32_t*) (p += 4)) = photometric;             //

    *((uint16_t*) (p += 4)) = TIFFTAG_SAMPLESPERPIXEL; //
    *((uint16_t*) (p += 2)) = TIFF_SHORT;              //
    *((uint32_t*) (p += 2)) = 1;                       //
    *((uint32_t*) (p += 4)) = samplesperpixel;         //

    *((uint16_t*) (p += 4)) = TIFFTAG_TILEWIDTH;       //
    *((uint16_t*) (p += 2)) = TIFF_LONG;               //
    *((uint32_t*) (p += 2)) = 1;                       //
    *((uint32_t*) (p += 4)) = tilewidth;               //

    *((uint16_t*) (p += 4)) = TIFFTAG_TILELENGTH;      //
    *((uint16_t*) (p += 2)) = TIFF_LONG;               //
    *((uint32_t*) (p += 2)) = 1;                       //
    *((uint32_t*) (p += 4)) = tilelength;              //

    *((uint16_t*) (p += 4)) = TIFFTAG_TILEOFFSETS;     //
    *((uint16_t*) (p += 2)) = TIFF_LONG;               //
    *((uint32_t*) (p += 2)) = tilex*tiley;             //
    if (tilex * tiley == 1) {
/* Dans le cas d'une tuile unique, le champs contient directement la valeur et pas l'adresse de la valeur.
 * Cependant, étant donnée le mode de foncionnement de Rok4, on doit laisser la valeur au début de l'image.
 * Voilà pourquoi on ajoute 8 à RESERVED_SIZE : 4 pour le TileOffset et 4 pour le TileByteCount.
 */     *((uint32_t*) (p += 4)) = RESERVED_SIZE+8;           //
    } else {
        *((uint32_t*) (p += 4)) = RESERVED_SIZE;           //
    }

    *((uint16_t*) (p += 4)) = TIFFTAG_TILEBYTECOUNTS;  //
    *((uint16_t*) (p += 2)) = TIFF_LONG;               //
    *((uint32_t*) (p += 2)) = tilex*tiley;             //
    *((uint32_t*) (p += 4)) = RESERVED_SIZE + 4*tilex*tiley;//

    *((uint16_t*) (p += 4)) = TIFFTAG_SAMPLEFORMAT;  //
    *((uint16_t*) (p += 2)) = TIFF_SHORT;            //
    *((uint32_t*) (p += 2)) = 1;             //
    *((uint32_t*) (p += 4)) = sampleformat;//

    if(photometric == PHOTOMETRIC_YCBCR) {
        *((uint16_t*) (p += 4)) = TIFFTAG_YCBCRSUBSAMPLING;  //
        *((uint16_t*) (p += 2)) = TIFF_SHORT;              //
        *((uint32_t*) (p += 2)) = 2;                       //
        *((uint16_t*) (p += 4)) = 2;                       //
        *((uint16_t*) (p + 2))  = 2;                       // FIXME : ce serait pas p+=2 ?
    }

    *((uint32_t*) (p += 4)) = 0;                       // end of IFD
    output.write(header, sizeof(header));

// variables initalizations
    TileOffset = new uint32_t[tilex*tiley];
    TileByteCounts = new uint32_t[tilex*tiley];
    memset(TileOffset, 0, tilex*tiley*4);
    memset(TileByteCounts, 0, tilex*tiley*4);
    position = sizeof(header) + 8*tilex*tiley;

    tilelinesize = tilewidth*samplesperpixel*bitspersample/8;
    rawtilesize = tilelinesize*tilelength;

    Buffer = new uint8_t[2*rawtilesize];

// z compression initalization
    if(compression == COMPRESSION_PNG) {
        PNG_buffer = new uint8_t[rawtilesize + tilelength];
        zstream.zalloc = Z_NULL;
        zstream.zfree  = Z_NULL;
        zstream.opaque = Z_NULL;
        zstream.data_type = Z_BINARY;
        deflateInit(&zstream, quality);
    }

    if(compression == COMPRESSION_JPEG) {
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);

        cinfo.dest = new jpeg_destination_mgr;  
        cinfo.dest->init_destination = init_destination;
        cinfo.dest->empty_output_buffer = empty_output_buffer;
        cinfo.dest->term_destination = term_destination;   

        cinfo.image_width  = tilewidth;
        cinfo.image_height = tilelength;  
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB; // TODO : Jpeg gris

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality (&cinfo, quality, true);
    }

}

int TiledTiffWriter::close() {
    output.seekp(RESERVED_SIZE);
    output.write((char*) TileOffset, 4*tilex*tiley);
    output.write((char*) TileByteCounts, 4*tilex*tiley);
    output.close();
    if(output.fail()) return -1;

    delete[] TileOffset;
    delete[] TileByteCounts;
    delete[] Buffer;
    if(compression == COMPRESSION_PNG) {
        delete[] PNG_buffer;
        deflateEnd(&zstream);
    } 
    if(compression == COMPRESSION_JPEG) {
        jpeg_destroy_compress(&cinfo);
    }
    return 1;
};

size_t TiledTiffWriter::computeRawTile(uint8_t *buffer, uint8_t *data) {
    memcpy(buffer, data, rawtilesize);
    return rawtilesize; 
}

size_t TiledTiffWriter::computeLzwTile(uint8_t *buffer, uint8_t *data) {
    return lzw_encode(data, rawtilesize, buffer);
}


size_t TiledTiffWriter::computePngTile(uint8_t *buffer, uint8_t *data) {
    uint8_t *B = PNG_buffer; 
    for(unsigned int h = 0; h < tilelength; h++) {
        *B++ = 0; // on met un 0 devant chaque ligne (spec png -> mode de filtrage simple)
        memcpy(B, data + h*tilelinesize, tilelinesize);
        B += tilelinesize;
    }

    memcpy(buffer, PNG_HEADER, sizeof(PNG_HEADER));
    *((uint32_t*)(buffer+16)) = bswap_32(tilewidth);
    *((uint32_t*)(buffer+20)) = bswap_32(tilelength);
    if(photometric == PHOTOMETRIC_MINISBLACK) buffer[25] = 0; // gray
    else buffer[25] = 2;                                      // RGB

    uint32_t crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, buffer + 12, 17);
    *((uint32_t*)(buffer+29)) = bswap_32(crc);

    zstream.next_out  = buffer + sizeof(PNG_HEADER) + 8;
    zstream.avail_out = 2*rawtilesize - 12 - sizeof(PNG_HEADER) - sizeof(PNG_IEND);
    zstream.next_in   = PNG_buffer;
    zstream.avail_in  = rawtilesize + tilelength;

    if (deflateReset(&zstream) != Z_OK) return -1;
    if (deflate(&zstream, Z_FINISH) != Z_STREAM_END) return -1;

    *((uint32_t*)(buffer+sizeof(PNG_HEADER))) =  bswap_32(zstream.total_out);
    buffer[sizeof(PNG_HEADER) + 4] = 'I';
    buffer[sizeof(PNG_HEADER) + 5] = 'D';
    buffer[sizeof(PNG_HEADER) + 6] = 'A';
    buffer[sizeof(PNG_HEADER) + 7] = 'T';

    crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, buffer + sizeof(PNG_HEADER) + 4, zstream.total_out+4);
    *((uint32_t*) zstream.next_out) = bswap_32(crc);

    memcpy(zstream.next_out + 4, PNG_IEND, sizeof(PNG_IEND));
    return zstream.total_out + 12 + sizeof(PNG_IEND) + sizeof(PNG_HEADER);
}


size_t TiledTiffWriter::computeJpegTile(uint8_t *buffer, uint8_t *data) {
    cinfo.dest->next_output_byte = buffer;
    cinfo.dest->free_in_buffer = 2*rawtilesize;
    jpeg_start_compress(&cinfo, true);
    while (cinfo.next_scanline < cinfo.image_height) {
        uint8_t *line = data + cinfo.next_scanline*tilelinesize;
        if(jpeg_write_scanlines(&cinfo, &line, 1) != 1) return 0;
    }
    jpeg_finish_compress(&cinfo);
    return 2*rawtilesize - cinfo.dest->free_in_buffer;
}


int TiledTiffWriter::WriteTile(int n, uint8_t *data) {

    if(n > tilex*tiley || n < 0) {std::cerr << "invalid tile number" << std::endl; return -1;}
    size_t size;

    switch(compression) {
        case COMPRESSION_NONE: size = computeRawTile(Buffer, data); break;
        case COMPRESSION_LZW : size = computeLzwTile(Buffer, data); break;
        case COMPRESSION_JPEG: size = computeJpegTile(Buffer, data); break;
        case COMPRESSION_PNG : size = computePngTile(Buffer, data); break;
    }
    if(size == 0) return -1;
    
    if (tilex*tiley == 1) {
        output.seekp(132);
        uint32_t Size[1];
        Size[0] = (uint32_t) size;
        output.write((char*) Size,4);
    }
    
    TileOffset[n] = position;
    TileByteCounts[n] = size;
    output.seekp(position);
    output.write((char*) Buffer, size);
    if(output.fail()) return -1;
    position = (position + size + 15) & ~15; // Align the next position on 16byte

    return 1;  
}

int TiledTiffWriter::WriteTile(int x, int y, uint8_t *data) {
    return WriteTile(y*tilex + x, data);
}
