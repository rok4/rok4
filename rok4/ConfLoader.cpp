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
#include <map>
#include <fcntl.h>
#include <cstddef>
#include <malloc.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>

#include "ConfLoader.h"

/**********************************************************************************************************/
/***************************************** SERVER & SERVICES **********************************************/
/**********************************************************************************************************/

ServerXML* ConfLoader::getTechnicalParam ( std::string serverConfigFile ) {

    return new ServerXML( serverConfigFile );
}

ServicesXML* ConfLoader::buildServicesConf ( std::string servicesConfigFile ) {

    return new ServicesXML ( servicesConfigFile );
}

/**********************************************************************************************************/
/********************************************** STYLES ****************************************************/
/**********************************************************************************************************/

bool ConfLoader::buildStylesList ( ServerXML* serverXML, ServicesXML* servicesXML ) {
    LOGGER_INFO ( _ ( "CHARGEMENT DES STYLES" ) );

    // lister les fichier du repertoire styleDir
    std::vector<std::string> styleFiles;
    std::vector<std::string> styleName;
    std::string styleDir = serverXML->getStylesDir();
    std::string styleFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( styleDir.c_str() ) ) == NULL ) {
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
            serverXML->addStyle ( styleName[i], style );
        } else {
            LOGGER_ERROR ( _ ( "Ne peut charger le style: " ) << styleFiles[i] );
        }
    }

    if ( serverXML->getNbStyles() ==0 ) {
        LOGGER_FATAL ( _ ( "Aucun Style n'a pu etre charge!" ) );
        return false;
    }

    LOGGER_INFO ( _ ( "NOMBRE DE STYLES CHARGES : " ) << serverXML->getNbStyles() );

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

bool ConfLoader::buildTMSList ( ServerXML* serverXML ) {
    LOGGER_INFO ( _ ( "CHARGEMENT DES TMS" ) );

    // lister les fichier du repertoire tmsDir
    std::vector<std::string> tmsFiles;
    std::string tmsFileName;
    std::string tmsDir = serverXML->getTmsDir();
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( tmsDir.c_str() ) ) == NULL ) {
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
            serverXML->addTMS ( tms->getId(), tms );
        } else {
            LOGGER_ERROR ( _ ( "Ne peut charger le tms: " ) << tmsFiles[i] );
        }
    }

    if ( serverXML->getNbTMS() ==0 ) {
        LOGGER_FATAL ( _ ( "Aucun TMS n'a pu etre charge!" ) );
        return false;
    }

    LOGGER_INFO ( _ ( "NOMBRE DE TMS CHARGES : " ) << serverXML->getNbTMS() );

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

bool ConfLoader::buildLayersList ( ServerXML* serverXML, ServicesXML* servicesXML ) {

    LOGGER_INFO ( _ ( "CHARGEMENT DES LAYERS" ) );
    // lister les fichier du repertoire layerDir
    std::vector<std::string> layerFiles;
    std::string layerDir = serverXML->getLayersDir();
    std::string layerFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( layerDir.c_str() ) ) == NULL ) {
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
        layer = buildLayer ( layerFiles[i], serverXML, servicesXML );
        if ( layer ) {
            serverXML->addLayer ( layer->getId(), layer );
        } else {
            LOGGER_ERROR ( _ ( "Ne peut charger le layer: " ) << layerFiles[i] );
        }
    }

    if ( serverXML->getNbLayers() ==0 ) {
        LOGGER_ERROR ( _ ( "Aucun layer n'a pu etre charge!" ) );
        //return false;
    }

    LOGGER_INFO ( _ ( "NOMBRE DE LAYERS CHARGES : " ) << serverXML->getNbLayers() );
    return true;
}

