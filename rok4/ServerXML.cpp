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

#include "ServerXML.h"

ServerXML::ServerXML(std::string path ) : DocumentXML(path) {
    ok = false;

    std::cout<<_ ( "Chargement des parametres techniques depuis " ) <<filePath<<std::endl;

    TiXmlDocument doc ( filePath );
    if ( ! doc.LoadFile() ) {
        std::cerr<<_ ( "Ne peut pas charger le fichier " ) << filePath << std::endl;
        return;
    }

    TiXmlHandle hDoc ( &doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        std::cerr<<filePath <<_ ( " impossible de recuperer la racine." ) <<std::endl;
        return;
    }

    if ( strcmp ( pElem->Value(), "serverConf" ) ) {
        std::cerr<<filePath <<_ ( " La racine n'est pas un serverConf." ) <<std::endl;
        return;
    }
    hRoot=TiXmlHandle ( pElem );

    pElem=hRoot.FirstChild ( "logOutput" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de logOutput => logOutput = " ) << DEFAULT_LOG_OUTPUT;
        logOutput = DEFAULT_LOG_OUTPUT;
    } else {
        std::string strLogOutput= ( DocumentXML::getTextStrFromElem(pElem) );
        if ( strLogOutput=="rolling_file" ) {
            logOutput=ROLLING_FILE;
        } else if ( strLogOutput=="standard_output_stream_for_errors" ) {
            logOutput=STANDARD_OUTPUT_STREAM_FOR_ERRORS;
        } else if ( strLogOutput=="static_file" ) {
            logOutput=STATIC_FILE;
        } else {
            std::cerr<<_ ( "Le logOutput [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "]  est inconnu." ) <<std::endl;
            return;
        }
    }

    pElem=hRoot.FirstChild ( "logFilePrefix" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de logFilePrefix => logFilePrefix = " ) << DEFAULT_LOG_FILE_PREFIX;
        logFilePrefix = DEFAULT_LOG_FILE_PREFIX;
    } else {
        logFilePrefix= DocumentXML::getTextStrFromElem(pElem);
    }
    pElem=hRoot.FirstChild ( "logFilePeriod" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de logFilePeriod => logFilePeriod = " ) << DEFAULT_LOG_FILE_PERIOD;
        logFilePeriod = DEFAULT_LOG_FILE_PERIOD;
    } else if ( !sscanf ( pElem->GetText(),"%d",&logFilePeriod ) )  {
        std::cerr<<_ ( "Le logFilePeriod [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "]  is not an integer." ) <<std::endl;
        return;
    }

    pElem=hRoot.FirstChild ( "logLevel" ).Element();

    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de logLevel => logLevel = " ) << DEFAULT_LOG_LEVEL;
        logLevel = DEFAULT_LOG_LEVEL;
    } else {
        std::string strLogLevel ( pElem->GetText() );
        if ( strLogLevel=="fatal" ) logLevel=FATAL;
        else if ( strLogLevel=="error" ) logLevel=ERROR;
        else if ( strLogLevel=="warn" ) logLevel=WARN;
        else if ( strLogLevel=="info" ) logLevel=INFO;
        else if ( strLogLevel=="debug" ) logLevel=DEBUG;
        else {
            std::cerr<<_ ( "Le logLevel [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "]  est inconnu." ) <<std::endl;
            return;
        }
    }

    pElem=hRoot.FirstChild ( "nbThread" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de nbThread => nbThread = " ) << DEFAULT_NB_THREAD<<std::endl;
        nbThread = DEFAULT_NB_THREAD;
    } else if ( !sscanf ( pElem->GetText(),"%d",&nbThread ) ) {
        std::cerr<<_ ( "Le nbThread [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "] is not an integer." ) <<std::endl;
        return;
    }

    pElem=hRoot.FirstChild ( "nbProcess" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de nbProcess=> nbProcess = " ) << DEFAULT_NB_PROCESS<<std::endl;
        nbProcess = DEFAULT_NB_PROCESS;
    } else if ( !sscanf ( pElem->GetText(),"%d",&nbProcess ) ) {
        std::cerr<<_ ( "Le nbProcess [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "] is not an integer." ) <<std::endl;
        std::cerr<<_ ( "=> nbProcess = " ) << DEFAULT_NB_PROCESS<<std::endl;
        nbProcess = DEFAULT_NB_PROCESS;
    }
    if (nbProcess > MAX_NB_PROCESS) {
        std::cerr<<_ ( "Le nbProcess [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "] is bigger than " ) << MAX_NB_PROCESS <<std::endl;
        std::cerr<<_ ( "=> nbProcess = " ) << MAX_NB_PROCESS<<std::endl;
        nbProcess = MAX_NB_PROCESS;
    }

    pElem=hRoot.FirstChild ( "timeForProcess" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        timeKill = DEFAULT_TIME_PROCESS;
    } else if ( !sscanf ( pElem->GetText(),"%d",&timeKill ) ) {
        timeKill = DEFAULT_TIME_PROCESS;
    }
    if (timeKill > DEFAULT_MAX_TIME_PROCESS) {
        timeKill = DEFAULT_MAX_TIME_PROCESS;
    }

    pElem=hRoot.FirstChild ( "WMTSSupport" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de WMTSSupport => supportWMTS = true" ) <<std::endl;
        supportWMTS = true;
    } else {
        std::string strReprojection ( pElem->GetText() );
        if ( strReprojection=="true" ) supportWMTS=true;
        else if ( strReprojection=="false" ) supportWMTS=false;
        else {
            std::cerr<<_ ( "Le WMTSSupport [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "] n'est pas un booleen." ) <<std::endl;
            return;
        }
    }

    pElem=hRoot.FirstChild ( "TMSSupport" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de TMSSupport => supportTMS = true" ) <<std::endl;
        supportTMS = true;
    } else {
        std::string strReprojection ( pElem->GetText() );
        if ( strReprojection=="true" ) supportTMS=true;
        else if ( strReprojection=="false" ) supportTMS=false;
        else {
            std::cerr<<_ ( "Le TMSSupport [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "] n'est pas un booleen." ) <<std::endl;
            return;
        }
    }

    pElem=hRoot.FirstChild ( "WMSSupport" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de WMSSupport => supportWMS = true" ) <<std::endl;
        supportWMS = true;
    } else {
        std::string strReprojection ( pElem->GetText() );
        if ( strReprojection=="true" ) supportWMS=true;
        else if ( strReprojection=="false" ) supportWMS=false;
        else {
            std::cerr<<_ ( "Le WMSSupport [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "] n'est pas un booleen." ) <<std::endl;
            return;
        }
    }

    pElem=hRoot.FirstChild ( "proxy" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        proxy.proxyName = "";
    } else {
        proxy.proxyName = DocumentXML::getTextStrFromElem(pElem);
    }

    pElem=hRoot.FirstChild ( "noProxy" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        proxy.noProxy = "";
    } else {
        proxy.noProxy = DocumentXML::getTextStrFromElem(pElem);
    }

    if ( !supportWMS && !supportWMTS && !supportTMS ) {
        std::cerr<<_ ( "WMTS, TMS et WMS desactives, extinction du serveur" ) <<std::endl;
        return;
    }

    if ( supportWMS ) {
        pElem=hRoot.FirstChild ( "reprojectionCapability" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            std::cerr<<_ ( "Pas de reprojectionCapability => reprojectionCapability = true" ) <<std::endl;
            reprojectionCapability = true;
        } else {
            std::string strReprojection ( pElem->GetText() );
            if ( strReprojection=="true" ) reprojectionCapability=true;
            else if ( strReprojection=="false" ) reprojectionCapability=false;
            else {
                std::cerr<<_ ( "Le reprojectionCapability [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "] n'est pas un booleen." ) <<std::endl;
                return;
            }
        }
    } else {
        reprojectionCapability = false;
    }

    pElem=hRoot.FirstChild ( "servicesConfigFile" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de servicesConfigFile => servicesConfigFile = " ) << DEFAULT_SERVICES_CONF_PATH <<std::endl;
        servicesConfigFile = DEFAULT_SERVICES_CONF_PATH;
    } else {
        servicesConfigFile= DocumentXML::getTextStrFromElem(pElem);
    }

    pElem=hRoot.FirstChild ( "layerDir" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de layerDir => layerDir = " ) << DEFAULT_LAYER_DIR<<std::endl;
        layerDir = DEFAULT_LAYER_DIR;
    } else {
        layerDir= DocumentXML::getTextStrFromElem(pElem);
    }

    pElem=hRoot.FirstChild ( "tileMatrixSetDir" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de tileMatrixSetDir => tileMatrixSetDir = " ) << DEFAULT_TMS_DIR<<std::endl;
        tmsDir = DEFAULT_TMS_DIR;
    } else {
        tmsDir= DocumentXML::getTextStrFromElem(pElem);
    }

    pElem=hRoot.FirstChild ( "styleDir" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de styleDir => styleDir = " ) << DEFAULT_STYLE_DIR<<std::endl;
        styleDir = DEFAULT_STYLE_DIR;
    } else {
        styleDir = DocumentXML::getTextStrFromElem(pElem);
    }

    // Definition de la variable PROJ_LIB à partir de la configuration
    bool absolut=true;
    pElem=hRoot.FirstChild ( "projConfigDir" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de projConfigDir => projConfigDir = " ) << DEFAULT_PROJ_DIR<<std::endl;
        char* pwdBuff = ( char* ) malloc ( PATH_MAX );
        getcwd ( pwdBuff,PATH_MAX );
        projDir = std::string ( pwdBuff );
        projDir.append ( "/" ).append ( DEFAULT_PROJ_DIR );
        free ( pwdBuff );
    } else {
        projDir= DocumentXML::getTextStrFromElem(pElem);
        //Gestion des chemins relatif
        if ( projDir.compare ( 0,1,"/" ) != 0 ) {
            absolut=false;
            char* pwdBuff = ( char* ) malloc ( PATH_MAX );
            getcwd ( pwdBuff,PATH_MAX );
            std::string pwdBuffStr = std::string ( pwdBuff );
            pwdBuffStr.append ( "/" );
            projDir.insert ( 0,pwdBuffStr );
            free ( pwdBuff );
        }
    }

    if ( setenv ( "PROJ_LIB", projDir.c_str(), 1 ) !=0 ) {
        std::cerr<<_ ( "ERREUR FATALE : Impossible de definir le chemin pour proj " ) << projDir<<std::endl;
        return;
    }
    std::clog << _ ( "Env : PROJ_LIB = " ) << getenv ( "PROJ_LIB" ) << std::endl;


    pElem=hRoot.FirstChild ( "serverPort" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas d'element <serverPort> fonctionnement autonome impossible" ) <<std::endl;
        socket = "";
    } else {
        std::cerr<<_ ( "Element <serverPort> : Lancement interne impossible (Apache, spawn-fcgi)" ) <<std::endl;
        socket = DocumentXML::getTextStrFromElem(pElem);
    }

    pElem=hRoot.FirstChild ( "serverBackLog" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::clog<<_ ( "Pas d'element <serverBackLog> valeur par defaut : 0" ) <<std::endl;
        backlog = 0;
    } else if ( !sscanf ( pElem->GetText(),"%d",&backlog ) )  {
        std::cerr<<_ ( "Le logFilePeriod [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "]  is not an integer." ) <<std::endl;
        backlog = 0;
    }

