#ifndef _TIFFENCODER_
#define _TIFFENCODER_

#include "Data.h"
#include "Tile.h"

class TiffEncoderStream : public DataStream {
	Image *image;
  	int line;	// Ligne courante

  	public:
  	TiffEncoderStream(Image *image) : image(image), line(-1) {}
  	~TiffEncoderStream();
  	size_t read(uint8_t *buffer, size_t size);
	bool eof();
	std::string gettype() {return "image/tiff";}
};

class TiffEncoderSource : public DataSource {
        Tile *tile;
	uint8_t *tif_data;
	size_t size;
        public:
        TiffEncoderSource(Tile *tile);
        ~TiffEncoderSource();
	const uint8_t* get_data(size_t &size);
	bool release_data();
        std::string gettype() {return "image/tiff";}
};

#endif