Layer * ConfLoader::buildLayer ( std::string fileName, ServerXML* serverXML, ServicesXML* servicesXML ) {

    LayerXML layerXML(fileName, serverXML, servicesXML );
    if ( ! layerXML.isOk() ) {
        return NULL;
    }

    Layer* pLay =  new Layer(layerXML);

    // Si une pyramide contient un niveau à la demande ou à la volée, on n'authorize pas le WMS 
    // car c'est un cas non géré dans les processus de reponse du serveur
    if (pLay->getDataPyramid()->getContainOdLevels()) {
        pLay->setWMSAuthorized(false);
    }

    return pLay;
}

/**********************************************************************************************************/
/******************************************** PYRAMIDS ****************************************************/
/**********************************************************************************************************/

Pyramid* ConfLoader::buildPyramid ( std::string fileName, ServerXML* serverXML, ServicesXML* servicesXML, bool times ) {
    
    PyramidXML pyrXML(fileName, serverXML, servicesXML, times);

    if ( ! pyrXML.isOk() ) {
        return NULL;
    }

    return new Pyramid(&pyrXML);
}

/**********************************************************************************************************/
/*********************************************** SOURCES **************************************************/
/**********************************************************************************************************/

Pyramid* ConfLoader::buildBasedPyramid (
    TiXmlElement* pElemBP,
    ServerXML* serverXML, ServicesXML* servicesXML,
    std::string levelOD,
    TileMatrixSet* tmsOD,
    std::string parentDir
) {

    TiXmlElement* sFile = pElemBP->FirstChildElement("file");
    TiXmlElement* sTransparent = pElemBP->FirstChildElement("transparent");
    TiXmlElement* sStyle = pElemBP->FirstChildElement("style");

    bool transparent = false;
    std::string str_transparent, basedPyramidFilePath;
    std::string str_style = "";

    if (! sFile || ! sTransparent || ! sStyle || ! sFile->GetText() || ! sTransparent->GetText() || ! sStyle->GetText()) {
        // Il manque un des trois elements necessaires pour initialiser une
        // nouvelle pyramide de base
        LOGGER_ERROR ( _ ( "Source Pyramid: " ) << basedPyramidFilePath << _ ( " can't be loaded because information are missing" ) );
        return NULL;
    }

    str_transparent = DocumentXML::getTextStrFromElem(sTransparent);
    str_style = DocumentXML::getTextStrFromElem(sStyle);

    basedPyramidFilePath = DocumentXML::getTextStrFromElem(sFile) ;
    //Relative Path
    if ( basedPyramidFilePath.compare ( 0,2,"./" ) == 0 ) {
        basedPyramidFilePath.replace ( 0,1,parentDir );
    } else if ( basedPyramidFilePath.compare ( 0,1,"/" ) != 0 ) {
        basedPyramidFilePath.insert ( 0,"/" );
        basedPyramidFilePath.insert ( 0,parentDir );
    }

    // On commence par charger toute la pyramide

    Pyramid* basedPyramid = buildPyramid ( basedPyramidFilePath, serverXML, servicesXML, false );

    if ( ! basedPyramid) {
        LOGGER_ERROR ( _ ( "La pyramide source " ) << basedPyramidFilePath << _ ( " ne peut etre chargee" ) );
        return NULL;
    }

    // On met à jour la transparence
    if (str_transparent == "true") {
        transparent = true;
    }

    basedPyramid->setTransparent(transparent);

    // On teste le style et on ajoute le normal au pire
    Style* style = serverXML->getStyle(str_style);
    if ( style == NULL ) {
        LOGGER_ERROR ( _ ( "Style " ) << str_style << _ ( "non defini" ) );
        style = serverXML->getStyle("normal");
    }
    basedPyramid->setStyle(style);

    /* Calcul du meilleur niveau */

    std::map<std::string, double> levelsRatios = tmsOD->getCorrespondingLevels(levelOD, basedPyramid->getTms());

    if (levelsRatios.size() == 0) {
        // Aucun niveau dans la pyramide de base ne convient
        LOGGER_ERROR ( "La pyramide source " << basedPyramidFilePath << " ne contient pas de niveau satisfaisant pour le niveau " << levelOD );
        delete basedPyramid;
        return NULL;
    }

    Level* bestLevel = NULL;

    std::map<std::string, double>::iterator itRatio = levelsRatios.begin();
    for ( itRatio; itRatio != levelsRatios.end(); itRatio++ ) {
        if (itRatio->second < 0.8 || itRatio->second > 1.8) continue;

        bestLevel = basedPyramid->getLevel(itRatio->first);
        if (bestLevel != NULL) {
            // On a le niveau satisfaisant
            break;
        }
    }

    if (bestLevel == NULL) {
        LOGGER_ERROR ( "La pyramide source " << basedPyramidFilePath << " ne contient pas de niveau satisfaisant pour le niveau " << levelOD );
        delete basedPyramid;
        return NULL;
    }

    // On va pouvoir ne garder que le niveau utile dans la pyramide de base
    std::map<std::string, Level*>::iterator itLev = basedPyramid->getLevels().begin();
    for ( itLev; itLev != basedPyramid->getLevels().end(); itLev++ ) {
        if (itLev->first != bestLevel->getId()) basedPyramid->removeLevel(itLev->first);
    }
    basedPyramid->setUniqueLevel(bestLevel->getId());

    return basedPyramid;
}