#if BUILD_OBJECT

    /************************************ PARTIE OBJET ************************************/

    pElem=hRoot.FirstChild ( "reconnectionFrequency" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de reconnectionFrequency => reconnectionFrequency = " ) << DEFAULT_RECONNECTION_FREQUENCY<<std::endl;
        reconnectionFrequency = DEFAULT_RECONNECTION_FREQUENCY;
    } else if ( !sscanf ( pElem->GetText(),"%d",&reconnectionFrequency ) ) {
        std::cerr<<_ ( "Le reconnectionFrequency [" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "] is not an integer." ) <<std::endl;
        return;
    }

    pElem = hRoot.FirstChild ( "cephContext" ).Element();
    if ( pElem) {

        TiXmlElement* pElemCephContext;

        pElemCephContext = hRoot.FirstChild ( "cephContext" ).FirstChild ( "clusterName" ).Element();
        if ( !pElemCephContext  || ! ( pElemCephContext->GetText() ) ) {
            char* cluster = getenv ("ROK4_CEPH_CLUSTERNAME");
            if (cluster == NULL) {
                std::cerr<< ("L'utilisation d'un cephContext necessite de preciser un clusterName" ) <<std::endl;
                return;
            } else {
                cephName.assign(cluster);
            }
        } else {
            cephName = pElemCephContext->GetText();
        }

        pElemCephContext = hRoot.FirstChild ( "cephContext" ).FirstChild ( "userName" ).Element();
        if ( !pElemCephContext  || ! ( pElemCephContext->GetText() ) ) {
            char* user = getenv ("ROK4_CEPH_USERNAME");
            if (user == NULL) {
                std::cerr<< ("L'utilisation d'un cephContext necessite de preciser un userName" ) <<std::endl;
                return;
            } else {
                cephUser.assign(user);
            }
        } else {
            cephUser = pElemCephContext->GetText();
        }

        pElemCephContext = hRoot.FirstChild ( "cephContext" ).FirstChild ( "confFile" ).Element();
        if ( !pElemCephContext  || ! ( pElemCephContext->GetText() ) ) {
            char* conf = getenv ("ROK4_CEPH_CONFFILE");
            if (conf == NULL) {
                std::cerr<< ("L'utilisation d'un cephContext necessite de preciser un confFile" ) <<std::endl;
                return;
            } else {
                cephConf.assign(conf);
            }
        } else {
            cephConf = pElemCephContext->GetText();
        }

        cephBook = new ContextBook(CEPHCONTEXT, cephName, cephUser, cephConf);
    } else {
        cephBook = NULL;
    }

    pElem = hRoot.FirstChild ( "s3Context" ).Element();
    if ( pElem) {

        TiXmlElement* pElemS3Context;

        pElemS3Context = hRoot.FirstChild ( "s3Context" ).FirstChild ( "url" ).Element();
        if ( !pElemS3Context  || ! ( pElemS3Context->GetText() ) ) {
            char* url = getenv ("ROK4_S3_URL");
            if (url == NULL) {
                std::cerr<< ("L'utilisation d'un cephContext necessite de preciser une url" ) <<std::endl;
                return;
            } else {
                s3URL.assign(url);
            }
        } else {
            s3URL = pElemS3Context->GetText();
        }

        pElemS3Context = hRoot.FirstChild ( "s3Context" ).FirstChild ( "key" ).Element();
        if ( !pElemS3Context  || ! ( pElemS3Context->GetText() ) ) {
            char* k = getenv ("ROK4_S3_KEY");
            if (k == NULL) {
                LOGGER_ERROR ("L'utilisation d'un cephContext necessite de preciser une key" ) <<std::endl;
                return;
            } else {
                s3AccessKey.assign(k);
            }
        } else {
            s3AccessKey = pElemS3Context->GetText();
        }

        pElemS3Context = hRoot.FirstChild ( "s3Context" ).FirstChild ( "secretKey" ).Element();
        if ( !pElemS3Context  || ! ( pElemS3Context->GetText() ) ) {
            char* sk = getenv ("ROK4_S3_SECRETKEY");
            if (sk == NULL) {
                std::cerr<< ("L'utilisation d'un cephContext necessite de preciser une secretKey" ) <<std::endl;
                return;
            } else {
                s3SecretKey.assign(sk);
            }
        } else {
            s3SecretKey = pElemS3Context->GetText();
        }

        s3Book = new ContextBook(S3CONTEXT, s3URL,s3AccessKey,s3SecretKey);
    } else {
        s3Book = NULL;
    }

    pElem = hRoot.FirstChild ( "swiftContext" ).Element();
    if ( pElem ) {

        TiXmlElement* pElemSwiftContext;

        pElemSwiftContext = hRoot.FirstChild ( "swiftContext" ).FirstChild ( "authUrl" ).Element();
        if ( !pElemSwiftContext  || ! ( pElemSwiftContext->GetText() ) ) {
            char* auth = getenv ("ROK4_SWIFT_AUTHURL");
            if (auth == NULL) {
                std::cerr<< ("L'utilisation d'un swiftContext necessite de preciser un authUrl" ) <<std::endl;
                return;
            } else {
                swiftAuthUrl.assign(auth);
            }
        } else {
            swiftAuthUrl = pElemSwiftContext->GetText();
        }

        pElemSwiftContext = hRoot.FirstChild ( "swiftContext" ).FirstChild ( "userName" ).Element();
        if ( !pElemSwiftContext  || ! ( pElemSwiftContext->GetText() ) ) {
            char* user = getenv ("ROK4_SWIFT_USER");
            if (user == NULL) {
                std::cerr<< ("L'utilisation d'un swiftContext necessite de preciser un userName" ) <<std::endl;
                return;
            } else {
                swiftUserName.assign(user);
            }
        } else {
            swiftUserName = pElemSwiftContext->GetText();
        }

        pElemSwiftContext = hRoot.FirstChild ( "swiftContext" ).FirstChild ( "userPassword" ).Element();
        if ( !pElemSwiftContext  || ! ( pElemSwiftContext->GetText() ) ) {
            char* passwd = getenv ("ROK4_SWIFT_PASSWD");
            if (passwd == NULL) {
                std::cerr<< ("L'utilisation d'un swiftContext necessite de preciser un userPassword" ) <<std::endl;
                return;
            } else {
                swiftUserPassword.assign(passwd);
            }
        } else {
            swiftUserPassword = pElemSwiftContext->GetText();
        }

        swiftBook = new ContextBook(SWIFTCONTEXT, swiftAuthUrl, swiftUserName, swiftUserPassword);
    } else {
        swiftBook = NULL;
    }

