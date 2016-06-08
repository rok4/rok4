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
 * \file ConfLoader.cpp
 * \~french
 * \brief Implémenation des fonctions de chargement de la configuration
 * \brief pendant l'initialisation du serveur
 * \~english
 * \brief Implements configuration loader functions
 * \brief during server initialization
 */


#include <dirent.h>
#include <cstdio>
#include "ConfLoader.h"
#include "Rok4Server.h"
#include "Pyramid.h"
#include "tinyxml.h"
#include "tinystr.h"
#include "config.h"
#include "CephPoolContext.h"
#include "SwiftContext.h"
#include "FileContext.h"
#include "Format.h"
#include "MetadataURL.h"
#include "LegendURL.h"
#include <malloc.h>
#include <stdlib.h>
#include <libgen.h>
#include "Interpolation.h"
#include "intl.h"
#include "config.h"
#include "Keyword.h"
#include <fcntl.h>
#include <cstddef>
#include <string>
#include "WebService.h"
#include "EmptyDataSource.h"

/**********************************************************************************************************/
/***************************************** SERVER & SERVICES **********************************************/
/**********************************************************************************************************/

ServerXML* ConfLoader::getTechnicalParam ( std::string serverConfigFile ) {

    return new ServerXML(serverConfigFile);
}

ServicesXML* ConfLoader::buildServicesConf ( std::string servicesConfigFile ) {

    return new ServicesXML ( servicesConfigFile );
}

/**********************************************************************************************************/
/********************************************** STYLES ****************************************************/
/**********************************************************************************************************/

bool ConfLoader::buildStylesList ( ServerXML* serverXML, ServicesXML* servicesXML, std::map<std::string,Style*> &stylesList ) {
    LOGGER_INFO ( _ ( "CHARGEMENT DES STYLES" ) );

    // lister les fichier du repertoire styleDir
    std::vector<std::string> styleFiles;
    std::vector<std::string> styleName;
    std::string styleFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( serverXML->getStylesDir().c_str() ) ) == NULL ) {
        LOGGER_FATAL ( _ ( "Le repertoire des Styles " ) << styleDir << _ ( " n'est pas accessible." ) );
        return false;
    }
    while ( ( fileEntry = readdir ( dir ) ) ) {
        styleFileName = fileEntry->d_name;
        if ( styleFileName.rfind ( ".stl" ) ==styleFileName.size()-4 ) {
            styleFiles.push_back ( styleDir+"/"+styleFileName );
            styleName.push_back ( styleFileName.substr ( 0,styleFileName.size()-4 ) );
        }
    }
    closedir ( dir );

    if ( styleFiles.empty() ) {
        // FIXME:
        // Aucun Style presents.
        LOGGER_FATAL ( _ ( "Aucun fichier *.stl dans le repertoire " ) << styleDir );
        return false;
    }

    // generer les styles decrits par les fichiers.
    for ( unsigned int i=0; i<styleFiles.size(); i++ ) {
        Style * style;
        style = buildStyle ( styleFiles[i], servicesXML );
        if ( style ) {
            stylesList.insert ( std::pair<std::string, Style *> ( styleName[i], style ) );
        } else {
            LOGGER_ERROR ( _ ( "Ne peut charger le style: " ) << styleFiles[i] );
        }
    }

    if ( stylesList.size() ==0 ) {
        LOGGER_FATAL ( _ ( "Aucun Style n'a pu etre charge!" ) );
        return false;
    }

    LOGGER_INFO ( _ ( "NOMBRE DE STYLES CHARGES : " ) <<stylesList.size() );

    return true;
}

Style* ConfLoader::buildStyle ( std::string fileName, ServicesXML* servicesXML ) {
    StyleXML styXML(fileName, servicesXML);

    if ( ! styXML.isOk() ) {
        return NULL;
    }

    return new Style(styXML);
}



/**********************************************************************************************************/
/*********************************************** TMS ******************************************************/
/**********************************************************************************************************/

