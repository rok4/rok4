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

/**
 * \file ExtendedCompoundImage.cpp
 ** \~french
 * \brief Implémentation des classes ExtendedCompoundImage, ExtendedCompoundMask et ExtendedCompoundImageFactory
 * \details
 * \li ExtendedCompoundImage : image composée d'images compatibles, superposables
 * \li ExtendedCompoundMask : masque composé, associé à une image composée
 * \li ExtendedCompoundImageFactory : usine de création d'objet ExtendedCompoundImage
 ** \~english
 * \brief Implement classes ExtendedCompoundImage, ExtendedCompoundMask and ExtendedCompoundImageFactory
 * \details
 * \li ExtendedCompoundImage : image compounded with superimpose images
 * \li ExtendedCompoundMask : compounded mask, associated with a compounded image
 * \li ExtendedCompoundImageFactory : factory to create ExtendedCompoundImage object
 */

#include "ExtendedCompoundImage.h"
#include "Logger.h"
#include "Utils.h"
#include "EmptyImage.h"

/********************************************** ExtendedCompoundImage ************************************************/

template <typename T>
int ExtendedCompoundImage::_getline ( T* buffer, int line ) {
    int i;

    // Initialisation de tous les pixels de la ligne avec la valeur de nodata
    for ( i=0; i<width*channels; i++ ) {
        buffer[i]= ( T ) nodata[i%channels];
    }

    for ( i = 0; i < ( int ) sourceImages.size(); i++ ) {
        
        int lineInSource = line - rowsOffsets[i];
        
        // On ecarte les images qui ne se trouvent pas sur la ligne
        // On evite de comparer des coordonnees terrain (comparaison de flottants)
        // Les coordonnees image sont obtenues en arrondissant au pixel le plus proche

        if ( lineInSource < 0 || lineInSource >= sourceImages[i]->getHeight() ) {
            continue;
        }
        if ( sourceImages[i]->getXmin() >= getXmax() || sourceImages[i]->getXmax() <= getXmin() ) {
            continue;
        }

        // c0 : indice de la 1ere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
        int c0 = c0s[i];
        // c1 : indice de la derniere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
        int c1 = c1s[i];

        // c2 : indice de de la 1ere colonne de l'ExtendedCompoundImage dans l'image courante
        int c2 = c2s[i];

        T* buffer_t = new T[sourceImages[i]->getWidth() * sourceImages[i]->getChannels()];

        sourceImages[i]->getline ( buffer_t,lineInSource );

        if ( getMask ( i ) == NULL ) {
            memcpy ( &buffer[c0*channels], &buffer_t[c2*channels], ( c1 + 1 - c0) *channels*sizeof ( T ) );
        } else {

            uint8_t* buffer_m = new uint8_t[getMask ( i )->getWidth()];
            getMask ( i )->getline ( buffer_m,lineInSource );

            for ( int j=0; j < c1 - c0 + 1; j++ ) {
                if ( buffer_m[c2+j] ) {
                    memcpy ( &buffer[ ( c0 + j ) *channels],&buffer_t[ ( c2+j ) *channels],sizeof ( T ) *channels );
                }
            }

            delete [] buffer_m;
        }
        delete [] buffer_t;
    }
    return width*channels*sizeof ( T );
}


/* Implementation de getline pour les uint8_t */
int ExtendedCompoundImage::getline ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de getline pour les float */
int ExtendedCompoundImage::getline ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de getline pour les float */
int ExtendedCompoundImage::getline ( float* buffer, int line ) {
    return _getline ( buffer, line );
}

bool ExtendedCompoundImage::addMirrors ( int mirrorSize ) {
    MirrorImageFactory MIF;
    std::vector< Image*>  mirrorImages;

    if ( mirrorSize <= 0 ) {
        LOGGER_ERROR ( "Unable to add mirror : mirror's size negative or null : " << mirrorSize );
        return false;
    }

    if ( sourceImages.size() == 0 ) {
        LOGGER_ERROR ( "Unable to add mirror : no source image" );
        return false;
    }

    for ( uint i = 0; i < sourceImages.size(); i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            MirrorImage* mirrorImage = MIF.createMirrorImage ( sourceImages.at ( i ), j, mirrorSize );
            if ( mirrorImage == NULL ) {
                LOGGER_ERROR ( "Unable to calculate image's mirror" );
                return false;
            }

            if ( sourceImages.at ( i )->getMask() ) {
                MirrorImage* mirrorMask = MIF.createMirrorImage ( sourceImages.at ( i )->getMask(), j, mirrorSize );
                if ( mirrorMask == NULL ) {
                    LOGGER_ERROR ( "Unable to calculate mask's mirror" );
                    return false;
                }
                if ( ! mirrorImage->setMask ( mirrorMask ) ) {
                    LOGGER_ERROR ( "Unable to add mask to mirror" );
                    return false;
                }
            }

            mirrorImages.push_back ( mirrorImage );
        }
    }

    sourceImages.insert ( sourceImages.begin(),mirrorImages.begin(),mirrorImages.end() );
    mirrorsNumber = mirrorImages.size();

    // Mise à jour des dimensions en tenant compte des miroirs : BBOX et tailles pixel
    for ( unsigned int j = 0; j < mirrorsNumber; j++ ) {
        if ( sourceImages.at ( j )->getXmin() < bbox.xmin )  bbox.xmin = sourceImages.at ( j )->getXmin();
        if ( sourceImages.at ( j )->getYmin() < bbox.ymin )  bbox.ymin = sourceImages.at ( j )->getYmin();
        if ( sourceImages.at ( j )->getXmax() > bbox.xmax )  bbox.xmax = sourceImages.at ( j )->getXmax();
        if ( sourceImages.at ( j )->getYmax() > bbox.ymax )  bbox.ymax = sourceImages.at ( j )->getYmax();
    }

    width = int ( ( bbox.xmax-bbox.xmin ) / getResX() + 0.5 );
    height = int ( ( bbox.ymax-bbox.ymin ) / getResY() + 0.5 );

    if ( mask ) {
        // Mise à jour du masque associé à l'image composée
        ExtendedCompoundMask* newMask = new ExtendedCompoundMask ( this );

        if ( ! setMask ( newMask ) ) {
            LOGGER_ERROR ( "Unable to add mask to ExtendedCompoundImage with mirrors" );
            return false;
        }
    }
    
    calculateOffsets();

    return true;
}