#endif

    ok = true;
}

/*********************** DESTRUCTOR ********************/

ServerXML::~ServerXML(){ 

    // Les TMS
    std::map<std::string, TileMatrixSet*>::iterator itTMS;
    for ( itTMS=tmsList.begin(); itTMS!=tmsList.end(); itTMS++ )
        delete itTMS->second;

    // Les styles
    std::map<std::string, Style*>::iterator itSty;
    for ( itSty=stylesList.begin(); itSty!=stylesList.end(); itSty++ )
        delete itSty->second;

    // Les couches
    std::map<std::string, Layer*>::iterator itLay;
    for ( itLay=layersList.begin(); itLay!=layersList.end(); itLay++ )
        delete itLay->second;

#if BUILD_OBJECT

    if (cephBook != NULL) {
        delete cephBook;
    }
    if (s3Book != NULL) {
        delete s3Book;
    }
    if (swiftBook != NULL) {
        delete swiftBook;
    }
#endif
}

/******************* GETTERS / SETTERS *****************/

bool ServerXML::isOk() { return ok; }

LogOutput ServerXML::getLogOutput() {return logOutput;}
int ServerXML::getLogFilePeriod() {return logFilePeriod;}
std::string ServerXML::getLogFilePrefix() {return logFilePrefix;}
LogLevel ServerXML::getLogLevel() {return logLevel;}

