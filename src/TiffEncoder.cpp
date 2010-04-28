#include <iostream>

#include "TiffEncoder.h"
#include "pixel_type.h"
#include "Logger.h"

template<typename pixel_t>
size_t TiffEncoder<pixel_t>::getdata(uint8_t *buffer, size_t size) {
  size_t offset = 0;
  if(line == -1) { // écire le header tiff
    // Si pas assez de place pour le header, ne rien écrire.
    if(size < sizeof(pixel_t::TIFF_HEADER)) return 0;

    // Ceci est du tiff avec une seule strip.
    memcpy(buffer, pixel_t::TIFF_HEADER, sizeof(pixel_t::TIFF_HEADER));
    *((uint32_t*)(buffer+18))  = image->width;
    *((uint32_t*)(buffer+30))  = image->height;
    *((uint32_t*)(buffer+102)) = image->height;
    *((uint32_t*)(buffer+114)) = image->height*image->linesize;
    offset = sizeof(pixel_t::TIFF_HEADER);
    line = 0;
  }
  
  for(; line < image->height && offset + image->linesize <= size; line++) {
//   LOGGER(DEBUG) << "line : " << line << " " << offset << std::endl;
    image->getline((typename pixel_t::data_t*)(buffer + offset), line);    
    offset += image->linesize*sizeof(typename pixel_t::data_t);
  //  LOGGER(DEBUG) << "line : " << line << " " << offset << std::endl;
  }

  return offset;
}

template<typename pixel_t>
TiffEncoder<pixel_t>::~TiffEncoder() {
  LOGGER(DEBUG) << "delete TiffEncoder" << std::endl;
  delete image;
}

template class TiffEncoder<pixel_rgb>;
template class TiffEncoder<pixel_gray>;
template class TiffEncoder<pixel_float>;
