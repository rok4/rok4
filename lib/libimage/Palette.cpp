/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière 
 * 
 * Géoportail SAV <geop_services@geoportail.fr>
 * 
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 * 
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * 
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

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
	pngPaletteSize = 0;
	pngPalette = NULL;
}

Palette::Palette(size_t pngPaletteSize, uint8_t* pngPaletteData)  : pngPaletteSize(pngPaletteSize)
{
    pngPalette = new uint8_t[pngPaletteSize];
    memcpy(pngPalette,pngPaletteData,pngPaletteSize);
}



Palette::Palette(const Palette& pal) : pngPaletteSize(0)
{

	pngPaletteSize = pal.pngPaletteSize;

	if (pngPaletteSize !=0) {
		pngPalette = new uint8_t[pngPaletteSize];
		memcpy(pngPalette,pal.pngPalette,pngPaletteSize);
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
		pngPaletteSize = numberColor* 3 + 12 +numberColor +12; // Palette(Nombre de couleur* nombre de canaux + header) + Transparence(Nombre de couleur + header)
		pngPalette = new uint8_t[pngPaletteSize];
		memset(pngPalette,0,pngPaletteSize);
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
		pngPaletteSize=0;
		pngPalette=NULL;
		LOGGER_INFO("Palette incompatible avec le PNG");
	}
}

Palette& Palette::operator=(const Palette& pal)
{
    if (this != &pal) {
        this->pngPaletteSize = pal.pngPaletteSize;
        if (this->pngPaletteSize !=0) {
            this->pngPalette = new uint8_t[pngPaletteSize];
            memcpy(pngPalette,pal.pngPalette,this->pngPaletteSize);
        }else {
            this->pngPalette = NULL;
        }
    }
    return *this;
}


uint8_t* Palette::getPalettePNG()
{
	return pngPalette;
}


bool Palette::operator==(const Palette& other) const
{
    bool equal = true;
    if (this->pngPaletteSize != other.pngPaletteSize)
        return false;
    for ( size_t pos = this->pngPaletteSize -1; pos && equal; --pos ) {
         equal = ( * ( this->pngPalette+pos ) == * ( other.pngPalette+pos ) );
    }
    return equal;
}

bool Palette::operator!=(const Palette& other) const
{
    return !(*this == other);
}


Palette::~Palette()
{
    if (pngPalette)
        delete[] pngPalette;
}

