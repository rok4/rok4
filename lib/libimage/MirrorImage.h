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

#ifndef MIRROR_IMAGE_H
#define MIRROR_IMAGE_H

/**
* @file MirrorImage.h
* @brief Image obtenue par effet miroir d'une image et de sa position par rapport à celle-ci
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
*/

/*   mirrorSize
    <---------> 
     ________________________________________
    |                                        |  /\
    |             position 0                 |  |   mirrorSize
    |________________________________________|  V
    |         |                    |         |  
    |         |                    |         |  
    | position|                    |position |  
    |   3     |   Image source     |    1    |  
    |         |                    |         |  
    |         |                    |         |  
    |_________|____________________|_________|  
    |                                        |  
    |             position 2                 |  
    |________________________________________|  


*/
class MirrorImage : public Image {

    friend class mirrorImageFactory;

    private:
    Image* image;
    int position;
    uint mirrorSize;

    protected:
    /** Constructeur */
    MirrorImage(int width, int height, int channels, BoundingBox<double> bbox, Image* image, int position,uint mirrorSize);

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
    MirrorImage* createMirrorImage(Image* pImageSrc, int position,uint mirrorSize);
};

#endif
