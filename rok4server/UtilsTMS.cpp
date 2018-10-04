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
 * \file CapabilitiesBuilder.cpp
 * \~french
 * \brief Implémentation des fonctions de générations des GetCapabilities
 * \~english
 * \brief Implement the GetCapabilities generation function
 */

#include "Rok4Server.h"
#include "tinyxml.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <cmath>
#include "TileMatrixSet.h"
#include "Pyramid.h"
#include "intl.h"


DataSource* Rok4Server::getTileParamTMS ( Request* request, Layer*& layer, std::string& str_tileMatrix, int& tileCol, int& tileRow, std::string& format, Style*& style) {
    
    // On cherche la version 1.0.0 dans le path pour savoir où trouver les informations
    
    std::stringstream ss(request->path);
    std::string token;
    char delim = '/';
    int tmsVersionPos = -1;
    
    std::vector<std::string> pathParts;
    while (std::getline(ss, token, delim)) {
        if (token == "1.0.0") {
            tmsVersionPos = pathParts.size();
        }
        pathParts.push_back(token);
    }

    if (tmsVersionPos == -1) {
        // La version n'a pas été rencontrée
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre VERSION absent." ),"tms" ) );
    }

    if (tmsVersionPos == pathParts.size() - 1) {
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre LAYER absent." ),"tms" ) );
    }
    if (tmsVersionPos == pathParts.size() - 2) {
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre Z absent." ),"tms" ) );
    }
    if (tmsVersionPos == pathParts.size() - 3) {
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre X absent." ),"tms" ) );
    }
    if (tmsVersionPos == pathParts.size() - 4) {
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre Y absent." ),"tms" ) );
    }

    // La couche
    std::string str_layer = pathParts.at(tmsVersionPos + 1);
    layer = serverConf->getLayer(str_layer);

    if ( layer == NULL || ! layer->getTMSAuthorized() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Layer " ) +str_layer+_ ( " inconnu." ),"tms" ) );


    // Le niveau
    str_tileMatrix = pathParts.at(tmsVersionPos + 2);
    TileMatrixSet* tms = layer->getDataPyramid()->getTms();
    if ( tms->getTm(str_tileMatrix) == NULL )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "TileMatrix " ) +str_tileMatrix+_ ( " inconnu pour le TileMatrixSet " ) +tms->getId(),"wmts" ) );


    // La colonne
    std::string str_TileCol = pathParts.at(tmsVersionPos + 3);
    if ( sscanf ( str_TileCol.c_str(),"%d",&tileCol ) != 1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre TILECOL est incorrecte." ),"tms" ) );

    // La ligne        
    std::string tileRowWithExtension = pathParts.at(tmsVersionPos + 4);
    std::string str_TileRow;
    std::string extension;
    delim = '.';
    ss = std::stringstream(tileRowWithExtension);
    std::getline(ss, str_TileRow, delim);
    std::getline(ss, extension, delim);

    if ( sscanf ( str_TileRow.c_str(),"%d",&tileRow ) != 1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre TILEROW est incorrecte." ),"tms" ) );

    // Le style
    style = serverConf->getStyle(layer->getDefaultStyle());

    // Le format : on vérifie la cohérence de l'extension avec le format des données

    if ( extension == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Extension absente." ),"tms" ) );

    if ( extension.compare ( Rok4Format::toExtension ( ( layer->getDataPyramid()->getFormat() ) ) ) != 0 ) {
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "L'extension " ) +extension+_ ( " n'est pas gere pour la couche " ) +str_layer,"tms" ) );
    }

    format = Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) );

    return NULL;
}


