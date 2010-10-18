#include <iostream>
#include <string.h> // Pour memcpy

#include "TiffEncoder.h"
#include "Logger.h"

size_t TiffEncoder::read(uint8_t *buffer, size_t size_to_read) {
	size_t offset = 0, header_size=128, linesize=image->width*image->channels;
	if(line == -1) { // écrire le header tiff
		// Si pas assez de place pour le header, ne rien écrire.
		if(size_to_read < header_size) return 0;

		// Ceci est du tiff avec une seule strip.
		if (image->channels==1)
			memcpy(buffer, TIFF_HEADER_GRAY, header_size);
		else if (image->channels==3)
			memcpy(buffer, TIFF_HEADER_RGB, header_size);
		else if (image->channels==4)
			memcpy(buffer, TIFF_HEADER_RGBA, header_size);
		*((uint32_t*)(buffer+18))  = image->width;
		*((uint32_t*)(buffer+30))  = image->height;
		*((uint32_t*)(buffer+102)) = image->height;
		*((uint32_t*)(buffer+114)) = image->height*linesize;
		offset = header_size;
		line = 0;
	}

	for(; line < image->height && offset + linesize <= size_to_read; line++) {
		// Pour l'instant on ne gere que le type uint8_t
		image->getline((uint8_t*)(buffer + offset), line);
		offset += linesize*sizeof(uint8_t);
	}

	return offset;
}

bool TiffEncoder::eof()
{
	return (line>=image->height);	
}

TiffEncoder::~TiffEncoder() {
	LOGGER_DEBUG("delete TiffEncoder");
	delete image;
}
