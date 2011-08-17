#ifndef _BILENCODER_
#define _BILENCODER_

#include "Data.h"
#include "Image.h"

class BilEncoder : public DataStream {
	Image* image;
	int line;

public:
	BilEncoder(Image* image) : image(image), line(0) {}
	~BilEncoder();
	size_t read(uint8_t *buffer, size_t size);
	int getHttpStatus() {return 200;}
	std::string getType() {return "image/x-bil;bits=32";}
	bool eof();

};
#endif