bool ExtendedCompoundImage::extendBbox ( BoundingBox< double > otherbbox, int morePix = 0 ) {

    BoundingBox<double> newBbox ( bbox );
    double nbPix;

    /******************** Mise à jour des dimensions **********************/

    //XMIN
    if ( otherbbox.xmin < bbox.xmin ) {
        nbPix = ( double ) ceil ( ( bbox.xmin - otherbbox.xmin ) / resx );
        LOGGER_DEBUG ( "Ajout de " << nbPix << " à gauche" );
        width += (int) nbPix;
        newBbox.xmin -= nbPix * resx;
    }

    // XMAX
    if ( otherbbox.xmax > bbox.xmax ) {
        nbPix = ( double ) ceil ( ( otherbbox.xmax - bbox.xmax ) / resx );
        LOGGER_DEBUG ( "Ajout de " << nbPix << " à droite" );
        width += (int) nbPix;
        newBbox.xmax += nbPix * resx;
    }

    //YMIN
    if ( otherbbox.ymin < bbox.ymin ) {
        nbPix = ( double ) ceil ( ( bbox.ymin - otherbbox.ymin ) / resy );
        LOGGER_DEBUG ( "Ajout de " << nbPix << " en bas" );
        height += (int) nbPix;
        newBbox.ymin -= nbPix * resy;
    }

    // YMAX
    if ( otherbbox.ymax > bbox.ymax ) {
        nbPix = ( double ) ceil ( ( otherbbox.ymax - bbox.ymax ) / resy );
        LOGGER_DEBUG ( "Ajout de " << nbPix << " en haut" );
        height += (int) nbPix;
        newBbox.ymax += nbPix * resy;
    }

    /******************** ajout de pixels supplémentaires *****************/

    if ( morePix > 0 ) {
        LOGGER_DEBUG("Ajout de " << morePix << " pixels de tous les côtés");
        width += 2*morePix;
        height += 2*morePix;

        newBbox.xmin -= morePix * resx;
        newBbox.ymin -= morePix * resy;
        newBbox.xmax += morePix * resx;
        newBbox.ymax += morePix * resy;
    }
    
    if (! Image::dimensionsAreConsistent(resx, resy, width, height, newBbox)) {
        LOGGER_ERROR ( "Resolutions, new bounding box and new pixels dimensions of the enlarged ExtendedCompoundImage are not consistent" );
        return false;
    }

    /*********************** mise à jour des attributs ********************/

    bbox = newBbox;

    // Mise à jour du masque associé à l'image composée
    if ( mask ) {
        ExtendedCompoundMask* newMask = new ExtendedCompoundMask ( this );

        if ( ! setMask ( newMask ) ) {
            LOGGER_ERROR ( "Unable to add mask to enlarged ExtendedCompoundImage" );
            return false;
        }
    }
    
    calculateOffsets();

    return true;
}

/****************************************** ExtendedCompoundImageFactory *********************************************/

