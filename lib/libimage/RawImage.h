#ifndef RAW_IMAGE_H
#define RAW_IMAGE_H

#include "Image.h"
#include "Data.h"
#include <cstring> // pour memcpy

class RawImage : public Image {

	DataSource *source;

  	public:
  	/** Constructeur */
  	RawImage(int width, int height, int channels, DataSource* data_source) : Image(width, height, channels) {
	// Verifier en amont que data_source n'est pas nul
	source=data_source;
}

  virtual int getline(uint8_t *buffer, int line) {
	size_t size;
        memcpy(buffer,(uint8_t*)&source->getData(size)[line*channels*width],width*channels*sizeof(uint8_t));
  };
  virtual int getline(float *buffer, int line) {
        buffer = 0;
  };

  virtual ~RawImage() {
  };
};

#endif
