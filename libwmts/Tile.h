#ifndef TILE_H
#define TILE_H

#include <iostream>
#include "Logger.h"
#include "Utils.h"
#include "Level.h"

typedef enum {
        RAW_UINT8,
        RAW_FLOAT,
        JPEG_UINT8,
        PNG_UINT8
}       TILE_CODING;


class RawDecoder {
  public:
  static void decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data) {
    memcpy(raw_data, encoded_data, encoded_size);
  }
};


class JpegDecoder {
  public:
  static void decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data, int height, int linesize);
};

class PngDecoder {
  public:
  static void decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data, int height, int linesize);
};

class Tile : public Image {
  private:

  StaticHttpResponse *data;
  uint8_t* raw_data;

  int tile_width;
  int tile_height;
  int left;
  int top;
  int coding;

  public:
  int getline(uint8_t* buffer, int line) {
    convert(buffer, raw_data + ((top + line) * tile_width + left) * channels, width * channels);
    return width * channels;
  }

  int getline(float* buffer, int line) {
    convert(buffer, raw_data + ((top + line) * tile_width + left) * channels, width * channels);
    return width * channels;
  }

  Tile(int tile_width, int tile_height, int channels, StaticHttpResponse* data, int left, int top, int right, int bottom, int coding);

  ~Tile() {
   delete[] raw_data;
   delete data;
  }
};


#endif
