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
	delete image;
}

bool BilEncoder::eof() {return line >= image->height;}
