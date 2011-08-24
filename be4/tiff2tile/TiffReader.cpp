#include <cstdlib>
#include <iostream>
#include <string.h>
#include "TiffReader.h"

TiffReader::TiffReader(const char* filename) {
  input = TIFFOpen(filename, "r");
  if(!input) {std::cerr << "Unable to open file" << std::endl; exit(2);}

  TIFFGetField(input, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(input, TIFFTAG_IMAGELENGTH, &length);
  TIFFGetField(input, TIFFTAG_PHOTOMETRIC, &photometric);

  uint16_t sampleperpixel;
  TIFFGetField(input, TIFFTAG_BITSPERSAMPLE, &bitspersample);
  TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel);

  sampleSize = (bitspersample * sampleperpixel) / 8;

  if(TIFFIsTiled(input)) {
    TIFFGetField(input, TIFFTAG_TILEWIDTH, &tileWidth);
    TIFFGetField(input, TIFFTAG_TILELENGTH, &tileLength);
    BufferSize = 2*length/tileLength;
    LineBuffer = new uint8_t[sampleSize*length];
  }
  else {
    BufferSize = 512;
    tileWidth = tileLength = 0;
    LineBuffer = 0;
  }

  _Buffer = new uint8_t*[BufferSize];
  memset(_Buffer, 0, BufferSize*sizeof(uint8_t*));
  Buffer = new uint8_t*[length];
  memset(Buffer, 0, length*sizeof(uint8_t*));
  BIndex = new int[BufferSize];
  memset(BIndex, -1, BufferSize*sizeof(int));
  Buffer_pos = 0;
}

uint8_t* TiffReader::getRawLine(uint32_t line) {
  if(!Buffer[line]) {
    if(!_Buffer[Buffer_pos]) _Buffer[Buffer_pos] = new uint8_t[width*sampleSize];
    if(BIndex[Buffer_pos] != -1) Buffer[BIndex[Buffer_pos]] = 0;    
    BIndex[Buffer_pos] = line;
    Buffer[line] = _Buffer[Buffer_pos];
    if(TIFFReadScanline(input, Buffer[line], line) == -1) {std::cerr << "Unable to read tiff line" << std::endl; exit(2);}
    Buffer_pos = (Buffer_pos + 1) % BufferSize;
  }
  return Buffer[line];
}

uint8_t* TiffReader::getRawTile(uint32_t tile) {
  if(!Buffer[tile]) {
    uint32_t tileSize = tileWidth*tileLength*sampleSize;
    if(!_Buffer[Buffer_pos]) _Buffer[Buffer_pos] = new uint8_t[tileSize];
    if(BIndex[Buffer_pos] != -1) Buffer[BIndex[Buffer_pos]] = 0;
    BIndex[Buffer_pos] = tile;
    Buffer[tile] = _Buffer[Buffer_pos];
    if(TIFFReadEncodedTile(input, tile, Buffer[tile], tileSize) == -1) {std::cerr << "Unable to read tiff tile" << std::endl; exit(2);}
    Buffer_pos = (Buffer_pos + 1) % BufferSize;
  }
  return Buffer[tile];
}

uint8_t* TiffReader::getEncodedTile(uint32_t tile)
{
    if(!Buffer[tile]) {
    uint32_t tileSize = TIFFTileSize(input);
    if(!_Buffer[Buffer_pos]) _Buffer[Buffer_pos] = new uint8_t[tileSize];
    if(BIndex[Buffer_pos] != -1) Buffer[BIndex[Buffer_pos]] = 0;
    BIndex[Buffer_pos] = tile;
    Buffer[tile] = _Buffer[Buffer_pos];
    if(TIFFReadRawTile(input, tile, Buffer[tile], tileSize) == -1) {std::cerr << "Unable to read tiff tile" << std::endl; exit(2);}
    Buffer_pos = (Buffer_pos + 1) % BufferSize;
  }
  return Buffer[tile];
}

uint8_t* TiffReader::getLine(uint32_t line, uint32_t offset, uint32_t size) {
  if(offset > length) offset = length;
  if(size > length - offset) size = length - offset;
  if(tileWidth) { // tilled tiff   
    int xmin = offset / tileWidth;
    int xmax = (offset + size - 1) / tileWidth;
    int n = (line / tileLength) * ((width + tileWidth - 1) / tileWidth) + xmin; // tile number.
    int tileLineSize = tileLength*sampleSize;
    int tileOffset = (line % tileLength) * tileLineSize;
    for(int x = xmin; x <= xmax; x++) {
      uint8_t *tile = getRawTile(n++);
      memcpy(LineBuffer + tileLineSize*x, tile + tileOffset, tileLineSize);
    }
    return LineBuffer + offset*sampleSize;    
  }
  else { // scanline tiff
    return getRawLine(line) + offset*sampleSize;
  }
}

int TiffReader::getWindow(int offsetx, int offsety, int w, int l, uint8_t *buffer) {
  for(int y = 0; y < l; y++) {
    uint8_t* data = getLine(offsety + y, offsetx, w);
    if(!data) return -1;
    memcpy(buffer + y*w*sampleSize, data, w*sampleSize);
  }
  return 1;
}

void TiffReader::close() {
  TIFFClose(input);
  for(int i = 0; i < BufferSize; i++) if(_Buffer[i]) delete[] _Buffer[i];
  if(LineBuffer) delete[] LineBuffer;
  delete[] _Buffer;
  delete[] Buffer;
  delete[] BIndex;
}

