#include <iostream>

#include "BilEncoder.h"
#include "Logger.h"

size_t BilEncoder::read(uint8_t *buffer, size_t size) {
	size_t offset = 0;

	// Hypothese 1 : tous les bil produits sont en float
	int linesize = image->width * image->channels * sizeof(float);
	// Hypothese 2 : le pixel de l'image est de type float
	float* buf_f=new float[image->width*image->channels]; 

	for(; line < image->height && offset + linesize <= size; line++) {
		
		//image->getline(buffer + offset, line);
		image->getline(buf_f,line);

		// Hypothese 3 : image->channels=1
		// On n'utilise pas la fonction convert qui caste un float en uint8_t
		// On copie simplement les octets des floats dans des uint8_t
		for (int i=0;i<image->width;i++){
			for (int j=0;j<sizeof(float);j++)	
				buffer[offset+sizeof(float)*i+j]=* ((uint8_t*) (&buf_f[i])+j);
		}

		offset += linesize;
	}

	delete buf_f;

	return offset;
}

BilEncoder::~BilEncoder() {
	delete image;
}

bool BilEncoder::eof() {return line >= image->height;}