DataStream* Rok4Server::getLayerParamTMS ( Request* request, Layer*& layer, std::string& url ) {
    // On cherche la version 1.0.0 dans le path pour savoir où trouver les informations
    
    std::stringstream ss(request->path);
    std::string token;
    char delim = '/';
    int tmsVersionPos = -1;

    url = request->scheme + request->hostName;

    
    std::vector<std::string> pathParts;
    while (std::getline(ss, token, delim)) {
        if (tmsVersionPos == -1) {
            // On n'a pas encore rencontré la version (c'est peut-être ce morceau là)
            // On ajoute doncà notre URL
            url += token + "/";
        }
        if (token == "1.0.0") {
            tmsVersionPos = pathParts.size();
        }
        pathParts.push_back(token);
    }


    if (tmsVersionPos == -1) {
        // La version n'a pas été rencontrée
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre VERSION absent." ),"tms" ) );
    }
    
    // On enlève le dernier slash de trop
    url.pop_back();

    if (tmsVersionPos == pathParts.size() - 1) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre LAYER absent." ),"tms" ) );
    }

    // La couche
    std::string str_layer = pathParts.at(tmsVersionPos + 1);
    layer = serverConf->getLayer(str_layer);

    if ( layer == NULL || ! layer->getTMSAuthorized() ) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Layer " ) +str_layer+_ ( " inconnu." ),"tms" ) );
    }


    return NULL;
}

DataStream* Rok4Server::TMSGetLayer ( Request* request ) {

    Layer* layer;
    std::string serviceURL;
    DataStream* errorResp = getLayerParamTMS(request, layer, serviceURL);

    if ( errorResp ) {
        return errorResp;
    }
    errorResp = NULL;


    std::ostringstream res;
    res << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    res << "<TileMap version=\"1.0.0\" tilemapservice=\"" + serviceURL + "\">\n";
    res << "  <Title>" << layer->getTitle() << "</Title>\n";
 // | <KeywordList></KeywordList>
    res << "  <SRS>" << layer->getDataPyramid()->getTms()->getCrs().getRequestCode() << "</SRS>\n";
    res << "  <BoundingBox minx=\"" <<
        layer->getBoundingBox().minx << "\" miny=\"" <<
        layer->getBoundingBox().miny << "\" maxx=\"" <<
        layer->getBoundingBox().maxx << "\" maxy=\"" <<
        layer->getBoundingBox().maxy << "\" />\n";

    TileMatrix* tm = layer->getDataPyramid()->getTms()->getTmList()->begin()->second;

    res << "  <Origin x=\"" << tm->getX0() << "\" y=\"" << tm->getY0() << "\" />\n";

    res << "  <TileFormat width=\"" << tm->getTileW() << 
        "\" height=\"" << tm->getTileH() <<
        "\" mime-type=\"" << Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) << 
        "\" extension=\"" << Rok4Format::toExtension ( ( layer->getDataPyramid()->getFormat() ) ) << "\" />\n";

    res << "  <TileSets profile=\"none\">\n";

    int order = 0;
    std::map<std::string, Level*> layerLevelList = layer->getDataPyramid()->getLevels();

    // On va déclarer un comparateur pour lire les niveaux dans l'ordre des résolutions décroissante
    typedef std::function<bool(std::pair<std::string, Level*>, std::pair<std::string, Level*>)> Comparator;
    Comparator compFunctor =
            [](std::pair<std::string, Level*> elem1 ,std::pair<std::string, Level*> elem2)
            {
                return elem1.second->getRes() > elem2.second->getRes();
            };
 
    // Declaring a set that will store the pairs using above comparision logic
    std::set<std::pair<std::string, Level*>, Comparator> orderedLevels(
            layerLevelList.begin(), layerLevelList.end(), compFunctor);

    std::map<std::string, Level*>::iterator itLevelList ( layerLevelList.begin() );

    for (std::pair<std::string, Level*> element : orderedLevels) {
        Level * level = element.second;
        tm = layer->getDataPyramid()->getTms()->getTm(level->getId());
        res << "    <TileSet href=\"" << serviceURL << "/" << layer->getId() << "/" << tm->getId() << 
            "\" units-per-pixel=\"" << tm->getRes() << "\" order=\"" << order << "\" />\n";
        order++;
    }

    res << "  </TileSets>\n";
    res << "</TileMap>\n";

    return new MessageDataStream ( res.str(),"application/xml" );
}

DataStream* Rok4Server::TMSGetLayerMetadata ( Request* request ) {

    Layer* layer;
    std::string serviceURL;
    DataStream* errorResp = getLayerParamTMS(request, layer, serviceURL);

    if ( errorResp ) {
        return errorResp;
    }
    errorResp = NULL;


    std::ostringstream res;
    res << "{}";

    return new MessageDataStream ( res.str(),"application/json" );
}