bool ConfLoader::buildTMSList ( ServerXML* serverXML, std::map<std::string, TileMatrixSet*> &tmsList ) {
    LOGGER_INFO ( _ ( "CHARGEMENT DES TMS" ) );

    // lister les fichier du repertoire tmsDir
    std::vector<std::string> tmsFiles;
    std::string tmsFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( serverXML->getTmsDir().c_str() ) ) == NULL ) {
        LOGGER_FATAL ( _ ( "Le repertoire des TMS " ) << tmsDir << _ ( " n'est pas accessible." ) );
        return false;
    }
    while ( ( fileEntry = readdir ( dir ) ) ) {
        tmsFileName = fileEntry->d_name;
        if ( tmsFileName.rfind ( ".tms" ) ==tmsFileName.size()-4 ) {
            tmsFiles.push_back ( tmsDir+"/"+tmsFileName );
        }
    }
    closedir ( dir );

    if ( tmsFiles.empty() ) {
        // FIXME:
        // Aucun TMS presents. Ce n'est pas necessairement grave si le serveur
        // ne sert pas pour le WMTS et qu'on exploite pas de cache tuile.
        // Cependant pour le moment (07/2010) on ne gere que des caches tuiles
        LOGGER_FATAL ( _ ( "Aucun fichier *.tms dans le repertoire " ) << tmsDir );
        return false;
    }

    // generer les TMS decrits par les fichiers.
    for ( unsigned int i=0; i<tmsFiles.size(); i++ ) {
        TileMatrixSet * tms;
        tms = buildTileMatrixSet ( tmsFiles[i] );
        if ( tms ) {
            tmsList.insert ( std::pair<std::string, TileMatrixSet *> ( tms->getId(), tms ) );
        } else {
            LOGGER_ERROR ( _ ( "Ne peut charger le tms: " ) << tmsFiles[i] );
        }
    }

    if ( tmsList.size() ==0 ) {
        LOGGER_FATAL ( _ ( "Aucun TMS n'a pu etre charge!" ) );
        return false;
    }

    LOGGER_INFO ( _ ( "NOMBRE DE TMS CHARGES : " ) <<tmsList.size() );

    return true;
}

TileMatrixSet* ConfLoader::buildTileMatrixSet ( std::string fileName ) {
    TileMatrixSetXML tmsXML(fileName);

    if ( ! tmsXML.isOk() ) {
        return NULL;
    }

    return new TileMatrixSet(tmsXML);
}


/**********************************************************************************************************/
/********************************************* LAYERS *****************************************************/
/**********************************************************************************************************/

bool ConfLoader::buildLayersList (ServerXML* serverXML, ServicesXML* servicesXML, 
                                  std::map<std::string, TileMatrixSet*> &tmsList, 
                                  std::map<std::string,Style*> &stylesList, 
                                  std::map<std::string,Layer*> &layers) {

    LOGGER_INFO ( _ ( "CHARGEMENT DES LAYERS" ) );
    // lister les fichier du repertoire layerDir
    std::vector<std::string> layerFiles;
    std::string layerFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( serverXML->getLayersDir().c_str() ) ) == NULL ) {
        LOGGER_FATAL ( _ ( "Le repertoire " ) << layerDir << _ ( " n'est pas accessible." ) );
        return false;
    }
    while ( ( fileEntry = readdir ( dir ) ) ) {
        layerFileName = fileEntry->d_name;
        if ( layerFileName.rfind ( ".lay" ) == layerFileName.size()-4 ) {
            layerFiles.push_back ( layerDir+"/"+layerFileName );
        }
    }
    closedir ( dir );

    if ( layerFiles.empty() ) {
        LOGGER_ERROR ( _ ( "Aucun fichier *.lay dans le repertoire " ) << layerDir );
        LOGGER_ERROR ( _ ( "Le serveur n'a aucune donnees à servir. Dommage..." ) );
        //return false;
    }

    // generer les Layers decrits par les fichiers.
    for ( unsigned int i=0; i<layerFiles.size(); i++ ) {
        Layer * layer;
        layer = buildLayer (serverXML, servicesXML, layerFiles[i], tmsList, stylesList);
        if ( layer ) {
            layers.insert ( std::pair<std::string, Layer *> ( layer->getId(), layer ) );
        } else {
            LOGGER_ERROR ( _ ( "Ne peut charger le layer: " ) << layerFiles[i] );
        }
    }

    if ( layers.size() ==0 ) {
        LOGGER_ERROR ( _ ( "Aucun layer n'a pu etre charge!" ) );
        //return false;
    }

    LOGGER_INFO ( _ ( "NOMBRE DE LAYERS CHARGES : " ) << layers.size() );
    return true;
}

Layer * ConfLoader::buildLayer ( std::string fileName, ServerXML* serverXML, ServicesXML* servicesXML, std::map<std::string, TileMatrixSet*> &tmsList, std::map<std::string,Style*> stylesList ) {

    LayerXML layerXML(fileName, serverXML, servicesXML, tmsList, stylesList);
    if ( ! layerXML.isOk() ) {
        return NULL;
    }

    Layer* pLay =  new Layer(layerXML);

    // Si une pyramide est à la demande, on n'authorize pas le WMS car c'est un cas non gérer dans les processus de reponse du serveur
    if (pLay->getDataPyramid()->getOnDemand()) {
        pLay->setWMSAuthorized(false);
    }

    return pLay;
}

/**********************************************************************************************************/
/******************************************** PYRAMIDS ****************************************************/
/**********************************************************************************************************/

