#ifndef _TIFFENCODER_
#define _TIFFENCODER_

#include "HttpResponse.h"
//#include "Pixel_type.h"
#include "Image.h"

template<typename pixel_t>
class TiffEncoder : public HttpResponse {
  Image<pixel_t> *image;
  int line;

  public:
  TiffEncoder(Image<pixel_t> *image) : HttpResponse("image/tiff", sizeof(pixel_t::TIFF_HEADER) + image->height * image->linesize), image(image), line(-1) {
    assert(image);
    std::cerr << "Tiff" << std::endl;
    
    }
  ~TiffEncoder();
  size_t getdata(uint8_t *buffer, size_t size);
};
#endif