std::string ServerXML::getServicesConfigFile() {return servicesConfigFile;}

std::string ServerXML::getTmsDir() {return tmsDir;}
std::map<std::string, TileMatrixSet*> ServerXML::getTmsList() {return tmsList;}
void ServerXML::addTMS(TileMatrixSet* t) {
    tmsList.insert ( std::pair<std::string, TileMatrixSet *> ( t->getId(), t ) );
}
int ServerXML::getNbTMS() {
    return tmsList.size();
}
TileMatrixSet* ServerXML::getTMS(std::string id) {
    std::map<std::string, TileMatrixSet*>::iterator tmsIt= tmsList.find ( id );
    if ( tmsIt == tmsList.end() ) {
        return NULL;
    }
    return tmsIt->second;
}
void ServerXML::removeTMS(std::string id) {
    std::map<std::string, TileMatrixSet*>::iterator itTms= tmsList.find ( id );
    if ( itTms != tmsList.end() ) {
        delete itTms->second;
        tmsList.erase(itTms);
    }
}
void ServerXML::cleanTMSs(std::vector<std::string> ids) {
    std::map<std::string, TileMatrixSet*>::iterator itTms;
    for ( itTms = tmsList.begin(); itTms != tmsList.end(); itTms++ ) {

        if (find (ids.begin(), ids.end(), itTms->first) == ids.end()) {
            LOGGER_DEBUG ( _ ( "TMS supprimé : " ) << itTms->first );
            // Le TMS n'est pas présent dans la liste d'ids fournie, on le supprime
            delete itTms->second;
            tmsList.erase(itTms);
        }
    }
}


