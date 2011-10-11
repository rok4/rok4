#ifndef COLORIZEPNGENCODER_H
#define COLORIZEPNGENCODER_H
#include <PNGEncoder.h>

#include "PaletteConfig.h"

class ColorizePNGEncoder : public PNGEncoder {
private:
	bool transparent;
	uint8_t PLTE[3*256+12];

public:
	  ColorizePNGEncoder(Image *image, bool transparent = true, const uint8_t rgb[3] = BLACK);
	  ~ColorizePNGEncoder();
protected:
	virtual size_t write_IHDRP(uint8_t *buffer, size_t size, uint8_t colortype/* = pixel_t::png_colortype*/);
	virtual size_t write_PLTE(uint8_t *buffer, size_t size);
	virtual size_t write_tRNS(uint8_t *buffer, size_t size);
	
	virtual size_t read(uint8_t* buffer, size_t size);
	virtual bool eof();
};

/*
    static const uint8_t BLACK[3] = {0, 0, 0};
    class ColorizePNGEncoder : public PNGEncoder<pixel_gray> {
      private:
      bool transparent;
      uint8_t PLTE[3*256+12];

      size_t write_PLTE(uint8_t *buffer, size_t size);
      size_t write_tRNS(uint8_t *buffer, size_t size);
      public:
      ColorizePNGEncoder(Image<pixel_gray> *image, bool transparent = true, const uint8_t rgb[3] = BLACK);
      ~ColorizePNGEncoder();
      size_t getdata(uint8_t *buffer, size_t size);

    };
*/


#endif // COLORIZEPNGENCODER_H
