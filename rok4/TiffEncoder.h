#ifndef _TIFFENCODER_
#define _TIFFENCODER_

#include "Data.h"
#include "Image.h"

class TiffEncoderStream : public DataStream {
	Image *image;
  	int line;	// Ligne courante

  	public:
  	TiffEncoderStream(Image *image) : image(image), line(-1) {}
  	~TiffEncoderStream();
  	size_t read(uint8_t *buffer, size_t size);
	bool eof();
	std::string gettype() {return "image/tiff";}
	int getHttpStatus() {return 200;}	
};

class TiffEncoderSource : public DataSource {
	uint8_t *tif_data;	// Donnee de la tuile avec l'en-tete
	size_t size;
        public:
        TiffEncoderSource(int w, int h, int channels, DataSource* source);
        ~TiffEncoderSource();
	const uint8_t* getData(size_t &size);
	bool releaseData();
        std::string gettype() {return "image/tiff";}
	int getHttpStatus() {return 200;}
};

#endif


