/*
 * Copyright © (2011-2013) Institut national de l'information
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

#include <cmath>
#include "Pyramid.h"
#include "Logger.h"
#include "Message.h"
#include "Grid.h"
#include "Decoder.h"
#include "JPEGEncoder.h"
#include "PNGEncoder.h"
#include "TiffEncoder.h"
#include "BilEncoder.h"
#include "AscEncoder.h"
#include "ExtendedCompoundImage.h"
#include "Format.h"
#include "TiffEncoder.h"
#include "Level.h"
#include <cfloat>
#include "intl.h"
#include "config.h"
#include "EmptyImage.h"

ComparatorLevel compLevelDesc =
    [](std::pair<std::string, Level*> elem1 ,std::pair<std::string, Level*> elem2)
    {
        return elem1.second->getRes() > elem2.second->getRes();
    };

ComparatorLevel compLevelAsc =
    [](std::pair<std::string, Level*> elem1 ,std::pair<std::string, Level*> elem2)
    {
        return elem1.second->getRes() < elem2.second->getRes();
    };

Pyramid::Pyramid (PyramidXML* p) : Source(PYRAMID) {
    levels = p->levels;
    tms = p->tms;
    format = p->format;

    if (Rok4Format::isRaster(format)) {
        photo = p->photo;
        channels = p->channels;
        transparent = false;
        ndValues = new int[p->noDataValues.size()];
        for (int i = 0; i < p->noDataValues.size(); i++) {
            ndValues[i] = p->noDataValues.at(i);
        }
        isBasedPyramid = p->isBasedPyramid;
        containOdLevels = p->containOdLevels;

    }

    std::map<std::string, Level*>::iterator itLevel;
    double minRes= DBL_MAX;
    double maxRes= DBL_MIN;
    for ( itLevel=levels.begin(); itLevel!=levels.end(); itLevel++ ) {

        //Determine Higher and Lower Levels
        double d = itLevel->second->getRes();
        if ( minRes > d ) {
            minRes = d;
            lowestLevel = itLevel->second;
        }
        if ( maxRes < d ) {
            maxRes = d;
            highestLevel = itLevel->second;
        }
    }

}

Pyramid::Pyramid (Pyramid* obj, ServerXML* sxml): Source(PYRAMID) {
    format = obj->format;

    // On récupère bien le pointeur vers le nouveau TMS (celui de la nouvelle liste)
    tms = sxml->getTMS (obj->tms->getId());
    if ( tms == NULL ) {
        LOGGER_ERROR ( "Une pyramide clonée reference un TMS [" << obj->tms->getId() << "] qui n'existe plus." );
        return;
        // Tester la nullité du TMS en sortie pour faire remonter l'erreur
    }


    std::map<std::string, Level*>::iterator itLevel;

    // On clone bien tous les niveaux
    for ( itLevel = obj->levels.begin(); itLevel != obj->levels.end(); itLevel++ ) {
        Level* levObj = new Level(itLevel->second, sxml, tms);
        if (levObj->getContext() == NULL) {
            LOGGER_ERROR ( "Impossible de cloner le niveau " << itLevel->first );
            tms = NULL;
            // Tester la nullité du TMS en sortie pour faire remonter l'erreur
            return;
        }

        levels.insert ( std::pair<std::string, Level*> ( levObj->getId(), levObj ) );
    }

    double minRes= DBL_MAX;
    double maxRes= DBL_MIN;
    for ( itLevel = levels.begin(); itLevel != levels.end(); itLevel++ ) {

        //Determine Higher and Lower Levels
        double d = itLevel->second->getRes();
        if ( minRes > d ) {
            minRes = d;
            lowestLevel = itLevel->second;
        }
        if ( maxRes < d ) {
            maxRes = d;
            highestLevel = itLevel->second;
        }
    }

    if (Rok4Format::isRaster(format)) {
        photo = obj->photo;
        channels = obj->channels;
        transparent = obj->transparent;

        isBasedPyramid = obj->isBasedPyramid;
        containOdLevels = obj->containOdLevels;

        ndValues = new int[channels];
        for (int i = 0; i < channels; i++) {
            ndValues[i] = obj->ndValues[i];
        }
    }

}

std::string Pyramid::best_level ( double resolution_x, double resolution_y, bool onDemand ) {

    // TODO: A REFAIRE !!!!
    // res_level/resx ou resy ne doit pas exceder une certaine valeur
    double resolution = sqrt ( resolution_x * resolution_y );

    std::map<std::string, Level*>::iterator it ( levels.begin() ), itend ( levels.end() );
    std::string best_h = it->first;
    double best = resolution_x / it->second->getRes();
    ++it;
    for ( ; it!=itend; ++it ) {
        double d = resolution / it->second->getRes();
        if ( ( best < 0.8 && d > best ) ||
                ( best >= 0.8 && d >= 0.8 && d < best ) ) {
            best = d;
            best_h = it->first;
        }
    }

    if (onDemand) {

        if (best <= 1.8 && best >= 0.8) {
            return best_h;
        } else {
            return "";
        }

    } else {
        return best_h;
    }

}


Image* Pyramid::getbbox ( ServicesXML* servicesXML, BoundingBox<double> bbox, int width, int height, CRS dst_crs, Interpolation::KernelType interpolation, int dpi, int& error ) {

    // On calcule la résolution de la requete dans le crs source selon une diagonale de l'image
    double resolution_x, resolution_y;

    LOGGER_DEBUG ( "source tms->getCRS() is " << tms->getCrs().getProj4Code() << " and destination dst_crs is " << dst_crs.getProj4Code() );

    if ( tms->getCrs() == dst_crs || servicesXML->are_the_two_CRS_equal( tms->getCrs().getProj4Code(), dst_crs.getProj4Code() ) ) {
        resolution_x = ( bbox.xmax - bbox.xmin ) / width;
        resolution_y = ( bbox.ymax - bbox.ymin ) / height;
    } else {
        Grid* grid = new Grid ( width, height, bbox );


        LOGGER_DEBUG ( _ ( "debut pyramide" ) );
        if ( !grid->reproject ( dst_crs.getProj4Code(),tms->getCrs().getProj4Code() ) ) {
            // BBOX invalide
            delete grid;
            error=1;
            return 0;
        }
        LOGGER_DEBUG ( _ ( "fin pyramide" ) );

        resolution_x = ( grid->bbox.xmax - grid->bbox.xmin ) / width;
        resolution_y = ( grid->bbox.ymax - grid->bbox.ymin ) / height;
        delete grid;
    }

    if (dpi != 0) {
        //si un parametre dpi a ete donne dans la requete, alors on l'utilise
        resolution_x = resolution_x * dpi / 90.7;
        resolution_y = resolution_y * dpi / 90.7;
        //on teste si on vient d'avoir des NaN
        if (resolution_x != resolution_x || resolution_y != resolution_y) {
            error = 3;
            return 0;
        }
    }

    std::string l = best_level ( resolution_x, resolution_y, false );
    LOGGER_DEBUG ( _ ( "best_level=" ) << l << _ ( " resolution requete=" ) << resolution_x << " " << resolution_y );

    if ( tms->getCrs() == dst_crs || servicesXML->are_the_two_CRS_equal( tms->getCrs().getProj4Code(), dst_crs.getProj4Code() ) ) {
        return levels[l]->getbbox ( servicesXML, bbox, width, height, interpolation, error );
    } else {
        return createReprojectedImage(l, bbox, dst_crs, servicesXML, width, height, interpolation, error);
    }

}

Image * Pyramid::createReprojectedImage(std::string l, BoundingBox<double> bbox, CRS dst_crs, ServicesXML* servicesXML, int width, int height, Interpolation::KernelType interpolation, int error) {

    if ( dst_crs.validateBBox ( bbox ) ) {
        return levels[l]->getbbox ( servicesXML, bbox, width, height, tms->getCrs(), dst_crs, interpolation, error );
    } else {
        BoundingBox<double> cropBBox = dst_crs.cropBBox ( bbox );
        return createExtendedCompoundImage(l,bbox,cropBBox,dst_crs,servicesXML,width,height,interpolation,error);
    }

}

Image *Pyramid::createExtendedCompoundImage(std::string l, BoundingBox<double> bbox, BoundingBox<double> cropBBox,CRS dst_crs, ServicesXML* servicesXML, int width, int height, Interpolation::KernelType interpolation, int error){

    ExtendedCompoundImageFactory facto;
    std::vector<Image*> images;
    LOGGER_DEBUG ( _ ( "BBox en dehors de la definition du CRS" ) );


    if ( cropBBox.xmin == cropBBox.xmax || cropBBox.ymin == cropBBox.ymax ) { // BBox out of CRS definition area Only NoData
        LOGGER_DEBUG ( _ ( "BBox decoupe incorrect" ) );
    } else {

        double ratio_x = ( cropBBox.xmax - cropBBox.xmin ) / ( bbox.xmax - bbox.xmin );
        double ratio_y = ( cropBBox.ymax - cropBBox.ymin ) / ( bbox.ymax - bbox.ymin ) ;
        int newWidth = lround(width * ratio_x);
        int newHeigth = lround(height * ratio_y);


//Avec lround, taille en pixel et cropBBox ne sont plus cohérents.
//On ajoute la différence de l'arrondi dans la cropBBox et on ajoute un pixel en plus tout autour.

//Calcul de l'erreur d'arrondi converti en coordonnées
        double delta_h = double (newHeigth) - double(height) * ratio_y ;
        double delta_w = double (newWidth) - double(width) * ratio_x ;

        double res_y = ( cropBBox.ymax - cropBBox.ymin ) / double(height * ratio_y) ;
        double res_x = ( cropBBox.xmax - cropBBox.xmin ) / double(width * ratio_x) ;

        double delta_y = res_y * delta_h ;
        double delta_x = res_x * delta_w ;

//Ajout de l'erreur d'arrondi et le pixel en plus
        cropBBox.ymax += delta_y +res_y;
        cropBBox.ymin -= res_y;

        cropBBox.xmax += delta_x +res_x;
        cropBBox.xmin -= res_x;

        newHeigth += 2;
        newWidth += 2;

        LOGGER_DEBUG ( _ ( "New Width = " ) << newWidth << " " << _ ( "New Height = " ) << newHeigth );
        LOGGER_DEBUG ( _ ( "ratio_x = " ) << ratio_x << " " << _ ( "ratio_y = " ) << ratio_y );


        Image* tmp = 0;
        int cropError = 0;
        if ( (1/ratio_x > 5 && newWidth < 3) || (newHeigth < 3 && 1/ratio_y > 5) ){ //Too small BBox
            LOGGER_DEBUG ( _ ( "BBox decoupe incorrect" ) );
            tmp = 0;
        } else if ( newWidth > 0 && newHeigth > 0 ) {
            tmp = levels[l]->getbbox ( servicesXML, cropBBox, newWidth, newHeigth, tms->getCrs(), dst_crs, interpolation, cropError );
        }
        if ( tmp != 0 ) {
            LOGGER_DEBUG ( _ ( "Image decoupe valide" ) );
            images.push_back ( tmp );
        }
    }

    if ( images.empty() ) {
        EmptyImage* fond = new EmptyImage(width, height, channels, ndValues);
        fond->setBbox(bbox);
        return fond;
    }

    return facto.createExtendedCompoundImage ( width,height,channels,bbox,images,ndValues,0 );

}

Image *Pyramid::createBasedSlab(std::string l, BoundingBox<double> bbox, CRS dst_crs, ServicesXML* servicesXML, int width, int height, Interpolation::KernelType interpolation, int error){

    LOGGER_INFO ( "Create Based Slab " );
    //variables
    BoundingBox<double> askBbox = bbox;
    BoundingBox<double> dataBbox = levels[l]->TMLimitsToBbox();

    //on regarde si elle n'est pas en dehors de la defintion de son CRS
    if (!dst_crs.validateBBox ( bbox )) {
        LOGGER_DEBUG ( "Bbox plus grande que sa definition dans le CRS, croppe à la taille maximale du CRS " );
        askBbox = dst_crs.cropBBox ( bbox );
    }

    //on met les deux bbox dans le même système de projection
    if ( servicesXML->are_the_two_CRS_equal( tms->getCrs().getProj4Code(), dst_crs.getProj4Code() ) ) {
        LOGGER_DEBUG ( "Les deux CRS sont équivalents " );
    } else {
        LOGGER_DEBUG ( "Conversion de la bbox demandee et de la bbox des donnees en EPSG:4326 " );
        if (askBbox.reproject(dst_crs.getProj4Code(),"epsg:4326") !=0 || dataBbox.reproject(tms->getCrs().getProj4Code(),"epsg:4326") != 0) {
            LOGGER_ERROR("Ne peut pas reprojeter les bbox");
            return NULL;
        }
    }

    //on compare les deux bbox
    if (tms->getCrs() == dst_crs) {
        //elles sont identiques
        LOGGER_DEBUG ( "Les deux bbox sont identiques " );
        return createReprojectedImage(l, bbox, dst_crs, servicesXML, width, height, interpolation, error);
    } else {
        if (askBbox.containsInside(dataBbox)) {
            //les données sont a l'intérieur de la bbox demandée
            LOGGER_DEBUG ( "les données sont a l'intérieur de la bbox demandée " );
            if (dataBbox.reproject("epsg:4326",dst_crs.getProj4Code()) != 0) {
                LOGGER_ERROR("Ne peut pas reprojeter la bbox des données");
                return NULL;
            }
            return createExtendedCompoundImage(l,bbox,dataBbox,dst_crs,servicesXML,width,height,interpolation,error);

        } else {

            if (dataBbox.containsInside(askBbox)) {
                //la bbox demandée est plus petite que les données disponibles
                LOGGER_DEBUG ("la bbox demandée est plus petite que les données disponibles");
                return createReprojectedImage(l, bbox, dst_crs, servicesXML, width, height, interpolation, error);

            } else {

                if (!dataBbox.intersects(askBbox)) {
                    //les deux ne s'intersectent pas donc on renvoit une image de nodata
                    LOGGER_DEBUG ("les deux ne s'intersectent pas donc on renvoit une image de nodata");

                    EmptyImage* fond = new EmptyImage(width, height, channels, ndValues);
                    fond->setBbox(bbox);
                    return fond;

                } else {
                    //les deux s'intersectent
                    LOGGER_DEBUG ("les deux bbox s'intersectent");
                    BoundingBox<double> partBbox = askBbox.cutIntersectionWith(dataBbox);
                    if (partBbox.reproject("epsg:4326",dst_crs.getProj4Code()) != 0) {
                        LOGGER_ERROR("Ne peut pas reprojeter la bbox partielle");
                        return NULL;
                    }
                    return createExtendedCompoundImage(l,bbox,partBbox,dst_crs,servicesXML,width,height,interpolation,error);
                }

            }

        }

    }



}


Pyramid::~Pyramid() {

    if (Rok4Format::isRaster(format)) delete[] ndValues;

    std::map<std::string, Level*>::iterator iLevel;
    for ( iLevel=levels.begin(); iLevel!=levels.end(); iLevel++ )
        delete iLevel->second;

}


Compression::eCompression Pyramid::getSampleCompression() {
    return Rok4Format::getCompression(format);
}

SampleFormat::eSampleFormat Pyramid::getSampleFormat() {
    return Rok4Format::getSampleFormat(format);
}

int Pyramid::getBitsPerSample() {
    return Rok4Format::getBitsPerSample(format);
}

Level* Pyramid::getHighestLevel() { return highestLevel; }
Level* Pyramid::getLowestLevel() { return lowestLevel; }
Level * Pyramid::getFirstLevel() {
    std::map<std::string, Level*>::iterator it ( levels.begin() );
    return it->second;
}
TileMatrixSet* Pyramid::getTms() { return tms; }
std::map<std::string, Level*>& Pyramid::getLevels() { return levels; }

std::set<std::pair<std::string, Level*>, ComparatorLevel> Pyramid::getOrderedLevels(bool asc) {
 
    if (asc) {
        return std::set<std::pair<std::string, Level*>, ComparatorLevel>(levels.begin(), levels.end(), compLevelAsc);
    } else {
        return std::set<std::pair<std::string, Level*>, ComparatorLevel>(levels.begin(), levels.end(), compLevelDesc);
    }

}

Level* Pyramid::getLevel(std::string id) {
    std::map<std::string, Level*>::iterator it= levels.find ( id );
    if ( it == levels.end() ) {
        return NULL;
    }
    return it->second;
}

Level* Pyramid::getUniqueLevel() {
    if (! isBasedPyramid) {
        // Cette fonction est pour les pyramides de base, pour lesquelles on a gardé un seul niveau
        return NULL;
    }

    // Dans le cas d'une pyramide de base à un niveau, highestLevel et lowestLevel sont ce niveau
    return highestLevel;
}

void Pyramid::setUniqueLevel(std::string id) {
    if (! isBasedPyramid) {
        // Cette fonction est pour les pyramides de base, pour lesquelles on ne garde qu'un seul niveau
    }
    std::map<std::string, Level*>::iterator uniqueLev= levels.find ( id );
    if ( uniqueLev == levels.end() ) {
        highestLevel = NULL;
        lowestLevel = NULL;
        return;
    }

    highestLevel = uniqueLev->second;
    lowestLevel = uniqueLev->second;

    return;
}

void Pyramid::removeLevel(std::string id) {

    std::map<std::string, Level*>::iterator lv = levels.find(id);
    delete lv->second;
    lv->second = NULL;
    levels.erase(lv);

}
Rok4Format::eformat_data Pyramid::getFormat() { return format; }
Photometric::ePhotometric Pyramid::getPhotometric() { return photo; }
int Pyramid::getChannels() { return channels; }
bool Pyramid::getContainOdLevels(){ return containOdLevels; }
bool Pyramid::getTransparent(){ return transparent; }
void Pyramid::setTransparent (bool tr) { transparent = tr; }
Style* Pyramid::getStyle(){ return style; }
void Pyramid::setStyle (Style * st) { style = st; }
int* Pyramid::getNdValues() { return ndValues; }
int Pyramid::getFirstNdValues () { return ndValues[0]; }