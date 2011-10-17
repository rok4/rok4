#ifndef PALETTE_H
#define PALETTE_H
#include <stdint.h>
#include <vector>
#include <stddef.h>

class Colour {
public:
	Colour(uint8_t r=0, uint8_t g=0,uint8_t b=0, int a=0);
	
	uint8_t r;
	uint8_t g;
	uint8_t b;
	int a;
	~Colour();
	
};



class Palette
{
private:
	size_t pngPalette_size;
	uint8_t* pngPalette;
public:
	/**
	 * 
	 * @param colours : doit contenir une valeur par niveau compris entre 0 et la dernière valeure possible
	 * @param alpha : doit contenir une valeur par niveau compris entre 0 et la dernière valeure possible ou être vide (désactivation de la transparence)
	 */
	Palette();
	Palette(const Palette& pal);
	Palette(const std::vector< Colour >& mcolours);
	virtual ~Palette();
	size_t getPalettePNGSize(){return pngPalette_size;}
	uint8_t* getPalettePNG();
	
};


#endif // PALETTE_H
