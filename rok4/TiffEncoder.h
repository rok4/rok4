#ifndef _TIFFENCODER_
#define _TIFFENCODER_

#include "Data.h"
#include "Image.h"

class TiffEncoder : public DataStream {
	Image *image;
  	int line;

  	public:
  	TiffEncoder(Image *image) : image(image), line(-1) {}
  	~TiffEncoder();
  	size_t read(uint8_t *buffer, size_t size);
	bool eof();
	std::string gettype() {return "image/tif";}
};
#endif


