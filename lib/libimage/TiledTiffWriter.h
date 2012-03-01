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

#ifndef _TILEDTIFFWRITER_
#define _TILEDTIFFWRITER_

#include <stdint.h>
#include <fstream>
#include "zlib.h"
#include <jpeglib.h>
#include <tiff.h>

//#define TIFF_SHORT              3       /* 16-bit unsigned integer */
//#define TIFF_LONG               4       /* 32-bit unsigned integer */
//#define TIFF_RATIONAL           5       /* 64-bit unsigned fraction */


#define TIFFTAG_SUBFILETYPE     254     /* subfile data descriptor */
#define	TIFFTAG_IMAGEWIDTH	256	/* image width in pixels */
#define	TIFFTAG_IMAGELENGTH	257	/* image height in pixels */
#define	TIFFTAG_BITSPERSAMPLE	258	/* bits per channel (sample) */
#define	TIFFTAG_COMPRESSION	259	/* data compression technique */
#define	TIFFTAG_PHOTOMETRIC	262	/* photometric interpretation */
#define	TIFFTAG_SAMPLESPERPIXEL	277	/* samples per pixel */
#define TIFFTAG_XRESOLUTION     282     /* pixels/resolution in x */
#define TIFFTAG_YRESOLUTION     283     /* pixels/resolution in y */
#define TIFFTAG_PLANARCONFIG    284     /* storage organization */
#define TIFFTAG_RESOLUTIONUNIT  296     /* units of resolutions */
#define	TIFFTAG_TILEWIDTH	322	/* !tile width in pixels */
#define	TIFFTAG_TILELENGTH	323	/* !tile height in pixels */
#define TIFFTAG_TILEOFFSETS	324	/* !offsets to data tiles */
#define TIFFTAG_TILEBYTECOUNTS	325	/* !byte counts for tiles */

#define TIFFTAG_EXTRASAMPLES	338	
#define TIFFTAG_SAMPLEFORMAT    339     /* !data sample format */

#define	TIFFTAG_YCBCRSUBSAMPLING	530	/* !YCbCr subsampling factors */


#define	COMPRESSION_NONE	1	/* dump mode */
#define COMPRESSION_JPEG        7       /* %JPEG DCT compression */
#define	COMPRESSION_PNG         8   	/* Zlib compression PNG spec */
#define	COMPRESSION_LZW         5   	/* liblzw */

#define	PHOTOMETRIC_MINISBLACK	1	/* min value is black */
#define	PHOTOMETRIC_RGB		2	/* RGB color model */
#define PHOTOMETRIC_YCBCR       6       /* !CCIR 601 */

#define RESERVED_SIZE             2048

#define     SAMPLEFORMAT_UINT           1       /* !unsigned integer data */
#define     SAMPLEFORMAT_IEEEFP         3       /* !IEEE floating point data */


/*
 * Tiff file structure :
 * 0    : [header 8 Bytes] 
 * 8    : [IFD (RESERVED_SIZE - 8) bytes]   // wifth some reserved an unused space
 * 
 * RESERVED_SIZE: [TileOffset 4*nbtile Bytes]
 *       	  [TileByteCount 4*nbtile Bytes] 
 * [Tile Data]
 */



class TiledTiffWriter {
  private:

    uint32_t width;        // TIFFTAG_IMAGEWIDTH
    uint32_t length;       // TIFFTAG_IMAGELENGTH

    uint16_t photometric;  // TIFFTAG_PHOTOMETRIC
    uint16_t compression;  // TIFFTAG_COMPRESSION
    uint16_t samplesperpixel;
    
    int quality;           // compression quality (jpeg or zlib)

    uint32_t tilewidth;    // TIFFTAG_TILEWIDTH
    uint32_t tilelength;   // TIFFTAG_TILELENGTH

    uint32_t bitspersample;
    uint16_t sampleformat;  // TIFFTAG_SAMPLEFORMAT

    int tilex;        // = width / tilewidth
    int tiley;        // = length / tilelength

    uint32_t position;     // current position
    uint32_t *TileOffset; 
    uint32_t *TileByteCounts;

    int tilelinesize; // size of an uncompressed tile line in bytes
    int rawtilesize;  // size of an uncompressed tile in bytes

    std::ofstream output;  // tiff file output stream

    size_t BufferSize;
    uint8_t* Buffer, *PNG_buffer;
    z_stream zstream;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr       jerr;


//    struct jpeg_compress_struct cinfo;
//    struct jpeg_error_mgr       jerr;

    size_t computeRawTile (uint8_t *buffer, uint8_t *data);
    size_t computeJpegTile(uint8_t *buffer, uint8_t *data, bool crop);
    void emptyWhiteBlock(uint8_t *buffheight, int l);
    size_t computeLzwTile(uint8_t *buffer, uint8_t *data);
    size_t computePngTile (uint8_t *buffer, uint8_t *data);
  public: 

    /*
     * Constrctor
     * Open a new tiff file and write header and IFD
     */
    TiledTiffWriter(const char *filename, uint32_t width, uint32_t length, uint16_t photometric,
        uint16_t compression, int quality, uint32_t tilewidth, uint32_t tilelength, uint32_t bitspersample, uint16_t samplesperpixel, uint16_t sampleformat);

    /*
     * Write tileoffset and tilebytecounts then close file
     * The tiff file is not valid until this function is called
     * This function can be called only once and no data can be written after.
     */
    int close();

    /*
     * Write a tile from uncompressed data
     * data must contain more than tilewidth*tilelength*samplesperpixel bytes
     */
    int WriteTile(int n, uint8_t *data, bool crop);      
    int WriteTile(int x, int y, uint8_t *data, bool crop = false);

};

#endif // _TILEDTIFFWRITER_