Pyramid* ConfLoader::buildPyramid ( std::string fileName, ServerXML* serverXML, ServicesXML* servicesXML, std::map<std::string, TileMatrixSet*> &tmsList , std::map<std::string, Style *> stylesList, bool times ) {
    
    PyramidXML pyrXML(fileName, serverXML, servicesXML, tmsList, stylesList, times);

    if ( ! pyrXML.isOk() ) {
        return NULL;
    }

    return new Pyramid(pyrXML);

    TiXmlDocument doc ( fileName.c_str() );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "Ne peut pas charger le fichier " ) << fileName );
        return NULL;
    }
}

void ConfLoader::cleanParsePyramid(std::map<std::string,std::vector<Source*> > &specificSources, std::vector<Source*> &sSources,std::map<std::string, Level *> &levels) {

    if (sSources.size() != 0) {
        for ( std::vector<int>::size_type i = 0; i != sSources.size(); i++) {
            delete sSources[i];
            sSources[i] = NULL;
        }
        sSources.clear();
    }
    if (specificSources.size() != 0) {
        for ( std::map<std::string,std::vector<Source*> >::iterator lv = specificSources.begin(); lv != specificSources.end(); lv++) {

            if (lv->second.size() != 0) {
                for ( std::vector<int>::size_type i = 0; i != lv->second.size(); i++) {
                    delete lv->second[i];
                    lv->second[i] = NULL;
                }
                lv->second.clear();
            }
        }
        specificSources.clear();
    }
    if (levels.size() != 0) {
        for ( std::map<std::string,Level*>::iterator lv = levels.begin(); lv != levels.end(); lv++) {
            delete lv->second;
            lv->second = NULL;
        }
        levels.clear();
    }

}


int ConfLoader::updatePyrLevel(Pyramid* pyr, TileMatrix *tm, TileMatrixSet *tms) {

    double Res, ratioX, ratioY, resX, resY;
    std::string best_h;

    Res = tm->getRes();

    BoundingBox<double> nBbox = tms->getCrs().getCrsDefinitionArea();

    BoundingBox<double> cBbox = pyr->getTms().getCrs().cropBBoxGeographic(nBbox);

    cBbox = tms->getCrs().cropBBoxGeographic(cBbox);
    BoundingBox<double> cBboxOld = cBbox;
    BoundingBox<double> cBboxNew = cBbox;

    if (cBboxNew.reproject("epsg:4326",tms->getCrs().getProj4Code())==0 &&
        cBboxOld.reproject("epsg:4326",pyr->getTms().getCrs().getProj4Code())==0)
    {

        ratioX = (cBboxOld.xmax - cBboxOld.xmin) / (cBboxNew.xmax - cBboxNew.xmin);
        ratioY = (cBboxOld.ymax - cBboxOld.ymin) / (cBboxNew.ymax - cBboxNew.ymin);

        resX = Res * ratioX;
        resY = Res * ratioY;

        //On recupère le best level de la basedPyramid en cours pour le tm en cours
        best_h = pyr->best_level(resX,resY,true);

    } else {
        //Si une des reprojections n'a pas marché

        best_h = "";

    }

    if (best_h != "") {

        std::vector<std::string> to_delete;
        std::map<std::string, Level*>::iterator lv = pyr->getLevels().begin();

        for ( ; lv != pyr->getLevels().end(); lv++) {
            if (lv->second->getId() != best_h) {
                to_delete.push_back(lv->second->getId());
            }
        }

        for (std::vector<int>::size_type i = 0; i != to_delete.size(); i++) {
            lv = pyr->getLevels().find(to_delete[i]);
            delete lv->second;
            lv->second = NULL;
            pyr->getLevels().erase(lv);
        }

        return 1;

    } else {
        return 0;
    }

}

