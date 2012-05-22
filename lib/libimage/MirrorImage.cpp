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

#include "MirrorImage.h"
#include "Logger.h"
#include "Utils.h"

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

/**
Creation d'une MirrorImage a partir d'une image et de la position par rapport à celle ci
retourne NULL en cas d erreur
*/

MirrorImage* mirrorImageFactory::createMirrorImage(Image* pImageSrc,int position,uint mirrorSize)
{
    int wTopBottom = pImageSrc->width+2*mirrorSize;
    int wLeftRight = mirrorSize;
    int hTopBottom = mirrorSize;
    int hLeftRight = pImageSrc->height;
    
    double xmin,ymin,xmax,ymax;
    
    if (mirrorSize > pImageSrc->width || mirrorSize > pImageSrc->height) {
        LOGGER_ERROR("Image is smaller than what we need for mirrors (we need "<< mirrorSize << " pixels)");
        return NULL;
    }
    
    if (position == 0){
        // TOP
        xmin=pImageSrc->getxmin()-pImageSrc->getresx()*mirrorSize;
        xmax=pImageSrc->getxmax()+pImageSrc->getresx()*mirrorSize;
        ymin=pImageSrc->getymax();
        ymax=pImageSrc->getymax()+mirrorSize*pImageSrc->getresy();
        BoundingBox<double> bbox(xmin,ymin,xmax,ymax);
        return new MirrorImage(wTopBottom,hTopBottom,pImageSrc->channels,bbox,pImageSrc,0,mirrorSize);
    }
    else if (position == 1){
        // RIGHT
        xmin=pImageSrc->getxmax();
        xmax=pImageSrc->getxmax()+pImageSrc->getresx()*mirrorSize;
        ymin=pImageSrc->getymin();
        ymax=pImageSrc->getymax();
        BoundingBox<double> bbox(xmin,ymin,xmax,ymax);
        return new MirrorImage(wLeftRight,hLeftRight,pImageSrc->channels,bbox,pImageSrc,1,mirrorSize);
    }
    else if (position == 2){
        // BOTTOM
        xmin=pImageSrc->getxmin()-pImageSrc->getresx()*mirrorSize;
        xmax=pImageSrc->getxmax()+pImageSrc->getresx()*mirrorSize;
        ymin=pImageSrc->getymin()-mirrorSize*pImageSrc->getresy();
        ymax=pImageSrc->getymin();
        BoundingBox<double> bbox(xmin,ymin,xmax,ymax);
        return new MirrorImage(wTopBottom,hTopBottom,pImageSrc->channels,bbox,pImageSrc,2,mirrorSize);
    }
    else if (position == 3){
        // LEFT
        xmin=pImageSrc->getxmin()-pImageSrc->getresx()*mirrorSize;
        xmax=pImageSrc->getxmin();
        ymin=pImageSrc->getymin();
        ymax=pImageSrc->getymax();
        BoundingBox<double> bbox(xmin,ymin,xmax,ymax);
        return new MirrorImage(wLeftRight,hLeftRight,pImageSrc->channels,bbox,pImageSrc,3,mirrorSize);
    }

}

MirrorImage::MirrorImage(int width, int height, int channels, BoundingBox<double> bbox, Image* image, int position,uint mirrorSize) : Image(width,height,channels,bbox), image(image), position(position), mirrorSize(mirrorSize)
{
}

int MirrorImage::getline(uint8_t* buffer, int line)
{
    uint32_t line_size=width*channels;
    uint8_t* buf0=new uint8_t[image->width*channels];


    if (position == 0) {
        // TOP
        int lineSrc = height - line -1;
        
        image->getline(buf0,lineSrc);
        
        memcpy(&buffer[mirrorSize*channels],buf0,image->width*channels);
        for (int j = 0; j < mirrorSize; j++) {
            memcpy(&buffer[j*channels],&buf0[(mirrorSize-j-1)*channels],channels); // left
            memcpy(&buffer[(width-j-1)*channels],&buf0[(image->width - mirrorSize + j)*channels],channels); // right
        }
        
    }
    else if (position == 1) {
        // RIGHT
        image->getline(buf0,line);
        for (int j = 0; j < mirrorSize; j++) {
            memcpy(&buffer[j*channels],&buf0[(image->width-j-1)*channels],channels); // right
        }

    }
    else if (position == 2) {
        // BOTTOM
        int lineSrc = image->height - line -1;
        
        image->getline(buf0,lineSrc);
        
        memcpy(&buffer[mirrorSize*channels],buf0,image->width*channels);
        for (int j = 0; j < mirrorSize; j++) {
            memcpy(&buffer[j*channels],&buf0[(mirrorSize-j-1)*channels],channels); // left
            memcpy(&buffer[(width-j-1)*channels],&buf0[(image->width - mirrorSize + j)*channels],channels); // right
        }

    }
    else if (position == 3) {
        // LEFT
        image->getline(buf0,line);
        for (int j = 0; j < mirrorSize; j++) {
            memcpy(&buffer[j*channels],&buf0[(mirrorSize-j-1)*channels],channels); // right
        }

    }

    return width*channels;
}

int MirrorImage::getline(float* buffer, int line)
{
    return width*channels;
}

MirrorImage::~MirrorImage()
{
}
