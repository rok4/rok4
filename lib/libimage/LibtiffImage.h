#ifndef LIBTIFF_IMAGE_H
#define LIBTIFF_IMAGE_H

#include "Image.h"
#include "tiffio.h"
#include <string.h>

#define LIBTIFFIMAGE_MAX_FILENAME_LENGTH 512

class LibtiffImage : public Image {

	friend class libtiffImageFactory;

  	private:
  	TIFF* tif;
  	char* filename;
	uint16_t bitspersample;
	uint16_t photometric;
	uint16_t compression;

	protected:
  	/** Constructeur */
  	LibtiffImage(int width, int height, int channels, BoundingBox<double> bbox, TIFF* tif, char* filename, int bitspersample, int photometric, int compression);

  	public:

  	/** D */
  	int getline(uint8_t* buffer, int line);

  	/** D */
  	int getline(float* buffer, int line);

	void inline setfilename(char* str) {strcpy(filename,str);};
	inline char* getfilename() {return filename;}
	uint16_t inline getbitspersample() {return bitspersample;}
	uint16_t inline getphotometric() {return photometric;}
	uint16_t inline getcompression() {return compression;}

  	/** Destructeur */
  	~LibtiffImage();
};

class libtiffImageFactory {
  	public:
        LibtiffImage* createLibtiffImage(char* filename, BoundingBox<double> bbox);
	LibtiffImage* createLibtiffImage(char* filename, BoundingBox<double> bbox, int width, int height, int channels, uint16_t bitspersample, uint16_t photometric, uint16_t compression);
};


#endif

