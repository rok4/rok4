#include "TiffHeaderDataSource.h"

const size_t header_size = 128;

const uint8_t TIFF_HEADER_RAW_INT8_RGB[128]  = {
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

const uint8_t TIFF_HEADER_RAW_INT8_RGBA[128]  = { //FIXME
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

const uint8_t TIFF_HEADER_RAW_INT8_GRAY[128]  = {
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

const uint8_t TIFF_HEADER_RAW_FLOAT32_GRAY[128]  = {
                73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
                9, 0,                                          // 8  | nombre de tags sur 16 bits (10)
                // .. | TIFFTAG              | DATA TYPE | NUMBER | VALUE
                0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
                1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
                2, 1,   3, 0,   1, 0, 0, 0,   32, 0, 0, 0,     // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 1      | pointeur vers 1 bloc mémoire 32
                3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 1 (pas de compression)
                6, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 1 (black is zero)
                17,1,   4, 0,   1 ,0, 0, 0,   128,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 128
                21,1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 1
                22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
                23,1,   4, 0,   1, 0, 0, 0,   0, 0, 1, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 1
                0, 0, 0, 0,                                    // 118| fin de l'IFD
                8, 0,   8, 0,   8, 0};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
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
                8, 0,   8, 0,   8, 0};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128

const uint8_t TIFF_HEADER_LZW_INT8_RGBA[128]  = { //FIXME
                73,73,  42,0,   8 ,0,   0, 0,                  // 0  | tiff header 'II' (Little endian) + magick number (42) + offset de la IFD (16)
                9, 0,                                          // 8  | nombre de tags sur 16 bits (10)
                // .. | TIFFTAG              | DATA TYPE | NUMBER | VALUE
                0, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 10 | IMAGEWIDTH      (256)| LONG  (4) | 1      | 256
                1, 1,   4, 0,   1, 0, 0, 0,   0, 1, 0, 0,      // 22 | IMAGELENGTH     (257)| LONG  (4) | 1      | 256
                2, 1,   3, 0,   3, 0, 0, 0,   122,0,0, 0,      // 34 | BITSPERSAMPLE   (258)| SHORT (3) | 3      | pointeur vers un bloc mémoire 8,8,8
                3, 1,   3, 0,   1, 0, 0, 0,   1, 0, 0, 0,      // 46 | COMPRESSION     (259)| SHORT (3) | 1      | 5 (LZW)
                6, 1,   3, 0,   1, 0, 0, 0,   2, 0, 0, 0,      // 58 | PHOTOMETRIC     (262)| SHORT (3) | 1      | 2 (RGB)
                17,1,   4, 0,   1 ,0, 0, 0,   128,0,0, 0,      // 70 | STRIPOFFSETS    (273)| LONG  (4) | 16     | 128
                21,1,   3, 0,   1, 0, 0, 0,   3, 0, 0, 0,      // 82 | SAMPLESPERPIXEL (277)| SHORT (3) | 1      | 3
                22,1,   4, 0,   1, 0, 0, 0,   255,255,255,255, // 94 | ROWSPERSTRIP    (278)| LONG  (4) | 1      | 2^32-1 = single strip tiff
                23,1,   4, 0,   1, 0, 0, 0,   0, 0, 3, 0,      // 106| STRIPBYTECOUNTS (279)| LONG  (4) | 1      | 256 * 256 * 3
                0, 0, 0, 0,                                    // 118| fin de l'IFD
                8, 0,   8, 0,   8, 0};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128



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
                8, 0,   8, 0,   8, 0};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
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
                8, 0,   8, 0,   8, 0};                         // 122| 3x 8 sur 16 bits (pointés par les samplesperpixels)
// 128


TiffHeaderDataSource::TiffHeaderDataSource(DataSource* dataSource,
					   eformat_data format, int channel,
					   int width, int height, size_t tileSize) : 
					   dataSource(dataSource), 
					   format(format), channel(channel),
					   width(width), height(height) , tileSize(tileSize)
{
	
	const uint8_t* tmp;
	dataSize = header_size;
	if (dataSource) {
		tmp = dataSource->getData(tileSize);
		dataSize+= tileSize;
	}
	
	data = new uint8_t[dataSize];
	
	switch (format) {
		case TIFF_RAW_INT8:
			if (channel == 1) {
				LOGGER_DEBUG("TIFF_HEADER_RAW_INT8_GRAY");
				memcpy(data, TIFF_HEADER_RAW_INT8_GRAY, header_size);
			}
			else if (channel == 3){
				LOGGER_DEBUG("TIFF_HEADER_RAW_INT8_RGB");
				memcpy(data, TIFF_HEADER_RAW_INT8_RGB, header_size);
			}
			else if (channel == 4){
				LOGGER_DEBUG("TIFF_HEADER_RAW_INT8_RGBA");
				memcpy(data, TIFF_HEADER_RAW_INT8_RGBA, header_size);
			}
			break;
		case TIFF_LZW_INT8:
			if (channel == 1) {
				LOGGER_DEBUG("TIFF_HEADER_LZW_INT8_GRAY");
				memcpy(data, TIFF_HEADER_LZW_INT8_GRAY, header_size);
			}
			else if (channel == 3){
				LOGGER_DEBUG("TIFF_HEADER_LZW_INT8_RGB");
				memcpy(data, TIFF_HEADER_LZW_INT8_RGB, header_size);
			}
			else if (channel == 4){
				LOGGER_DEBUG("TIFF_HEADER_LZW_INT8_RGBA");
				memcpy(data, TIFF_HEADER_LZW_INT8_RGBA, header_size);
			}
			break;			
		case TIFF_RAW_FLOAT32:
			if (channel == 1) {
				LOGGER_DEBUG("TIFF_HEADER_RAW_FLOAT32_GRAY");
				memcpy(data, TIFF_HEADER_RAW_FLOAT32_GRAY, header_size);
			}
			break;
		case TIFF_LZW_FLOAT32:
			if (channel == 1) {
				LOGGER_DEBUG("TIFF_HEADER_LZW_FLOAT32_GRAY");
				memcpy(data, TIFF_HEADER_LZW_FLOAT32_GRAY, header_size);
			}
			break;
	}
	*((uint32_t*)(data+18))  = width;
	*((uint32_t*)(data+30))  = height;
	*((uint32_t*)(data+102)) = height;
	*((uint32_t*)(data+114)) = tileSize;
	
	if (dataSource) {
		memcpy(data+header_size, tmp, tileSize);
	}
}




const uint8_t* TiffHeaderDataSource::getData(size_t& size)
{
	size = dataSize;
	return data;
}

TiffHeaderDataSource::~TiffHeaderDataSource()
{
	if (dataSource) {	
		dataSource->releaseData();
		delete dataSource;
	}
	if(data)
		delete[] data;
}
