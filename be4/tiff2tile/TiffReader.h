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
    uint32_t bitspersample;

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