ExtendedCompoundImage* ExtendedCompoundImageFactory::createExtendedCompoundImage (
    std::vector<Image*>& images, int* nodata, uint mirrors ) {

    // On doit forcément avoir une image en entrée, car les dimensions de l'ExtendedCompoundImage sont calculée à partir des sources
    if ( images.size() == 0 ) {
        LOGGER_ERROR ( "No source images to define compounded image" );
        return NULL;
    }

    for ( int i=0; i<images.size()-1; i++ ) {
        if ( ! images[i]->isCompatibleWith ( images[i+1] ) ) {
            LOGGER_ERROR ( "Source images are not consistent" );
            LOGGER_ERROR ( "Image " << i );
            images[i]->print();
            LOGGER_ERROR ( "Image " << i+1 );
            images[i+1]->print();
            return NULL;
        }
    }

    // Rectangle englobant des images d entree
    double xmin=1E12, ymin=1E12, xmax=-1E12, ymax=-1E12 ;
    for ( unsigned int j=0; j<images.size(); j++ ) {
        if ( images.at ( j )->getXmin() <xmin )  xmin=images.at ( j )->getXmin();
        if ( images.at ( j )->getYmin() <ymin )  ymin=images.at ( j )->getYmin();
        if ( images.at ( j )->getXmax() >xmax )  xmax=images.at ( j )->getXmax();
        if ( images.at ( j )->getYmax() >ymax )  ymax=images.at ( j )->getYmax();
    }

    int w = ( int ) ( ( xmax-xmin ) / ( *images.begin() )->getResX() +0.5 );
    int h = ( int ) ( ( ymax-ymin ) / ( *images.begin() )->getResY() +0.5 );
    
    ExtendedCompoundImage* pECI = new ExtendedCompoundImage (
        w, h, images.at ( 0 )->getChannels(), ( *images.begin() )->getResX(), ( *images.begin() )->getResY(), BoundingBox<double> ( xmin,ymin,xmax,ymax ),
        images, nodata, mirrors
    );
    pECI->setCRS ( images.at ( 0 )->getCRS() );

    return pECI;
}

ExtendedCompoundImage* ExtendedCompoundImageFactory::createExtendedCompoundImage (
    int width, int height, int channels, BoundingBox<double> bbox,
    std::vector<Image*>& images, int* nodata, uint mirrors ) {
    
    double resx = -1, resy = -1;

    if ( images.size() == 0 ) {
        // On peut ne pas avoir d'image en entrée, l'ExtendedCompoundImage sera alors pleine de nodata, et le masque sera vide (que des 0)
        LOGGER_INFO ( "No source images to compose the ExtendedCompoundImage (=> only nodata)" );
        resx = (bbox.xmax - bbox.xmin) / (double) width;
        resy = (bbox.ymax - bbox.ymin) / (double) height;
    } else {

        for ( int i=0; i<images.size()-1; i++ ) {
            if ( ! images[i]->isCompatibleWith ( images[i+1] ) ) {
                LOGGER_ERROR ( "Source images are not consistent" );
                LOGGER_ERROR ( "Image " << i );
                images[i]->print();
                LOGGER_ERROR ( "Image " << i+1 );
                images[i+1]->print();
                return NULL;
            }
        }
        
        resx = images.at(0)->getResX();
        resy = images.at(0)->getResY();
        
        if (! Image::dimensionsAreConsistent(resx, resy, width, height, bbox)) {
            LOGGER_ERROR ( "Resolutions, bounding box and dimensions for ExtendedCompoundImage are not consistent" );
            return NULL;
        }
    }

    return new ExtendedCompoundImage ( width,height,channels, resx, resy, bbox,images,nodata,mirrors );
}

/********************************************** ExtendedCompoundMask *************************************************/

int ExtendedCompoundMask::_getline ( uint8_t* buffer, int line ) {

    memset ( buffer,0,width );

    for ( uint i = ECI->getMirrorsNumber(); i < ECI->getImages()->size(); i++ ) {
        
        int ol, c0, c1, c2;
        
        ECI->getOffsets(i, &ol, &c0, &c1, &c2);
        
        int lineInSource = line - ol;
        
        /* On ecarte les images qui ne se trouvent pas sur la ligne
         * On evite de comparer des coordonnees terrain (comparaison de flottants)
         * Les coordonnees image sont obtenues en arrondissant au pixel le plus proche
         */
        if ( lineInSource < 0 || lineInSource >=  ECI->getImages()->at ( i )->getHeight() ) {
            continue;
        }
        if ( ECI->getImages()->at ( i )->getXmin() >= getXmax() || ECI->getImages()->at ( i )->getXmax() <= getXmin() ) {
            continue;
        }
 
        if ( ECI->getMask ( i ) == NULL ) {
            memset ( &buffer[c0], 255, c1 - c0 + 1 );
        } else {
            // Récupération du masque de l'image courante de l'ECI.
            uint8_t* buffer_m = new uint8_t[ECI->getMask ( i )->getWidth()];
            ECI->getMask ( i )->getline ( buffer_m,lineInSource );
            // On ajoute au masque actuel (on écrase si la valeur est différente de 0)
            for ( int j = 0; j < c1 - c0 + 1; j++ ) {
                if ( buffer_m[c2+j] ) {
                    memcpy ( &buffer[c0+j],&buffer_m[c2+j],1 );
                }
            }
            delete [] buffer_m;
        }
    }

    return width;
}

/* Implementation de getline pour les uint8_t */
int ExtendedCompoundMask::getline ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de getline pour les float */
int ExtendedCompoundMask::getline ( uint16_t* buffer, int line ) {
    uint8_t* buffer_t = new uint8_t[width*channels];
    getline ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return width*channels;
}

/* Implementation de getline pour les float */
int ExtendedCompoundMask::getline ( float* buffer, int line ) {
    uint8_t* buffer_t = new uint8_t[width*channels];
    getline ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return width*channels;
}