void ConfLoader::updateTileLimits(uint32_t &minTileCol, uint32_t &maxTileCol, uint32_t &minTileRow, uint32_t &maxTileRow, TileMatrix tm, TileMatrixSet *tms, std::vector<Source *> sources) {

    //On met à jour les Min et Max Tiles une fois que l'on a trouvé un équivalent dans chaque basedPyramid
    // pour le level créé

    int curMinCol, curMaxCol, curMinRow, curMaxRow, bPMinCol, bPMaxCol, bPMinRow, bPMaxRow, minCol, minRow, maxCol, maxRow;
    double xo, yo, res, tileW, tileH, xmin, xmax, ymin, ymax;

    int time = 1;

    if (sources.size() != 0) {

        for (int ip = 0; ip < sources.size(); ip++) {

            if (sources.at(ip)->getType() == PYRAMID) {
                Pyramid *pyr = reinterpret_cast<Pyramid*>(sources.at(ip));
                Level *lv;

                //On récupére les Min et Max de la basedPyramid
                lv = pyr->getLevels().begin()->second;


                bPMinCol = lv->getMinTileCol();
                bPMaxCol = lv->getMaxTileCol();
                bPMinRow = lv->getMinTileRow();
                bPMaxRow = lv->getMaxTileRow();

                //On récupère d'autres informations sur le TM
                xo = lv->getTm().getX0();
                yo = lv->getTm().getY0();
                res = lv->getTm().getRes();
                tileW = lv->getTm().getTileW();
                tileH = lv->getTm().getTileH();

                //On transforme en bbox
                xmin = bPMinCol * tileW * res + xo;
                ymax = yo - bPMinRow * tileH * res;
                xmax = xo + (bPMaxCol+1) * tileW * res;
                ymin = ymax - (bPMaxRow - bPMinRow + 1) * tileH * res;

                BoundingBox<double> MMbbox(xmin,ymin,xmax,ymax);


                //On reprojette la bbox
                MMbbox.reproject(pyr->getTms().getCrs().getProj4Code(), tms->getCrs().getProj4Code());

                //On récupère les Min et Max de Pyr pour ce level dans la nouvelle projection
                xo = tm.getX0();
                yo = tm.getY0();
                res = tm.getRes();
                tileW = tm.getTileW();
                tileH = tm.getTileH();

                curMinRow = floor((yo - MMbbox.ymax) / (tileW * res));
                curMinCol = floor((MMbbox.xmin - xo) / (tileH * res));
                curMaxRow = floor((yo - MMbbox.ymin) / (tileW * res));
                curMaxCol = floor((MMbbox.xmax - xo) / (tileH * res));

                if (curMinRow < 0) {
                    curMinRow = 0;
                }
                if (curMinCol < 0) {
                    curMinCol = 0;
                }
                if (curMaxRow < 0) {
                    curMaxRow = 0;
                }
                if (curMaxCol < 0) {
                    curMaxCol = 0;
                }

                if (time == 1) {
                    minCol = curMinCol;
                    maxCol = curMaxCol;
                    minRow = curMinRow;
                    maxRow = curMaxRow;
                }

                //On teste pour récupèrer la plus grande zone à l'intérieur du TMS
                if (curMinCol >= minTileCol && curMinCol >= 0 && curMinCol <= curMaxCol && curMinCol <= maxTileCol) {
                    if (curMinCol <= minCol) {
                        minCol = curMinCol;
                    }
                }
                if (curMinRow >= minTileRow && curMinRow >= 0 && curMinRow <= curMaxRow && curMinRow <= maxTileRow) {
                    if (curMinRow <= minRow) {
                        minRow = curMinRow;
                    }
                }
                if (curMaxCol <= maxTileCol && curMaxCol >= 0 && curMaxCol >= curMinCol && curMaxCol >= minTileCol) {
                    if (curMaxCol >= maxCol) {
                        maxCol = curMaxCol;
                    }
                }
                if (curMaxRow <= maxTileRow && curMaxRow >= 0 && curMaxRow >= curMinRow && curMaxRow >= minTileRow) {
                    if (curMaxRow >= maxRow) {
                        maxRow = curMaxRow;
                    }
                }

            }

            if (sources.at(ip)->getType() == WEBSERVICE) {

                WebMapService *wms = reinterpret_cast<WebMapService*>(sources.at(ip));

                BoundingBox<double> MMbbox = wms->getBbox();

                //On récupère les Min et Max de Pyr pour ce level dans la nouvelle projection
                xo = tm.getX0();
                yo = tm.getY0();
                res = tm.getRes();
                tileW = tm.getTileW();
                tileH = tm.getTileH();

                curMinRow = floor((yo - MMbbox.ymax) / (tileW * res));
                curMinCol = floor((MMbbox.xmin - xo) / (tileH * res));
                curMaxRow = floor((yo - MMbbox.ymin) / (tileW * res));
                curMaxCol = floor((MMbbox.xmax - xo) / (tileH * res));

                if (curMinRow < 0) {
                    curMinRow = 0;
                }
                if (curMinCol < 0) {
                    curMinCol = 0;
                }
                if (curMaxRow < 0) {
                    curMaxRow = 0;
                }
                if (curMaxCol < 0) {
                    curMaxCol = 0;
                }

                if (time == 1) {
                    minCol = curMinCol;
                    maxCol = curMaxCol;
                    minRow = curMinRow;
                    maxRow = curMaxRow;
                }

                //On teste pour récupèrer la plus grande zone à l'intérieur du TMS
                if (curMinCol >= minTileCol && curMinCol >= 0 && curMinCol <= curMaxCol && curMinCol <= maxTileCol) {
                    if (curMinCol <= minCol) {
                        minCol = curMinCol;
                    }
                }
                if (curMinRow >= minTileRow && curMinRow >= 0 && curMinRow <= curMaxRow && curMinRow <= maxTileRow) {
                    if (curMinRow <= minRow) {
                        minRow = curMinRow;
                    }
                }
                if (curMaxCol <= maxTileCol && curMaxCol >= 0 && curMaxCol >= curMinCol && curMaxCol >= minTileCol) {
                    if (curMaxCol >= maxCol) {
                        maxCol = curMaxCol;
                    }
                }
                if (curMaxRow <= maxTileRow && curMaxRow >= 0 && curMaxRow >= curMinRow && curMaxRow >= minTileRow) {
                    if (curMaxRow >= maxRow) {
                        maxRow = curMaxRow;
                    }
                }

            }


            time++;
        }

    }

    if (minCol > minTileCol ) {
        minTileCol = minCol;
    }
    if (minRow > minTileRow ) {
        minTileRow = minRow;
    }
    if (maxCol < maxTileCol ) {
        maxTileCol = maxCol;
    }
    if (maxRow < maxTileRow ) {
        maxTileRow = maxRow;
    }


}