DataStream* Rok4Server::TMSGetCapabilities ( Request* request ) {


    std::string version;

    /* concaténation des fragments invariant de capabilities en intercalant les
      * parties variables dépendantes de la requête */
    std::string capa = "";

    std::string url = request->scheme + request->hostName + request->path;

    if (url.compare ( url.size()-1,1,"/" ) == 0) {
        url.pop_back();
    }

    for ( int i=0; i < tmsCapaFrag.size()-1; i++ ) {
        capa = capa + tmsCapaFrag[i] + url;
    }
    capa = capa + tmsCapaFrag.back();

    return new MessageDataStream ( capa,"application/xml" );
}


DataStream* Rok4Server::TMSGetServices ( Request* request ) {

    std::string url = request->scheme + request->hostName + request->path;

    if (url.compare ( url.size()-1,1,"/" ) == 0) {
        url.pop_back();
    }

    std::ostringstream res;
    res << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    res << "<Services>\n";
    if (serverConf->supportTMS) {
        res << "  <TileMapService title=\"" << servicesConf->getTitle() << "\" version=\"1.0.0\" href=\"" << url << "/1.0.0/\" />\n";
    }
    if (serverConf->supportWMS) {
        res << "  <WebMapService title=\"" << servicesConf->getTitle() << "\" version=\"1.1.1\" href=\"" << url << "?SERVICE=WMS&amp;VERSION=1.1.1&amp;REQUEST=GetCapabilities\" />\n";
        res << "  <WebMapService title=\"" << servicesConf->getTitle() << "\" version=\"1.3.0\" href=\"" << url << "?SERVICE=WMS&amp;VERSION=1.3.0&amp;REQUEST=GetCapabilities\" />\n";
    }
    if (serverConf->supportWMTS) {
        res << "  <WebMapTileService title=\"" << servicesConf->getTitle() << "\" version=\"1.0.0\" href=\"" << url << "?SERVICE=WMTS&amp;VERSION=1.0.0&amp;REQUEST=GetCapabilities\" />\n";
    }
    res << "</Services>\n";



    return new MessageDataStream ( res.str(),"application/xml" );
}

void Rok4Server::buildTMSCapabilities() {
    std::string pathTag = "]HOSTNAME["; // Tag à remplacer par le chemin complet jusqu'à la version comprise

    std::string tmsCapaTemplate = "";
    tmsCapaTemplate += "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    tmsCapaTemplate += "<TileMapService version=\"1.0.0\" services=\"" + pathTag + "\">\n";
    tmsCapaTemplate += "  <Title>" + servicesConf->getTitle() + "</Title>\n";
    tmsCapaTemplate += "  <Abstract>" + servicesConf->getAbstract() + "</Abstract>\n";
    tmsCapaTemplate += "  <TileMaps>\n";

    std::map<std::string, Layer*>::iterator itLay ( serverConf->layersList.begin() ), itLayEnd ( serverConf->layersList.end() );
    for ( ; itLay != itLayEnd; ++itLay ) {
        Layer* lay = itLay->second;

        if (lay->getTMSAuthorized()) {
            tmsCapaTemplate += "    <TileMap\n";
            tmsCapaTemplate += "      title=\"" + lay->getTitle() + "\" \n";
            tmsCapaTemplate += "      srs=\"" + lay->getDataPyramid()->getTms()->getCrs().getRequestCode() + "\" \n";
            tmsCapaTemplate += "      profile=\"none\" \n";
            tmsCapaTemplate += "      href=\"" + pathTag + "/" + lay->getId() + "\" />\n";
        }
    }


    tmsCapaTemplate += "  </TileMaps>\n";
    tmsCapaTemplate += "</TileMapService>\n";

    // Découpage en fragments constants.
    size_t beginPos;
    size_t endPos;
    beginPos= 0;
    endPos  = tmsCapaTemplate.find ( pathTag );
    while ( endPos != std::string::npos ) {
        tmsCapaFrag.push_back ( tmsCapaTemplate.substr ( beginPos,endPos-beginPos ) );
        beginPos = endPos + pathTag.length();
        endPos=tmsCapaTemplate.find ( pathTag,beginPos );
    }
    tmsCapaFrag.push_back ( tmsCapaTemplate.substr ( beginPos ) );
}