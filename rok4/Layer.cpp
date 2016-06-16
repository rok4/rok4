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
    this->keyWords = l.keyWords;
    this->dataPyramid = l.pyramid;
    this->styles = l.styles;
    this->minRes = l.minRes;
    this->maxRes = l.maxRes;

    this->WMSCRSList = l.WMSCRSList;
    this->opaque = l.opaque;
    this->authority = l.authority;
    this->resampling = l.resampling;
    this->geographicBoundingBox = l.geographicBoundingBox;
    this->boundingBox = l.boundingBox;
    this->metadataURLs = l.metadataURLs;

    this->getFeatureInfoAvailability = l.getFeatureInfoAvailability;
    this->getFeatureInfoType = l.getFeatureInfoType;
    this->getFeatureInfoBaseURL = l.getFeatureInfoBaseURL;
    this->GFIVersion = l.GFIVersion;

    this->GFIService = l.GFIService;
    this->GFIQueryLayers = l.GFIQueryLayers;
    this->GFILayers = l.GFILayers;
    this->GFIForceEPSG = l.GFIForceEPSG;
}

Image* Layer::getbbox (ServicesXML* servicesConf, BoundingBox<double> bbox, int width, int height, CRS dst_crs, int& error ) {
    error=0;
    return dataPyramid->getbbox (servicesConf, bbox, width, height, dst_crs, resampling, error );
}

std::string Layer::getId() {
    return id;
}

Layer::~Layer() {

    /*for (int i = 0 ; i < WMSCRSList.size() ; i++) {
        CRS* tmp = WMSCRSList.at(i);
        delete tmp;
    }
    WMSCRSList.clear();*/

    delete dataPyramid;
}


std::string Layer::getAbstract() { return abstract; }
bool Layer::getWMSAuthorized() { return WMSAuthorized; }
void Layer::setWMSAuthorized(bool wmsA) { WMSAuthorized = wmsA; }
bool Layer::getWMTSAuthorized() { return WMTSAuthorized; }
void Layer::setWMTSAuthorized(bool wmtsA) { WMTSAuthorized = wmtsA; }
std::string Layer::getAuthority() { return authority; }
std::vector<Keyword>* Layer::getKeyWords() { return &keyWords; }
double Layer::getMaxRes() { return maxRes; }
double Layer::getMinRes() { return minRes; }
bool Layer::getOpaque() { return opaque; }
Pyramid* Layer::getDataPyramid() { return dataPyramid; }
Interpolation::KernelType Layer::getResampling() { return resampling; }
std::string Layer::getDefaultStyle() { return defaultStyle; }
std::vector<Style*> Layer::getStyles() { return styles; }
std::string Layer::getTitle() { return title; }
std::vector<CRS> Layer::getWMSCRSList() { return WMSCRSList; }
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
