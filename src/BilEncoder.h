#ifndef _BILENCODER_
#define _BILENCODER_

#include "HttpResponse.h"
//#include "Pixel_type.h"
#include "Image.h"

template<typename pixel_t>
class BilEncoder : public HttpResponse {
  Image<pixel_t> *image;
  int line;

  public:
  BilEncoder(Image<pixel_t> *image) : HttpResponse("image/bil", image->height * image->linesize), image(image), line(-1) {
    assert(image);
    std::cerr << "Bil" << std::endl;
    line=0;    
    }
  ~BilEncoder();
  size_t getdata(uint8_t *buffer, size_t size);
};
#endif


