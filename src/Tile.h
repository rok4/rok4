#ifndef TILE_H
#define TILE_H

//#include "config.h"
#include "HttpResponse.h"
#include "Logger.h"
#include "Convert.h"


class RawDecoder {
  public:
  typedef uint8_t data_t;
  static void decode(const uint8_t* encoded_data, size_t encoded_size, data_t* raw_data) {
    LOGGER(DEBUG) << " decode " << encoded_size << std::endl;
    memcpy(raw_data, encoded_data, encoded_size);
    LOGGER(DEBUG) << " decode " << encoded_size << std::endl;
  }

};


template<class Decoder>
class Tile : public Image {
  private:
  typedef typename Decoder::data_t data_t;

  StaticHttpResponse *data;
  data_t* raw_data;

  int tile_width;
  int tile_height;
  int left;
  int top;

  public:
  int getline(uint8_t* buffer, int line) {
    return convert(buffer, raw_data + ((top + line) * tile_width + left) * channels, width * channels);
  }

  int getline(float* buffer, int line) {
    return convert(buffer, raw_data + ((top + line) * tile_width + left) * channels, width * channels);
  }

  Tile(int tile_width, int tile_height, int channels, StaticHttpResponse* data, int left, int top, int right, int bottom) :
  Image(tile_width - left - right, tile_height - top - bottom, channels), data(data), tile_width(tile_width), tile_height(tile_height), left(left), top(top) {
    LOGGER(DEBUG) << " Tile " << tile_width << " " << left << " " << right << std::endl;    
    LOGGER(DEBUG) << " Tile " << width << " " << height << " " << channels << std::endl;    
    raw_data = new data_t[tile_width * tile_height * channels];
    size_t encoded_size;
    const uint8_t* encoded_data = data->get_data(encoded_size);
    LOGGER(DEBUG) << " Tile " << width << " " << height << " " << channels << " " << (void *) encoded_data << " " << encoded_size << std::endl;
    Decoder::decode(encoded_data, encoded_size, raw_data);
    LOGGER(DEBUG) << " Tile " << width << " " << height << " " << channels << " " << encoded_size << std::endl;    
  }
};

template class Tile<RawDecoder>;

#endif
