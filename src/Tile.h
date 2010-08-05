#ifndef TILE_H
#define TILE_H

#include <iostream>
#include "Logger.h"
#include "Convert.h"
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

  Tile(int tile_width, int tile_height, int channels, StaticHttpResponse* data, int left, int top, int right, int bottom, int coding) :
  Image(tile_width - left - right, tile_height - top - bottom, channels), data(data), tile_width(tile_width), tile_height(tile_height), left(left), top(top), coding(coding) {
    raw_data = new uint8_t[tile_width * tile_height * channels];
    size_t encoded_size;
    const uint8_t* encoded_data = data->get_data(encoded_size);
    LOGGER_DEBUG( " Tile " << width << " " << height << " " << channels << " " << (void *) encoded_data << " " << encoded_size );

    if (coding==RAW_UINT8 || coding==RAW_FLOAT) {
	RawDecoder::decode(encoded_data, encoded_size, raw_data);
    }
    else if (coding==JPEG_UINT8) {
	JpegDecoder::decode(encoded_data, encoded_size, raw_data,tile_height,tile_width*channels);
    }
    else if (coding==PNG_UINT8) {
	LOGGER_DEBUG("CHANNELS : " << channels);
        PngDecoder::decode(encoded_data, encoded_size, raw_data,tile_height,tile_width*channels);
    }
    else
	LOGGER_ERROR("Codage inconnu");

  //  Decoder::decode(encoded_data, encoded_size, raw_data);
    LOGGER_DEBUG( " Tile " << width << " " << height << " " << channels << " " << encoded_size );    
  }

  ~Tile() {
   delete[] raw_data;
   delete data;
  }
};


#endif
