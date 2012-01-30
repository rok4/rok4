#ifndef MIRROR_IMAGE_H
#define MIRROR_IMAGE_H

/**
* @file MirrorImage.h
* @brief Image obetnue par effet mirroir a partir de 4 images voisines
* @author IGN France - Geoportail
*/

#include "Image.h"
#include <vector>
#include <cstring>
#include "Logger.h"
#include "Utils.h"

/**
* @class MirrorImage
* @brief Image obtenue par effet miroir a partir de 4 autres images voisines
* Utiles pour eviter les effets de bord lors du reechantillonnage d'une ExtendCompoundImage contenant du nodata :
* Au niveau de la limite data/nodata et sur une bande de largeur la taille du noyau d'interpolation, les pixels ne sont pas significatifs
* (car obtenus a partir entre autres de pixels nodata)
* Cropper cette bande n'est pas une bonne solution, car si on veut reconstituer la dalle d'origine (cas classique des dalles de BDOrtho),
* cette bande sera soit blanche (nodata) soit remplie a partir de pixels issus d'autres images plus anciennes
* solution retenue : generer localement de la fausse information par effet miroir qui aura pour effet de combler cette bande avec une effet visuel correct
* Contrainte : images correctement disposees, de meme caracteristiques geometriques (Cf. les CompoundImage)
*/

/*
                     _______________
                    |               |
                    |               |
                    |   Image1      |        
                    |               |        
     _______________|_______________|_______________
    |               |               |               |
    |               |               |               |
    | Image0        |   Mirror      |   Image2      |
    |               |               |               |
    |_______________|_______________|_______________|
                    |               |
                    |               |
                    |   Image3      |               
                    |               |               
                    |_______________|


*/
class MirrorImage : public Image {

    friend class mirrorImageFactory;

    private:
    // Contient obligatoirement 4 images (NULL si une image contient du nodata)
    std::vector<Image*> images;

    protected:
    /** Constructeur */
    MirrorImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images);

    public:

    /** D */
    int getline(uint8_t* buffer, int line);

    /** D */
    int getline(float* buffer, int line);

    /** Destructeur */
    ~MirrorImage();
};

class mirrorImageFactory {
    public:
    MirrorImage* createMirrorImage(Image* pImage0, Image* pImage1, Image* pImage2, Image* pImage3);
};

#endif