WebService *ConfLoader::parseWebService(TiXmlElement* sWeb, CRS pyrCRS, Rok4Format::eformat_data pyrFormat, Proxy proxy_default) {

    WebService * ws = NULL;
    std::string url, user, proxy, noProxy,pwd, referer, userAgent, version, layers, styles, format, crs;
    std::map<std::string,std::string> options;
    int timeout, retry, interval, channels;
    std::string name,ndValuesStr,value;
    BoundingBox<double> bbox = BoundingBox<double> (0.,0.,0.,0.);
    std::vector<int> noDataValues;

    TiXmlElement* sUrl = sWeb->FirstChildElement("url");
    if (sUrl && sUrl->GetText()) {
        url = sUrl->GetTextStr();

        std::size_t found = url.find(" ");
        if (found!=std::string::npos) {
          LOGGER_ERROR("Une URL ne peut contenir des espaces");
          return NULL;
      }

      found = url.find("?");
      size_t size = url.size()-1;
      if (found!=std::string::npos && found!=size) {
        LOGGER_ERROR("Une URL ne peut contenir un ou des '?' hormis le dernier qui est un séparateur");
        return NULL;
    }

    if (found==std::string::npos) {
        url = url + "?";
    }

} else {
    LOGGER_ERROR("Une URL doit etre specifiee pour un WebService");
    return NULL;
}

TiXmlElement* sProxy = sWeb->FirstChildElement("proxy");
if (sProxy && sProxy->GetText()) {
    proxy = sProxy->GetTextStr();
} else {
    proxy = proxy_default.proxyName;
}

sProxy = sWeb->FirstChildElement("noProxy");
if (sProxy && sProxy->GetText()) {
    noProxy = sProxy->GetTextStr();
} else {
    noProxy = proxy_default.noProxy;
}

TiXmlElement* sTimeOut = sWeb->FirstChildElement("timeout");
if (sTimeOut && sTimeOut->GetText()) {
    timeout = atoi(sTimeOut->GetText());
} else {
    timeout = DEFAULT_TIMEOUT;
}

TiXmlElement* sRetry = sWeb->FirstChildElement("retry");
if (sRetry && sRetry->GetText()) {
    retry = atoi(sRetry->GetText());
} else {
    retry = DEFAULT_RETRY;
}

TiXmlElement* sInterval = sWeb->FirstChildElement("interval");
if (sInterval && sInterval->GetText()) {
    interval = atoi(sInterval->GetText());
} else {
    interval = DEFAULT_INTERVAL;
}

TiXmlElement* sUser = sWeb->FirstChildElement("user");
if (sUser && sUser->GetText()) {
    user = sUser->GetTextStr();
} else {
    user = "";
}

TiXmlElement* sPwd = sWeb->FirstChildElement("password");
if (sPwd && sPwd->GetText()) {
    pwd = sPwd->GetTextStr();
} else {
    pwd = "";
}

TiXmlElement* sReferer = sWeb->FirstChildElement("referer");
if (sReferer && sReferer->GetText()) {
    referer = sReferer->GetTextStr();
} else {
    referer = "";
}

TiXmlElement* sUserAgent = sWeb->FirstChildElement("userAgent");
if (sUserAgent && sUserAgent->GetText()) {
    userAgent = sUserAgent->GetTextStr();
} else {
    userAgent = "";
}


TiXmlElement* sWMS = sWeb->FirstChildElement("wms");
if (sWMS) {

    TiXmlElement* sVersion = sWMS->FirstChildElement("version");
    if (sVersion && sVersion->GetText()) {
        version = sVersion->GetTextStr();
    } else {
        LOGGER_ERROR("Un WMS doit contenir une version");
        return NULL;
    }

    TiXmlElement* sLayers= sWMS->FirstChildElement("layers");
    if (sLayers && sLayers->GetText()) {
        layers = sLayers->GetTextStr();
    } else {
        LOGGER_ERROR("Un WMS doit contenir un ou des layers séparés par des virgules");
        return NULL;
    }

    TiXmlElement* sStyles = sWMS->FirstChildElement("styles");
    if (sStyles && sStyles->GetText()) {
        styles = sStyles->GetTextStr();
    } else {
        LOGGER_ERROR("Un WMS doit contenir un ou des styles séparés par des virgules");
        return NULL;
    }

    TiXmlElement* sFormat = sWMS->FirstChildElement("format");
    if (sFormat && sFormat->GetText()) {
        format = sFormat->GetTextStr();
        Rok4Format::eformat_data fmt = Rok4Format::fromMimeType(format);
        if (fmt == Rok4Format::UNKNOWN) {
            LOGGER_ERROR("Un WMS doit être requete dans un format lisible par rok4");
            return NULL;
        }
            //Pour le moment, on autorise que deux formats (jpeg et png)
            //car les autres ne sont pas gérer correctement par les decodeurs de Rok4
            //il faudrait notamment creer un decodeur pour le tiff (lecture de l'en-tête, puis decompression)
        if (format != "image/jpeg" && format != "image/png") {
            LOGGER_ERROR("Un WMS doit être requete en image/jpeg ou image/png");
            return NULL;
        }
    } else {
        format = Rok4Format::toString(pyrFormat);
        LOGGER_ERROR("Un WMS doit contenir un format. Par défaut => " << format);
    }

    TiXmlElement* sCrs = sWMS->FirstChildElement("crs");
    if (sCrs && sCrs->GetText()) {
        crs = sCrs->GetTextStr();

            //le crs demandé et le crs de la pyramide en construction doivent être le même
        CRS askedCRS = CRS(crs);
        if (askedCRS != pyrCRS) {
            LOGGER_ERROR("Un WMS doit contenir un crs équivalent à celui de la pyramide en construction");
            return NULL;
        }

    } else {
        crs = pyrCRS.getProj4Code();
        LOGGER_ERROR("Un WMS doit contenir un crs. Par défaut => " << crs);
    }

    TiXmlElement* sChannels = sWMS->FirstChildElement("channels");
    if (sChannels && sChannels->GetText()) {
        channels = atoi(sChannels->GetTextStr().c_str());
    } else {
        LOGGER_ERROR("Un WMS doit contenir un channels");
        return NULL;
    }

    TiXmlElement* sOpt = sWMS->FirstChildElement("option");
    if (sOpt) {

        for ( sOpt; sOpt; sOpt=sOpt->NextSiblingElement() ) {

            name = sOpt->Attribute("name");
            value = sOpt->Attribute("value");

            if (name != "" && value != "") {
                options.insert(std::pair<std::string,std::string> ( name, value));
            }

        }

    }

    TiXmlElement* sBbox = sWMS->FirstChildElement("bbox");
    if (sBbox) {
        if ( ! ( sBbox->Attribute ( "minx" ) ) ) {
            LOGGER_ERROR ( "minx attribute is missing" );
            return NULL;
        }
        if ( !sscanf ( sBbox->Attribute ( "minx" ),"%lf",&bbox.xmin) ) {
            LOGGER_ERROR ( "Le minx est inexploitable:[" << sBbox->Attribute ( "minx" ) << "]" );
            return NULL;
        }
        if ( ! ( sBbox->Attribute ( "miny" ) ) ) {
            LOGGER_ERROR ( "miny attribute is missing" );
            return NULL;
        }
        if ( !sscanf ( sBbox->Attribute ( "miny" ),"%lf",&bbox.ymin ) ) {
            LOGGER_ERROR ("Le miny est inexploitable:[" << sBbox->Attribute ( "miny" ) << "]" );
            return NULL;
        }
        if ( ! ( sBbox->Attribute ( "maxx" ) ) ) {
            LOGGER_ERROR (  "maxx attribute is missing"  );
            return NULL;
        }
        if ( !sscanf ( sBbox->Attribute ( "maxx" ),"%lf",&bbox.xmax ) ) {
            LOGGER_ERROR (  "Le maxx est inexploitable:["  << sBbox->Attribute ( "maxx" ) << "]" );
            return NULL;
        }
        if ( ! ( sBbox->Attribute ( "maxy" ) ) ) {
            LOGGER_ERROR (  "maxy attribute is missing" );
            return NULL;
        }
        if ( !sscanf ( sBbox->Attribute ( "maxy" ),"%lf",&bbox.ymax ) ) {
            LOGGER_ERROR (  "Le maxy est inexploitable:["  << sBbox->Attribute ( "maxy" ) << "]" );
            return NULL;
        }

    } else {
        LOGGER_ERROR("Un WMS doit contenir une bbox");
        return NULL;
    }

    TiXmlElement* pND=sWMS->FirstChildElement ( "noDataValue" );
    if ( pND && pND->GetText() ) {
        ndValuesStr = pND->GetTextStr();

            //conversion string->vector
        std::size_t found = ndValuesStr.find_first_of(",");
        std::string currentValue = ndValuesStr.substr(0,found);
        std::string endOfValues = ndValuesStr.substr(found+1);
        int curVal = atoi(currentValue.c_str());
        if (currentValue == "") {
            curVal = DEFAULT_NODATAVALUE;
        }
        noDataValues.push_back(curVal);
        while (found!=std::string::npos) {
            found = endOfValues.find_first_of(",");
            currentValue = endOfValues.substr(0,found);
            endOfValues = endOfValues.substr(found+1);
            curVal = atoi(currentValue.c_str());
            if (currentValue == "") {
                curVal = DEFAULT_NODATAVALUE;
            }
            noDataValues.push_back(curVal);
        }
        if (noDataValues.size() < channels) {
            LOGGER_ERROR("Le nombre de channels indique est different du nombre de noDataValue donne");
            return NULL;
        }
    } else {
        for (int i=0;i<channels;i++) {
            noDataValues.push_back(DEFAULT_NODATAVALUE);
        }
    }

    ws = new WebMapService(url, proxy, noProxy, retry, interval, timeout, version, layers, styles, format, channels, crs, bbox, noDataValues,options);

} else {
        //On retourne une erreur car le WMS est le seul WebService disponible pour le moment
    LOGGER_ERROR("Un WebService doit contenir un WMS pour être utilisé");
    return NULL;
}

return ws;

}

