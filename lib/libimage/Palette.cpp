#include "Palette.h"
#include <stdint.h>
#include "zlib.h"
#include <string.h>
#include "byteswap.h"
#include "Logger.h"

Colour::Colour(uint8_t r, uint8_t g, uint8_t b, int a): r(r), g(g), b(b), a(a)
{

}

Colour::~Colour()
{

}


Palette::Palette()
{
	pngPalette_size = 0;
	pngPalette = NULL;
}


Palette::Palette(const Palette& pal) : pngPalette_size(0)
{

	pngPalette_size = pal.pngPalette_size;

	if (pngPalette_size !=0) {
		pngPalette = new uint8_t[pngPalette_size];
		memcpy(pngPalette,pal.pngPalette,pngPalette_size);
	}else {
		pngPalette = NULL;
	}

}


/**
 * 
 */
Palette::Palette(const std::vector<Colour>& mcolours)
{
	std::vector<Colour> colours= mcolours;
	LOGGER_DEBUG("Constructeur de Palette");
	int numberColor = colours.size();
	if (numberColor != 0) {
		LOGGER_DEBUG("Palette PNG OK");
		pngPalette_size = numberColor* 3 + 12 +numberColor +12; // Palette(Nombre de couleur* nombre de canaux + header) + Transparence(Nombre de couleur + header)
		pngPalette = new uint8_t[pngPalette_size];
		memset(pngPalette,0,pngPalette_size);
		// Définition de la taille de la palette
		uint32_t paletteLenght = 3 * numberColor; 
		*((uint32_t*) (pngPalette)) = bswap_32(paletteLenght);
		pngPalette[0] = 0;   pngPalette[1] = 0;   pngPalette[2] = 3;   pngPalette[3] = 0;
		pngPalette[4] = 'P'; pngPalette[5] = 'L'; pngPalette[6] = 'T'; pngPalette[7] = 'E';
	
		pngPalette[paletteLenght+12] = 0;   pngPalette[paletteLenght+12+1] = 0;   pngPalette[paletteLenght+12+2] = 3;   pngPalette[paletteLenght+12+3] = 0;
		pngPalette[paletteLenght+12+4] = 't'; pngPalette[paletteLenght+12+5] = 'R'; pngPalette[paletteLenght+12+6] = 'N'; pngPalette[paletteLenght+12+7] = 'S';
		
		Colour tmp;
		for(int i =0; i < numberColor; i++) {
			tmp = colours.at(i);
			/*pngPalette[3*i+8]  = 256 - colours.at(i).r;
			pngPalette[3*i+9]  = 256 - colours.at(i).g;
			pngPalette[3*i+10] = 256 - colours.at(i).b;
			pngPalette[paletteLenght+12+i+8] = 256 - colours.at(i).a;*/
			
			pngPalette[3*i+8]  = tmp.r;
			pngPalette[3*i+9]  = tmp.g;
			pngPalette[3*i+10] = tmp.b;
			pngPalette[paletteLenght+12+i+8] = 255 - colours.at(i).a;
			
			
			/*pngPalette[3*i+8]  = i + ((255 - i)*currentColour.r + 127) / 255;
			pngPalette[3*i+9]  = i + ((255 - i)*currentColour.g + 127) / 255;
			pngPalette[3*i+10] = i + ((255 - i)*currentColour.b + 127) / 255;
			pngPalette[paletteLenght+12+i+8] = i + ((255 - i)*currentColour.a + 127) / 255;*/
		}
		uint32_t crcPLTE = crc32(0, Z_NULL, 0);
		crcPLTE = crc32(crcPLTE, pngPalette + 4, paletteLenght+4);
		*((uint32_t*) (pngPalette + paletteLenght + 8)) = bswap_32(crcPLTE);
		
		uint32_t crctRNS = crc32(0, Z_NULL, 0);
		crctRNS = crc32(crctRNS, pngPalette+ paletteLenght+12 + 4, 4 + numberColor);
		*((uint32_t*) (pngPalette + paletteLenght+ 12 + 8 + numberColor)) = bswap_32(crctRNS);
		uint32_t trnsLenght = numberColor; 
		*((uint32_t*) (pngPalette+paletteLenght+12)) = bswap_32(trnsLenght);
	}else {
		pngPalette_size=0;
		pngPalette=NULL;
		LOGGER_INFO("Palette incompatible avec le PNG");
	}
}

uint8_t* Palette::getPalettePNG()
{
	return pngPalette;
}


Palette::~Palette()
{

}

