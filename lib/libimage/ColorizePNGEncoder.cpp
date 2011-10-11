#include "ColorizePNGEncoder.h"
#include <iostream>
#include "byteswap.h"
#include <string.h>
#include "Logger.h"
#include <string.h> // Pour memcpy


static const uint8_t PNG_HEADER_PAL[33] = {
		137, 80, 78, 71, 13, 10, 26, 10,               // 0  | 8 octets d'entête
		0, 0, 0, 13, 'I', 'H', 'D', 'R',               // 8  | taille et type du chunck IHDR
		0, 0, 1, 0,                                    // 16 | width
		0, 0, 1, 0,                                    // 20 | height
		8,                                             // 24 | bit depth
		2,                                             // 25 | Colour type
		0,                                             // 26 | Compression method
		0,                                             // 27 | Filter method
		0,                                             // 28 | Interlace method
		0xd3, 0x10, 0x3f, 0x31};                       // 29 | crc32
// 33




ColorizePNGEncoder::ColorizePNGEncoder(Image *image, bool transparent, const uint8_t rgb[3]) : PNGEncoder(image), transparent(transparent) {
  
	PLTE[0] = 0;   PLTE[1] = 0;   PLTE[2] = 3;   PLTE[3] = 0;
	PLTE[4] = 'P'; PLTE[5] = 'L'; PLTE[6] = 'T'; PLTE[7] = 'E';
	if(transparent) {
		for(int i = 0; i < 256; i++) {
			memcpy(PLTE + 8 + 3*i, rgb, 3);
		}
	}
	else for(int i = 0; i < 256; i++) {
		PLTE[3*i+8]  = i + ((255 - i)*rgb[0] + 127) / 255;
		PLTE[3*i+9]  = i + ((255 - i)*rgb[1] + 127) / 255;
		PLTE[3*i+10] = i + ((255 - i)*rgb[2] + 127) / 255;
  }
  uint32_t crc = crc32(0, Z_NULL, 0);
  crc = crc32(crc, PLTE + 4, 3*256+4);
 *((uint32_t*) (PLTE + 256*3 + 8)) = bswap_32(crc);
  line = -3;
}

ColorizePNGEncoder::~ColorizePNGEncoder() {
  
}

size_t ColorizePNGEncoder::write_IHDRP(uint8_t *buffer, size_t size, uint8_t colortype) {
	  
	if (colortype==3){
		if(sizeof(PNG_HEADER_PAL) > size) return 0;
		memcpy(buffer, PNG_HEADER_PAL, sizeof(PNG_HEADER_PAL));// cf: http://www.w3.org/TR/PNG/#11IHDR
	}
	else{
		LOGGER_ERROR("Type de couleur non gere : " << colortype);
		return 0;
	}
	*((uint32_t*)(buffer+16)) = bswap_32(image->width);   // ajoute le champs width
	*((uint32_t*)(buffer+20)) = bswap_32(image->height);  // ajoute le champs height
	buffer[25] = 3;                               // ajoute le champs colortype
	addCRC(buffer+8, 13);                                 // signe le chunck avca un CRC32
	line++;
	if (colortype==3)
		return sizeof(PNG_HEADER_PAL);
	return 0;
}

size_t ColorizePNGEncoder::write_PLTE(uint8_t *buffer, size_t size) {
  if(sizeof(PLTE) > size) return 0;
  memcpy(buffer, PLTE, sizeof(PLTE));
  line++;
  return sizeof(PLTE);
}

size_t ColorizePNGEncoder::write_tRNS(uint8_t *buffer, size_t size) {
  if(transparent) {
    if(sizeof(tRNS) > size) return 0;
    memcpy(buffer, tRNS, sizeof(tRNS));
    line++;
    return sizeof(tRNS);
  }
  else {line++; return 0;}
}


size_t ColorizePNGEncoder::read(uint8_t* buffer, size_t size){
    size_t pos = 0;
	uint8_t colortype=0;
	// On traite 2 cas : 'Greyscale' et 'Truecolor'
	// cf: http://www.w3.org/TR/PNG/#11IHDR
	if (image->channels==1){
		colortype=3;
	}
	else if (image->channels==3){
		if(line == -3){ // nombre de ligne standard 
			pos += write_IHDR(buffer, size, 2);
			line = 0;
		}
	}
	if(line == -3) pos += write_IHDRP(buffer, size, colortype);
	if(line == -2) pos += write_PLTE(buffer + pos, size - pos);
	if(line == -1) pos += write_tRNS(buffer + pos, size - pos);
	if(line >= 0 && line <= image->height) pos += write_IDAT(buffer + pos, size - pos);
	if(line == image->height+1) pos += write_IEND(buffer + pos, size - pos);
	return pos;
}
 
bool ColorizePNGEncoder::eof() {
    return PNGEncoder::eof();
}