std::string ServerXML::getStylesDir() {return styleDir;}
std::map<std::string, Style*> ServerXML::getStylesList() {return stylesList;}
void ServerXML::addStyle(Style* s) {
    stylesList.insert ( std::pair<std::string, Style *> ( s->getId(), s ) );
}
int ServerXML::getNbStyles() {
    return stylesList.size();
}
Style* ServerXML::getStyle(std::string id) {
    std::map<std::string, Style*>::iterator styleIt= stylesList.find ( id );
    if ( styleIt == stylesList.end() ) {
        return NULL;
    }
    return styleIt->second;
}
void ServerXML::removeStyle(std::string id) {
    std::map<std::string, Style*>::iterator styleIt= stylesList.find ( id );
    if ( styleIt != stylesList.end() ) {
        delete styleIt->second;
        stylesList.erase(styleIt);
    }
}
void ServerXML::cleanStyles(std::vector<std::string> ids) {
    std::map<std::string, Style*>::iterator itSty;
    for ( itSty = stylesList.begin(); itSty != stylesList.end(); itSty++ ) {

        if (find (ids.begin(), ids.end(), itSty->first) == ids.end()) {
            LOGGER_DEBUG ( _ ( "Style supprimé : " ) << itSty->first );
            // Le style n'est pas présent dans la liste d'ids fournie, on le supprime
            delete itSty->second;
            stylesList.erase(itSty);
        }
    }
}