Pyramid *ConfLoader::parseBasedPyramid(TiXmlElement* sPyr, std::map<std::string, TileMatrixSet*> &tmsList, bool timesSpecific, std::map<std::string,Style*> stylesList, std::string parentDir, Proxy proxy) {

    Pyramid *basedPyramid;

    TiXmlElement* sFile = sPyr->FirstChildElement("file");
    TiXmlElement* sTransparent = sPyr->FirstChildElement("transparent");
    TiXmlElement* sStyle = sPyr->FirstChildElement("style");

    bool transparent = false;
    std::string str_transparent,basedPyramidFilePath;
    std::string str_style = "";
    Style *style = NULL;

    if (sFile && sTransparent && sStyle && sFile->GetText() && sTransparent->GetText() && sStyle->GetText()) {

        str_transparent = sTransparent->GetTextStr();
        str_style = sStyle->GetTextStr();

        basedPyramidFilePath = sFile->GetTextStr() ;
        //Relative Path
        if ( basedPyramidFilePath.compare ( 0,2,"./" ) ==0 ) {
            basedPyramidFilePath.replace ( 0,1,parentDir );
        } else if ( basedPyramidFilePath.compare ( 0,1,"/" ) !=0 ) {
            basedPyramidFilePath.insert ( 0,"/" );
            basedPyramidFilePath.insert ( 0,parentDir );
        }

        basedPyramid = buildPyramid ( basedPyramidFilePath, tmsList, NULL, NULL, timesSpecific, stylesList, proxy );

        if ( !basedPyramid) {
            LOGGER_ERROR ( _ ( "La pyramide " ) << basedPyramidFilePath << _ ( " ne peut etre chargee" ) );
            return NULL;
        } else {


            if (str_transparent == "true") {
                transparent = true;
                basedPyramid->setTransparent(transparent);
            } else {
                basedPyramid->setTransparent(transparent);
            }

            std::map<std::string, Style*>::iterator styleIt= stylesList.find ( str_style );
            if ( styleIt == stylesList.end() ) {
                LOGGER_ERROR ( _ ( "Style " ) << str_style << _ ( "non defini" ) );
                styleIt= stylesList.find ( "normal" );
                if (styleIt != stylesList.end()) {
                    style = styleIt->second;
                }
            } else {
                style = styleIt->second;
            }

            basedPyramid->setStyle(style);

        }

    } else {
        //Il manque un des trois elements necessaires pour initialiser une
        //nouvelle pyramide de base
        LOGGER_ERROR ( _ ( "Pyramid: " ) << basedPyramidFilePath << _ ( " can't be loaded because information are missing" ) );
        return NULL;
    }

    return basedPyramid;

}






