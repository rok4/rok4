#ifndef RAW_IMAGE_H
#define RAW_IMAGE_H

#include "Image.h"
#include "Data.h"
#include <cstring> // pour memcpy
#include "Logger.h"

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
	const uint8_t* data=source->getData(size);
	if (!data){
		buffer=0;
		return 0;
	}
        memcpy(buffer,(uint8_t*)&data[line*channels*width],width*channels*sizeof(uint8_t));
	return width*channels*sizeof(uint8_t);
  };
  virtual int getline(float *buffer, int line) {
        buffer = 0;
  };

  virtual ~RawImage() {
  };
};

#endif
