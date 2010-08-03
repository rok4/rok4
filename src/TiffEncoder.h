#ifndef _TIFFENCODER_
#define _TIFFENCODER_

#include "HttpResponse.h"
//#include "Pixel_type.h"
#include "Image.h"

class TiffEncoder : public HttpResponse {
  Image *image;
  int line;

  public:
  TiffEncoder(Image *image) : HttpResponse("image/tiff"), image(image), line(-1) {
    //assert(image);
    //std::cerr << "Tiff" << std::endl;
    }
  ~TiffEncoder();
  size_t getdata(uint8_t *buffer, size_t size);
};
#endif