WebService *ConfLoader::parseWebService(TiXmlElement* sWeb, CRS pyrCRS, Rok4Format::eformat_data pyrFormat, Proxy proxy_default, ServicesXML* servicesXML) {

    WebService * ws = NULL;
    std::string url, user, proxy, noProxy,pwd, referer, userAgent, version, layers, styles, format, crs;
    std::map<std::string,std::string> options;
    int timeout, retry, interval, channels;
    std::string name,ndValuesStr,value;
    BoundingBox<double> bbox = BoundingBox<double> (0.,0.,0.,0.);
    std::vector<int> noDataValues;

    TiXmlElement* sUrl = sWeb->FirstChildElement("url");
    if (sUrl && sUrl->GetText()) {
        url = DocumentXML::getTextStrFromElem(sUrl);

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
        proxy = DocumentXML::getTextStrFromElem(sProxy);
    } else {
        proxy = proxy_default.proxyName;
    }

    sProxy = sWeb->FirstChildElement("noProxy");
    if (sProxy && sProxy->GetText()) {
        noProxy = DocumentXML::getTextStrFromElem(sProxy);
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
        user = DocumentXML::getTextStrFromElem(sUser);
    } else {
        user = "";
    }

    TiXmlElement* sPwd = sWeb->FirstChildElement("password");
    if (sPwd && sPwd->GetText()) {
        pwd = DocumentXML::getTextStrFromElem(sPwd);
    } else {
        pwd = "";
    }

    TiXmlElement* sReferer = sWeb->FirstChildElement("referer");
    if (sReferer && sReferer->GetText()) {
        referer = DocumentXML::getTextStrFromElem(sReferer);
    } else {
        referer = "";
    }

    TiXmlElement* sUserAgent = sWeb->FirstChildElement("userAgent");
    if (sUserAgent && sUserAgent->GetText()) {
        userAgent = DocumentXML::getTextStrFromElem(sUserAgent);
    } else {
        userAgent = "";
    }


    TiXmlElement* sWMS = sWeb->FirstChildElement("wms");
    if (sWMS) {

        TiXmlElement* sVersion = sWMS->FirstChildElement("version");
        if (sVersion && sVersion->GetText()) {
            version = DocumentXML::getTextStrFromElem(sVersion);
        } else {
            LOGGER_ERROR("Un WMS doit contenir une version");
            return NULL;
        }

        TiXmlElement* sLayers= sWMS->FirstChildElement("layers");
        if (sLayers && sLayers->GetText()) {
            layers = DocumentXML::getTextStrFromElem(sLayers);
        } else {
            LOGGER_ERROR("Un WMS doit contenir un ou des layers séparés par des virgules");
            return NULL;
        }

        TiXmlElement* sStyles = sWMS->FirstChildElement("styles");
        if (sStyles && sStyles->GetText()) {
            styles = DocumentXML::getTextStrFromElem(sStyles);
        } else {
            LOGGER_ERROR("Un WMS doit contenir un ou des styles séparés par des virgules");
            return NULL;
        }

        TiXmlElement* sFormat = sWMS->FirstChildElement("format");
        if (sFormat && sFormat->GetText()) {
            format = DocumentXML::getTextStrFromElem(sFormat);
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
            crs = DocumentXML::getTextStrFromElem(sCrs);

                //le crs demandé et le crs de la pyramide en construction doivent être le même
            CRS askedCRS = CRS(crs);
            std::string pcrs = pyrCRS.getProj4Code();

            if (askedCRS != pyrCRS && !serviceConf->are_the_two_CRS_equal(crs,pcrs)) {
                LOGGER_ERROR("Un WMS doit contenir un crs équivalent à celui de la pyramide en construction");
                return NULL;
            }

        } else {
            crs = pyrCRS.getProj4Code();
            LOGGER_ERROR("Un WMS doit contenir un crs. Par défaut => " << crs);
        }

        TiXmlElement* sChannels = sWMS->FirstChildElement("channels");
        if (sChannels && sChannels->GetText()) {
            channels = atoi(DocumentXML::getTextStrFromElem(sChannels).c_str());
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
            ndValuesStr = DocumentXML::getTextStrFromElem(pND);

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
        ws->setResponseType(format);
    } else {
         //On retourne une erreur car le WMS est le seul WebService disponible pour le moment
        LOGGER_ERROR("Un WebService doit contenir un WMS pour être utilisé");
        return NULL;
    }

    return ws;

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



time_t ConfLoader::getLastModifiedDate(std::string file) {
    struct stat sb;

    int result = stat(file.c_str(), &sb);
    if (result == 0) {
        return *(&sb.st_mtim.tv_sec);
    } else {
        return 0;
    }



}


std::string ConfLoader::getTagContentOfFile(std::string file, std::string tag) {

    std::string content;

    TiXmlDocument doc ( file );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR (  "Ne peut pas charger le fichier " << file );
        return "";
    }

    TiXmlHandle hDoc ( &doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( file << " impossible de recuperer la racine."  );
        return "";
    }
    if ( pElem->ValueStr() == tag ) {
        if (pElem->GetText()) {
            content = pElem->GetTextStr();
        }
    }

    hRoot=TiXmlHandle ( pElem );

    pElem=hRoot.FirstChild ( tag ).Element();
    if ( pElem && pElem->GetText() ) {
        content = pElem->GetTextStr();
    }

    return content;
}

std::vector<std::string> ConfLoader::listFileFromDir(std::string directory, std::string extension) {

    // lister les fichier du repertoire layerDir
    std::vector<std::string> files;
    std::string fileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( directory.c_str() ) ) == NULL ) {
        LOGGER_FATAL ( "Le repertoire "  << directory <<  " n'est pas accessible."  );
        return files;
    }
    while ( ( fileEntry = readdir ( dir ) ) ) {
        fileName = fileEntry->d_name;
        if ( fileName.rfind ( extension ) ==fileName.size()-extension.size() ) {
            files.push_back ( directory+"/"+fileName );
        }
    }
    closedir ( dir );

    if ( files.empty() ) {
        LOGGER_ERROR ( "Aucun fichier " << extension << " dans le repertoire "  << directory );
        return files;
    }

    return files;

}

bool ConfLoader::doesFileExist(std::string file) {
    struct stat buffer;

    if (stat (file.c_str(), &buffer) == 0) {
        return true;
    } else {
        return false;
    }
}

std::string ConfLoader::getFileName(std::string file, std::string extension) {

    std::string id;

    unsigned int idBegin=file.rfind ( "/" );
    if ( idBegin == std::string::npos ) {
        idBegin=0;
    }
    unsigned int idEnd=file.rfind ( extension );
    if ( idEnd == std::string::npos ) {
        idEnd=file.size();
    }
    id=file.substr ( idBegin+1, idEnd-idBegin-1 );

    return id;

}