std::string ServerXML::getLayersDir() {return layerDir;}
void ServerXML::addLayer(Layer* l) {
    layersList.insert ( std::pair<std::string, Layer *> ( l->getId(), l ) );
}
int ServerXML::getNbLayers() {
    return layersList.size();
}
Layer* ServerXML::getLayer(std::string id) {
    std::map<std::string, Layer*>::iterator itLay = layersList.find ( id );
    if ( itLay == layersList.end() ) {
        return NULL;
    }
    return itLay->second;
}
void ServerXML::removeLayer(std::string id) {
    std::map<std::string, Layer*>::iterator itLay = layersList.find ( id );
    if ( itLay != layersList.end() ) {
        delete itLay->second;
        layersList.erase(itLay);
    }
}
void ServerXML::cleanLayers(std::vector<std::string> ids) {
    std::map<std::string, Layer*>::iterator itLay;
    for ( itLay = layersList.begin(); itLay != layersList.end(); itLay++ ) {

        if (find (ids.begin(), ids.end(), itLay->first) == ids.end()) {
            LOGGER_DEBUG ( _ ( "Layer supprimé : " ) << itLay->first );
            // Le layer n'est pas présent dans la liste d'ids fournie, on le supprime
            delete itLay->second;
            layersList.erase(itLay);
        }
    }
}

#if BUILD_OBJECT
ContextBook* ServerXML::getCephContextBook(){return cephBook;}
ContextBook* ServerXML::getSwiftContextBook(){return swiftBook;}
ContextBook* ServerXML::getS3ContextBook(){return s3Book;}

int ServerXML::getReconnectionFrequency() {return reconnectionFrequency;}
#endif

int ServerXML::getNbThreads() {return nbThread;}
std::string ServerXML::getSocket() {return socket;}
bool ServerXML::getSupportWMTS() {return supportWMTS;}
bool ServerXML::getSupportTMS() {return supportTMS;}
bool ServerXML::getSupportWMS() {return supportWMS;}
int ServerXML::getBacklog() {return backlog;}
Proxy ServerXML::getProxy() {return proxy;}
int ServerXML::getTimeKill() {return timeKill;}
bool ServerXML::getReprojectionCapability() { return reprojectionCapability; }
