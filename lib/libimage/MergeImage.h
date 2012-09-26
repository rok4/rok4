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

#include "Image.h"

#ifndef MERGEIMAGE_H
#define MERGEIMAGE_H

class MergeImage : public Image {
public:
    enum MergeType {
        UNKNOWN = 0,
        NORMAL = 1,
        LIGHTEN = 2,
        DARKEN = 3,
        MULTIPLY = 4,
        MULTIPLYOLD = 5
    };


private:
    Image* backImage;
    Image* frontImage;

    MergeType composition;
    float factor;
    void mergeline ( uint8_t* buffer, uint8_t* back, uint8_t* front );
    void mergeline ( float* buffer, uint8_t* back, uint8_t* front );

public:
    virtual int getline ( float* buffer, int line );
    virtual int getline ( uint8_t* buffer, int line );
    MergeImage ( Image* backImage,Image* frontImage, MergeType composition = NORMAL, float factor = 1 );
    virtual ~MergeImage();
};

class Pixel {
public: 
    float Sr, Sg, Sb, Sa;
    float Sra, Sga, Sba;
    // 3 Chan + Alpha
    Pixel ( uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255): 
                    Sr(r), Sb(b), Sg(g)
                    {
                        Sa  = a/255.;
                        Sra = r * Sa;
                        Sga = g * Sa;
                        Sba = b * Sa;
                    }
    // 1 Chan + Alpha
    Pixel ( uint8_t x, uint8_t a = 255):
                    Sr(x/255), Sb(x/255), Sg(x/255), Sa(a/255)
                    {
                        Sra = Sr * Sa;
                        Sga = Sra;
                        Sba = Sra;
                    }
};

#endif // MERGEIMAGE_H
