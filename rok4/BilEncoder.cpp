#include <iostream>

#include "BilEncoder.h"
#include "Logger.h"

size_t BilEncoder::read(uint8_t *buffer, size_t size) {
	size_t offset = 0;

	int linesize = image->width * image->channels;

	for(; line < image->height && offset + linesize <= size; line++) {
		image->getline(buffer + offset, line);
		offset += linesize;
	}

	return offset;
}

BilEncoder::~BilEncoder() {
	//LOGGER_DEBUG( "delete BilEncoder");
	delete image;
}

bool BilEncoder::eof() {return line >= image->height;}
/*
template class BilEncoder<pixel_rgb>;
template class BilEncoder<pixel_rgba>;
template class BilEncoder<pixel_gray>;
template class BilEncoder<pixel_float>;
*/
