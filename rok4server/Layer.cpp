/*
 * Copyright © (2011-2013) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
 * \file Layer.cpp
 * \~french
 * \brief Implémentation de la classe Layer modélisant les couches de données.
 * \~english
 * \brief Implement the Layer Class handling data layer.
 */

#include "Layer.h"
#include "Pyramid.h"
#include "Logger.h"

GeographicBoundingBoxWMS::GeographicBoundingBoxWMS() {}
BoundingBoxWMS::BoundingBoxWMS() {}

Layer::Layer ( const LayerXML& l ) {
    this->id = l.id;
    this->title = l.title;
    this->abstract = l.abstract;
    this->WMSAuthorized = l.WMSauth;
    this->WMTSAuthorized = l.WMTSauth;
    this->TMSAuthorized = l.TMSauth;
    this->keyWords = l.keyWords;
    this->dataPyramidFilePath = l.pyramidFilePath;
    this->dataPyramid = l.pyramid;
    this->authority = l.authority;
    this->geographicBoundingBox = l.geographicBoundingBox;
    this->boundingBox = l.boundingBox;
    this->metadataURLs = l.metadataURLs;

    if (Rok4Format::isRaster(this->dataPyramid->getFormat())) {

        // Si la pyramide raster contient un niveau à la demande ou à la volée, on n'authorize pas le WMS 
        // car c'est un cas non géré dans les processus de reponse du serveur
        if (this->dataPyramid->getContainOdLevels()) {
            this->WMSAuthorized = false;
        }

        this->styles = l.styles;
        this->defaultStyle = l.styles[0]->getIdentifier();
        this->minRes = l.minRes;
        this->maxRes = l.maxRes;

        this->WMSCRSList = l.WMSCRSList;
        this->getFeatureInfoAvailability = l.getFeatureInfoAvailability;
        this->getFeatureInfoType = l.getFeatureInfoType;
        this->getFeatureInfoBaseURL = l.getFeatureInfoBaseURL;
        this->GFIVersion = l.GFIVersion;

        this->GFIService = l.GFIService;
        this->GFIQueryLayers = l.GFIQueryLayers;
        this->GFILayers = l.GFILayers;
        this->GFIForceEPSG = l.GFIForceEPSG;
        this->resampling = l.resampling;

    } else {
        // Une pyramide vecteur n'est diffusée qu'en WMTS et TMS et le GFI n'est pas possible
        this->WMSAuthorized = false;
        this->getFeatureInfoAvailability = false;
    }
}

Layer::Layer (Layer* obj, ServerXML* sxml) {
    id = obj->id;
    title = obj->title;
    abstract = obj->abstract;
    WMSAuthorized = obj->WMSAuthorized;
    WMTSAuthorized = obj->WMTSAuthorized;
    TMSAuthorized = obj->TMSAuthorized;
    keyWords = obj->keyWords;
    dataPyramidFilePath = obj->dataPyramidFilePath;
    authority = obj->authority;
    geographicBoundingBox = obj->geographicBoundingBox;
    boundingBox = obj->boundingBox;
    metadataURLs = obj->metadataURLs;

    // On clone la pyramide de données
    dataPyramid = new Pyramid(obj->dataPyramid, sxml);

    // Détection d'erreurs
    if (dataPyramid->getTms() == NULL) {
        LOGGER_ERROR("Impossible de cloner la pyramide");
        delete dataPyramid;
        dataPyramid = NULL;
        return;
    }

    if (Rok4Format::isRaster(this->dataPyramid->getFormat())) {
        /******************* PYRAMIDE RASTER *********************/

        minRes = obj->minRes;
        maxRes = obj->maxRes;

        WMSCRSList = obj->WMSCRSList;
        resampling = obj->resampling;

        getFeatureInfoAvailability = obj->getFeatureInfoAvailability;
        getFeatureInfoType = obj->getFeatureInfoType;
        getFeatureInfoBaseURL = obj->getFeatureInfoBaseURL;
        GFIVersion = obj->GFIVersion;

        GFIService = obj->GFIService;
        GFIQueryLayers = obj->GFIQueryLayers;
        GFILayers = obj->GFILayers;
        GFIForceEPSG = obj->GFIForceEPSG;

        // On met à jour les pointeurs des styles avec ceux de la nouvelle liste
        for (unsigned i = 0 ; i < obj->styles.size(); i++) {

            // On récupère bien le pointeur vers le nouveau TMS (celui de la nouvelle liste)
            Style* s = sxml->getStyle ( obj->styles.at(i)->getId() );
            if ( s == NULL ) {
                LOGGER_ERROR ( "Une couche clonée reference un style [" << obj->styles.at(i)->getId() << "] qui n'existe plus." );
                return;
                // Tester la nullité de la pyramide de donnée en sortie pour faire remonter l'erreur
            } else {
                styles.push_back(s);
            }
        }
        defaultStyle = styles[0]->getIdentifier();
    }
}