std::vector<std::string> ConfLoader::loadListEqualsCRS(){
    // Build the list (vector of string) of equals CRS from a file given in parameter
    char * fileCRS = "/listofequalscrs.txt";
    char * dirCRS = getenv ( "PROJ_LIB" ); // Get path from config
    char namebuffer[100];
    strcpy(namebuffer, dirCRS);
    strcat(namebuffer, fileCRS);
    LOGGER_INFO ( _ ( "Construction de la liste des CRS equivalents depuis " ) << namebuffer );
    std::vector<std::string> rawStrVector = loadStringVectorFromFile(std::string(namebuffer));
    std::vector<std::string> strVector;
    // rawStrVector can cointains some unknowned CRS => filtering using Proj4
    for ( unsigned int l=0; l<rawStrVector.size(); l++ ){
        std::string line = rawStrVector.at( l );
        std::string crsstr = "";
        std::string targetLine;
        //split
        size_t start_index = 0;
        size_t len = 0;
        size_t found_space = 0;
        while ( found_space != std::string::npos ){
            found_space = line.find(" ", start_index );
            if ( found_space == std::string::npos ) {
                len = line.size() - start_index ; //-1 pour le retour chariot
            } else {
                len = found_space - start_index;
            }
            crsstr = line.substr( start_index, len );
            
            //is the new CRS compatible with Proj4 ?
            CRS crs ( crsstr );
            if ( !crs.isProj4Compatible() ) {
                LOGGER_WARN ( _ ( "The Equivalent CRS [" ) << crsstr << _ ( "] is not present in Proj4" ) );
            } else {
                targetLine.append( crsstr );
                targetLine.append( " " );
            }
            
            start_index = found_space + 1;
        }
        if (targetLine.length() != 0) {
           strVector.push_back( targetLine.substr(0, targetLine.length()) );
       }
   }
   return strVector;
}


