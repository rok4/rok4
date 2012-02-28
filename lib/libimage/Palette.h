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
    size_t pngPaletteSize;
    uint8_t* pngPalette;
public:
    /**
     *
     * @param colours : doit contenir une valeur par niveau compris entre 0 et la dernière valeure possible
     * @param alpha : doit contenir une valeur par niveau compris entre 0 et la dernière valeure possible ou être vide (désactivation de la transparence)
     */
    Palette();
    Palette(size_t pngPaletteSize, uint8_t* pngPalette) : pngPaletteSize(pngPaletteSize), pngPalette(pngPalette) {}
    Palette(const Palette& pal);
    Palette(const std::vector< Colour >& mcolours);
    Palette & operator=(const Palette& pal);
    bool operator==(const Palette& other) const;
    bool operator!=(const Palette& other) const;
    virtual ~Palette();
    size_t getPalettePNGSize() {
        return pngPaletteSize;
    }
    uint8_t* getPalettePNG();

};


#endif // PALETTE_H