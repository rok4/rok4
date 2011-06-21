#ifndef _PNGENCODER_
#define _PNGENCODER_

#include "Data.h"
#include "Image.h"
#include "zlib.h"

/** D */
class PNGEncoder : public DataStream {
private:
	Image *image;
	uint8_t* linebuffer;

	z_stream zstream;
	void addCRC(uint8_t *buffer, uint32_t length);

protected:
	int line;
	size_t write_IHDR(uint8_t *buffer, size_t size, uint8_t colortype/* = pixel_t::png_colortype*/);
	size_t write_IDAT(uint8_t *buffer, size_t size);
	size_t write_IEND(uint8_t *buffer, size_t size);

public:
	/** D */
	PNGEncoder(Image* image);
	/** D */
	~PNGEncoder();

	/** D */
	size_t read(uint8_t* buffer, size_t size);
	bool eof();

	std::string getType() {return "image/png";}

	int getHttpStatus() {return 200;}
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

#endif

