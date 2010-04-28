#include <iostream>

#include "BilEncoder.h"
#include "pixel_type.h"
#include "Logger.h"

template<typename pixel_t>
size_t BilEncoder<pixel_t>::getdata(uint8_t *buffer, size_t size) {
  size_t offset = 0;

  LOGGER(DEBUG) << "Encodage BIL : height " <<image->height<< " image->linesize" <<image->linesize<<std::endl;

  for(; line < image->height && offset + image->linesize <= size; line++) {
    image->getline((typename pixel_t::data_t*)(buffer + offset), line);    
    offset += image->linesize*sizeof(typename pixel_t::data_t);
  }

  return offset;
}

template<typename pixel_t>
BilEncoder<pixel_t>::~BilEncoder() {
  LOGGER(DEBUG) << "delete BilEncoder" << std::endl;
  delete image;
}

template class BilEncoder<pixel_rgb>;
template class BilEncoder<pixel_rgba>;
template class BilEncoder<pixel_gray>;
template class BilEncoder<pixel_float>;