std::vector<std::string> ConfLoader::loadStringVectorFromFile(std::string file){
    std::vector<std::string> strVector;
    std::ifstream input ( file.c_str() );
    // We test if the stream is empty
    //   This can happen when the file can't be loaded or when the file is empty
    if ( input.peek() == std::ifstream::traits_type::eof() ) {
        LOGGER_ERROR ( _ ("Ne peut pas charger le fichier ") << file << _ (" ou fichier vide")  );
    }
    
    for( std::string line; getline(input, line); ) {
        if (line[0] != '#' ){
            strVector.push_back( line );
        }
    }
    return strVector;
}

std::vector<CRS> ConfLoader::getEqualsCRS(std::vector<std::string> listofequalsCRS, std::string basecrs)
{
    std::vector<CRS> returnCRS;
    for ( unsigned int l=0; l<listofequalsCRS.size(); l++ ){
        std::string workingbasecrs(basecrs);
        workingbasecrs.append(" ");
        size_t found = listofequalsCRS.at( l ).find ( workingbasecrs );
        if (found == std::string::npos){
            size_t found = listofequalsCRS.at( l ).find ( basecrs );
            if ( found != ( listofequalsCRS.at( l ).size() - basecrs.size()) ){
                found = std::string::npos;
            }
        }
        if (found != std::string::npos) {
            //Global CRS found !
            std::string line = listofequalsCRS.at( l );
            std::string crsstr = "";
            //split
            size_t start_index = 0;
            size_t len = 0;
            size_t found_space = 0;
            while ( found_space != std::string::npos ){
                found_space = line.find(" ", start_index );
                if ( found_space == std::string::npos ) {
                    len = line.size() - start_index ; //-1 pour le retour chariot
                } else {
                    len = found_space - start_index;
                }
                crsstr = line.substr( start_index, len );
                
                if ( crsstr.compare(basecrs) != 0 ){
                    //is the new CRS compatible with Proj4 ?
                    CRS crs ( crsstr );
                    if ( !crs.isProj4Compatible() ) {
                        LOGGER_DEBUG ( _ ( "The Equivalent CRS [" ) << crsstr << _ ( "] of [" ) << basecrs << _ ( "] is not present in Proj4" ) );
                    } else {
                        returnCRS.push_back( crs );
                    }
                }
                start_index = found_space + 1;
            }
        }
    }
    return returnCRS;
}

bool ConfLoader::isCRSAllowed(std::vector<std::string> restrictedCRSList, std::string crs, std::vector<CRS> equiCRSList){
    bool allowedCRS = false;
    //Is the CRS allowed ?
    for (unsigned int l = 0 ; l<restrictedCRSList.size() ; l++){
        if ( crs.compare( restrictedCRSList.at( l ) ) == 0 ){
            allowedCRS = true;
            break;
        }
    }
    if (allowedCRS){
        return true;
    }
    //Is an equivalent of this CRS allowed ?
    for (unsigned int k = 0 ; k < equiCRSList.size(); k++ ){
        std::string equicrsstr = equiCRSList.at( k ).getRequestCode();
        for (unsigned int l = 0 ; l<restrictedCRSList.size() ; l++){
            if ( equicrsstr.compare( restrictedCRSList.at( l ) ) == 0 ){
                allowedCRS = true;
                break;
            }
        }
        if (allowedCRS){
            break;
        }
    }
    return allowedCRS;
}


