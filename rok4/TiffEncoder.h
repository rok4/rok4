#ifndef _TIFFENCODER_
#define _TIFFENCODER_

#include "Data.h"
#include "Image.h"

class TiffEncoder : public DataStream {
	Image *image;
  	int line;	// Ligne courante

  	public:
  	TiffEncoder(Image *image) : image(image), line(-1) {}
  	~TiffEncoder();
  	size_t read(uint8_t *buffer, size_t size);
	bool eof();
	std::string gettype() {return "image/tif";}
};

const uint8_t TIFF_HEADER_RGB[128]  = {
                73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
                9, 0,                                          // 8  | nombre de tags sur 16 bits (10)
                // .. | TIFFTAG              | DATA TYPE | NUMBER | VALUE
                0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
                1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
                2, 1,   3, 0,   3, 0, 0, 0,   122,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
                3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
                6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
                17,1,   4, 0,   1 ,0, 0, 0,   128,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 128
                21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
                22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
                23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
                0, 0, 0, 0,                                    // 118| fin de l'IFD
                8, 0,   8, 0,   8, 0};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128

const uint8_t TIFF_HEADER_RGBA[128]  = { //FIXME
                73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
                9, 0,                                          // 8  | nombre de tags sur 16 bits (10)
                // .. | TIFFTAG              | DATA TYPE | NUMBER | VALUE
                0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
                1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
                2, 1,   3, 0,   3, 0, 0, 0,   122,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
                3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
                6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
                17,1,   4, 0,   1 ,0, 0, 0,   128,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 128
                21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
                22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
                23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
                0, 0, 0, 0,                                    // 118| fin de l'IFD
                8, 0,   8, 0,   8, 0};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128

const uint8_t TIFF_HEADER_GRAY[128]  = {
                73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
                9, 0,                                          // 8  | nombre de tags sur 16 bits (10)
                // .. | TIFFTAG              | DATA TYPE | NUMBER | VALUE
                0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
                1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
                2, 1,   3, 0,   1, 0, 0, 0,   8, 0, 0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers un bloc mémoire 8
                3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
                6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
                17,1,   4, 0,   1 ,0, 0, 0,   128,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 128
                21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
                22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
                23,1,   4, 0,   1, 0, 0, 0,   0, 0, 1, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 1
                0, 0, 0, 0,                                    // 118| fin de l'IFD
                8, 0,   8, 0,   8, 0};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128

#endif