Image* Layer::getbbox (ServicesXML* servicesConf, BoundingBox<double> bbox, int width, int height, CRS dst_crs, int dpi, int& error ) {
    error=0;
    return dataPyramid->getbbox (servicesConf, bbox, width, height, dst_crs, resampling, dpi, error );
}

std::string Layer::getId() {
    return id;
}

Layer::~Layer() {

    delete dataPyramid;
}


std::string Layer::getAbstract() { return abstract; }
bool Layer::getWMSAuthorized() { return WMSAuthorized; }
bool Layer::getTMSAuthorized() { return TMSAuthorized; }
bool Layer::getWMTSAuthorized() { return WMTSAuthorized; }
std::string Layer::getAuthority() { return authority; }
std::vector<Keyword>* Layer::getKeyWords() { return &keyWords; }
double Layer::getMaxRes() { return maxRes; }
double Layer::getMinRes() { return minRes; }
Pyramid* Layer::getDataPyramid() { return dataPyramid; }
std::string Layer::getDataPyramidFilePath() { return dataPyramidFilePath; }
Interpolation::KernelType Layer::getResampling() { return resampling; }
std::string Layer::getDefaultStyle() { return defaultStyle; }
std::vector<Style*> Layer::getStyles() { return styles; }
Style* Layer::getStyle(std::string id) {
    for ( unsigned int i = 0; i < styles.size(); i++ ) {
        if ( id == styles[i]->getId() )
            return styles[i];
    }
    return NULL;
}

Style* Layer::getStyleByIdentifier(std::string identifier) {
    for ( unsigned int i = 0; i < styles.size(); i++ ) {
        if ( identifier == styles[i]->getIdentifier() )
            return styles[i];
    }
    return NULL;
}

std::string Layer::getTitle() { return title; }
std::vector<CRS> Layer::getWMSCRSList() { return WMSCRSList; }
bool Layer::isInWMSCRSList(CRS* c) {
    for ( unsigned int k = 0; k < WMSCRSList.size(); k++ ) {
        if ( c->cmpRequestCode ( WMSCRSList.at (k).getRequestCode() ) ) {
            return true;
        }
    }
    return false;
}

GeographicBoundingBoxWMS Layer::getGeographicBoundingBox() { return geographicBoundingBox; }
BoundingBoxWMS Layer::getBoundingBox() { return boundingBox; }
std::vector<MetadataURL> Layer::getMetadataURLs() { return metadataURLs; }
bool Layer::isGetFeatureInfoAvailable() { return getFeatureInfoAvailability; }
std::string Layer::getGFIType() { return getFeatureInfoType; }
std::string Layer::getGFIBaseUrl() { return getFeatureInfoBaseURL; }
std::string Layer::getGFILayers() { return GFILayers; }
std::string Layer::getGFIQueryLayers() { return GFIQueryLayers; }
std::string Layer::getGFIService() { return GFIService; }
std::string Layer::getGFIVersion() { return GFIVersion; }
bool Layer::getGFIForceEPSG() { return GFIForceEPSG; }
