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
* \file Rok4Api.cpp
* \brief Implementation de l'API de ROK4
*/

#include "Rok4Api.h"
#include "config.h"
#include <proj_api.h>
#include "ConfLoader.h"
#include "Message.h"
#include "Request.h"
#include "RawImage.h"
#include "TiffEncoder.h"
#include "TiffHeaderDataSource.h"
#include "Palette.h"
#include "Style.h"
#include <cstdlib>
#include "PNGEncoder.h"
#include "Decoder.h"
#include "Pyramid.h"
#include "TileMatrixSet.h"
#include "TileMatrix.h"
#include "intl.h"
#include "Context.h"
#include "ContextBook.h"
#include <cfloat>
#include <libintl.h>
#include "ServerXML.h"
#include "ServicesXML.h"

static bool loggerInitialised = false;

/**
* \brief Initialisation du serveur ROK4
* \param serverConfigFile : nom du fichier de configuration des parametres techniques
* \return : pointeur sur le serveur ROK4, NULL en cas d'erreur (forcement fatale)
*/

Rok4Server* rok4InitServer ( const char* serverConfigFile ) {

    std::string strServerConfigFile = serverConfigFile;

    ServerXML* serverXML = ConfLoader::buildServerConf(strServerConfigFile);
    if ( ! serverXML->isOk() ) {
        std::cerr<<_ ( "ERREUR FATALE : Impossible d'interpreter le fichier de configuration du serveur " ) <<strServerConfigFile<<std::endl;
        return NULL;
    }

    if ( ! loggerInitialised ) {
        Logger::setOutput ( serverXML->getLogOutput() );

        // Initialisation du logger
        Accumulator *acc=0;
        switch ( serverXML->getLogOutput() ) {
            case ROLLING_FILE :
                acc = new RollingFileAccumulator ( serverXML->getLogFilePrefix(), serverXML->getLogFilePeriod() );
                break;
            case STATIC_FILE :
                acc = new StaticFileAccumulator ( serverXML->getLogFilePrefix() );
                break;
            case STANDARD_OUTPUT_STREAM_FOR_ERRORS :
                acc = new StreamAccumulator();
                break;
        }

        // Attention : la fonction Logger::setAccumulator n'est pas threadsafe
        for ( int i=0; i <= serverXML->getLogLevel(); i++ ) {
            Logger::setAccumulator ( ( LogLevel ) i, acc );
        }

        std::ostream &log = LOGGER ( DEBUG );
        log.precision ( 8 );
        log.setf ( std::ios::fixed,std::ios::floatfield );

        std::cout<< _ ( "Envoi des messages dans la sortie du logger" ) << std::endl;
        LOGGER_INFO ( _ ( "*** DEBUT DU FONCTIONNEMENT DU LOGGER ***" ) );
        loggerInitialised=true;
    } else {
        LOGGER_INFO ( _ ( "*** NOUVEAU CLIENT DU LOGGER ***" ) );
    }

    // Construction des parametres de service
    ServicesXML* servicesXML = ConfLoader::buildServicesConf ( serverXML->getServicesConfigFile() );
    if ( ! servicesXML->isOk() ) {
        LOGGER_FATAL ( _ ( "Impossible d'interpreter le fichier de conf " ) << serverXML->getServicesConfigFile() );
        LOGGER_FATAL ( _ ( "Extinction du serveur ROK4" ) );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    // Chargement des TMS
    if ( ! ConfLoader::buildTMSList ( serverXML ) ) {
        LOGGER_FATAL ( _ ( "Impossible de charger la conf des TileMatrix" ) );
        LOGGER_FATAL ( _ ( "Extinction du serveur ROK4" ) );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    //Chargement des styles
    if ( ! ConfLoader::buildStylesList ( serverXML, servicesXML ) ) {
        LOGGER_FATAL ( _ ( "Impossible de charger la conf des Styles" ) );
        LOGGER_FATAL ( _ ( "Extinction du serveur ROK4" ) );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    // Chargement des layers
    if ( ! ConfLoader::buildLayersList ( serverXML, servicesXML ) ) {
        LOGGER_FATAL ( _ ( "Impossible de charger la conf des Layers/pyramides" ) );
        LOGGER_FATAL ( _ ( "Extinction du serveur ROK4" ) );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    // Instanciation du serveur
    Logger::stopLogger();
    return new Rok4Server ( serverXML, servicesXML );
}

Rok4Server* rok4ReloadServer (const char* serverConfigFile, Rok4Server* server, time_t lastReload ) {

/*
    time_t lastModServerConf,lastMod;
    LogOutput logOutputNew;
    int nbThreadNew,logFilePeriodNew,backlogNew, nbProcessNew,timeKillNew;
    LogLevel logLevelNew;
    bool supportWMTSNew,supportWMSNew,reprojectionCapabilityNew;
    Proxy proxyNew;
    std::string strServerConfigFile=serverConfigFile,strLogFileprefixNew,strServicesConfigFileNew,
            strLayerDirNew,strTmsDirNew,strStyleDirNew,socketNew,fileName;

    std::vector<std::string> listOfFile;
    char* projDir = getenv("PROJ_LIB");
    std::string projDirstr(projDir);

    LOGGER_DEBUG("Rechargement de la conf");
    //--- server.conf
    LOGGER_DEBUG("Rechargement du server.conf");

    lastModServerConf = ConfLoader::getLastModifiedDate(strServerConfigFile);

    //on n'est obligé de le recharger pour avoir des informations qui ne sont pas accessible dans l'objet Rok4Server
    if ( !ConfLoader::getTechnicalParam ( strServerConfigFile, logOutputNew, strLogFileprefixNew, logFilePeriodNew,
                                          logLevelNew, nbThreadNew, supportWMTSNew, supportWMSNew, reprojectionCapabilityNew,
                                          strServicesConfigFileNew, strLayerDirNew, strTmsDirNew, strStyleDirNew, socketNew,
                                          backlogNew, nbProcessNew, proxyNew,timeKillNew ) ) {
        std::cerr << "ERREUR FATALE : Impossible d'interpreter le fichier de configuration du serveur " << strServerConfigFile << std::endl;
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }


    if (lastModServerConf > lastReload) {
        //fichier modifié, on recharge particulièrement le logger et on doit vérifier que les fichiers et dossiers
        //indiqués sont les mêmes qu'avant
        LOGGER_DEBUG("Server.conf modifie");
        // TODO: Reload du logger si nécessaire

    } else {
        //fichier non modifié, il n'y a rien à faire
        LOGGER_DEBUG("Server.conf non modifie");

    }

    //--- service.conf
    LOGGER_DEBUG("Rechargement du service.conf et des fichiers associes (listofequalcrs.txt et restrictedcrslist.txt)");

    delete sc;
    sc = NULL;

    sc = ConfLoader::buildServicesConf ( strServicesConfigFileNew );
    if ( sc==NULL ) {
        LOGGER_FATAL ( "Impossible d'interpreter le fichier de conf " << strServicesConfigFileNew );
        LOGGER_FATAL ( "Extinction du serveur ROK4" );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }


    //--- TMS
    LOGGER_DEBUG("Rechargement des TMS");

    std::map<std::string,TileMatrixSet* > tmsListNew;

    if (strTmsDirNew != server->getTmsDir()) {
        //on recharge tout comme à l'initialisation

        if ( !ConfLoader::buildTMSList ( strTmsDirNew,tmsListNew ) ) {
            LOGGER_FATAL ( "Impossible de charger la conf des TileMatrix" );
            LOGGER_FATAL ( "Extinction du serveur ROK4" );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

    } else {
        //Copie de l'ancien serveur
        std::map<std::string,TileMatrixSet* >::iterator lv;

        for (lv = server->getTmsList().begin();lv != server->getTmsList().end(); lv++) {
            TileMatrixSet* tms = new TileMatrixSet(*lv->second);
            tmsListNew.insert(std::pair<std::string,TileMatrixSet*> (lv->first,tms));
        }

        //Lecture du dossier et chargement de ce qui a changé
        listOfFile = ConfLoader::listFileFromDir(strTmsDirNew, ".tms");

        if (listOfFile.size() != 0) {
            for (unsigned i=0; i<listOfFile.size(); i++) {
                lastMod = ConfLoader::getLastModifiedDate(listOfFile[i]);

                if (lastMod > lastReload) {
                    //fichier modifié, on le recharge
                    TileMatrixSet* tms = ConfLoader::buildTileMatrixSet ( listOfFile[i] );

                    if (tms != NULL) {
                        lv = tmsListNew.find(tms->getId());
                        if (lv != tmsListNew.end()){
                            delete lv->second;
                            tmsListNew.erase(lv);
                            tmsListNew.insert(std::pair<std::string,TileMatrixSet*> (tms->getId(),tms));
                        } else {
                            //nouveau fichier
                            tmsListNew.insert(std::pair<std::string,TileMatrixSet*> (tms->getId(),tms));
                        }
                    } else {
                        LOGGER_ERROR("Impossible de charger " << listOfFile[i]);

                        fileName = ConfLoader::getFileName(listOfFile[i],".stl");
                        lv = tmsListNew.find(fileName);
                        if (lv != tmsListNew.end()){
                            delete lv->second;
                            tmsListNew.erase(lv);
                        } else {
                            //rien à faire
                        }
                    }

                } else {
                    //fichier non modifié

                    fileName = ConfLoader::getFileName(listOfFile[i],".tms");

                    lv = tmsListNew.find(fileName);

                    if (lv == tmsListNew.end()) {
                        //on l'inclue
                        TileMatrixSet* tms = ConfLoader::buildTileMatrixSet ( listOfFile[i] );
                        if (tms != NULL) {
                            tmsListNew.insert(std::pair<std::string,TileMatrixSet*> (tms->getId(),tms));
                        } else {
                            LOGGER_ERROR("Impossible de charger le TileMatrixSet " << listOfFile[i]);
                        }
                    } else {

                    }

                }

            }
        } else {
            //aucun fichier dans le dossier
            LOGGER_FATAL ( "Aucun fichier .tms dans le dossier " << strTmsDirNew );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

        //pour chaque entrée du std::map on regarde s'il y a bien un fichier correspondant
        // si ce n'est pas le cas, on le supprime de la map
        std::vector<std::string> to_delete;

        for (lv = tmsListNew.begin();lv != tmsListNew.end(); lv++) {
            std::string name = strTmsDirNew + "/" + lv->first + ".tms";
            if (!ConfLoader::doesFileExist(name)) {
                //le fichier a été supprimé donc on supprime de la map
                to_delete.push_back(lv->first);
            } else {
                //tout va bien
            }
        }

        for (std::vector<int>::size_type i = 0; i != to_delete.size(); i++) {
            lv = tmsListNew.find(to_delete[i]);
            delete lv->second;
            lv->second = NULL;
            tmsListNew.erase(lv);
        }

    }

    //--- Styles
    LOGGER_DEBUG("Rechargement des Styles");

    std::map<std::string,Style* > styleListNew;

    if (strStyleDirNew != server->getStylesDir()) {
        //on recharge tout comme à l'initialisation

        if ( !ConfLoader::buildStylesList ( strStyleDirNew,styleListNew, sc->isInspire() ) ) {
            LOGGER_FATAL ( "Impossible de charger la conf des Styles" );
            LOGGER_FATAL ( "Extinction du serveur ROK4" );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

    } else {
        //Copie de l'ancien serveur
        std::map<std::string,Style* >::iterator lv;

        for (lv = server->getStyleList().begin();lv != server->getStyleList().end(); lv++) {
            Style* stl = new Style(*lv->second);
            styleListNew.insert(std::pair<std::string,Style*> (lv->first,stl));
        }


        //Lecture du dossier et chargement de ce qui a changé
        listOfFile = ConfLoader::listFileFromDir(strStyleDirNew, ".stl");

        if (listOfFile.size() != 0) {
            for (unsigned i=0; i<listOfFile.size(); i++) {
                lastMod = ConfLoader::getLastModifiedDate(listOfFile[i]);

                if (lastMod > lastReload) {
                    //fichier modifié, on le recharge
                    Style* stl = ConfLoader::buildStyle ( listOfFile[i], sc->isInspire() );

                    if (stl != NULL) {
                        lv = styleListNew.find(stl->getId());
                        if (lv != styleListNew.end()){
                            delete lv->second;
                            styleListNew.erase(lv);
                            styleListNew.insert(std::pair<std::string,Style*> (stl->getId(),stl));
                        } else {
                            //nouveau fichier
                            styleListNew.insert(std::pair<std::string,Style*> (stl->getId(),stl));
                        }
                    } else {
                        LOGGER_ERROR("Impossible de charger " << listOfFile[i]);

                        fileName = ConfLoader::getFileName(listOfFile[i],".stl");
                        lv = styleListNew.find(fileName);
                        if (lv != styleListNew.end()){
                            delete lv->second;
                            styleListNew.erase(lv);
                        } else {
                            //rien à faire
                        }
                    }

                } else {
                    //fichier non modifié

                    fileName = ConfLoader::getFileName(listOfFile[i],".stl");

                    lv = styleListNew.find(fileName);

                    if (lv == styleListNew.end()) {
                        //on l'inclue
                        Style* stl = ConfLoader::buildStyle ( listOfFile[i], sc->isInspire() );
                        if (stl != NULL) {
                            styleListNew.insert(std::pair<std::string,Style*> (stl->getId(),stl));
                        } else {
                            LOGGER_ERROR("Impossible de charger le style " << listOfFile[i]);
                        }
                    } else {

                    }

                }

            }
        } else {
            //aucun fichier dans le dossier
            LOGGER_FATAL ( "Aucun fichier .stl dans le dossier " << strStyleDirNew );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

        //pour chaque entrée du std::map on regarde s'il y a bien un fichier correspondant
        // si ce n'est pas le cas, on le supprime de la map
        std::vector<std::string> to_delete;

        for (lv = styleListNew.begin();lv != styleListNew.end(); lv++) {
            std::string name = strStyleDirNew + "/" + lv->first + ".stl";
            if (!ConfLoader::doesFileExist(name)) {
                //le fichier a été supprimé donc on supprime de la map
                to_delete.push_back(lv->first);
            } else {
                //tout va bien
            }
        }

        for (std::vector<int>::size_type i = 0; i != to_delete.size(); i++) {
            lv = styleListNew.find(to_delete[i]);
            delete lv->second;
            lv->second = NULL;
            styleListNew.erase(lv);
        }

    }




    //--- Layers
    LOGGER_DEBUG("Rechargement des Layers");

    std::map<std::string,Layer* > layerListNew;


    if (strLayerDirNew != server->getLayersDir()) {
        //on recharge tout comme à l'initialisation
        LOGGER_DEBUG("Rechargement complet du nouveau dossier" << strLayerDirNew);

        if ( !ConfLoader::buildLayersList ( strLayerDirNew,tmsListNew, styleListNew,layerListNew,reprojectionCapabilityNew,sc,proxyNew ) ) {
            LOGGER_FATAL ( "Impossible de charger la conf des Layers" );
            LOGGER_FATAL ( "Extinction du serveur ROK4" );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

    } else {

        LOGGER_DEBUG("Copie des anciens layers");
        //Copie de l'ancien serveur
        std::map<std::string,Layer* >::iterator lv;

        for (lv = server->getLayerList().begin();lv != server->getLayerList().end(); lv++) {
            Layer* lay = new Layer(*lv->second,styleListNew,tmsListNew);
            layerListNew.insert(std::pair<std::string,Layer*> (lv->first,lay));
        }

        LOGGER_DEBUG("Lecture du dossier");

        //Lecture du dossier et chargement de ce qui a changé
        listOfFile = ConfLoader::listFileFromDir(strLayerDirNew, ".lay");

        if (listOfFile.size() != 0) {
            for (unsigned i=0; i<listOfFile.size(); i++) {
                LOGGER_DEBUG("Rechargement du layer " << listOfFile[i]);

                lastMod = ConfLoader::getLastModifiedDate(listOfFile[i]);

                if (lastMod > lastReload) {
                    //fichier modifié, on le recharge
                    LOGGER_DEBUG("Fichier layer modifie");
                    Layer* lay = ConfLoader::buildLayer ( listOfFile[i], tmsListNew, styleListNew, reprojectionCapabilityNew, sc, proxyNew );

                    if (lay != NULL) {
                        lv = layerListNew.find(lay->getId());
                        if (lv != layerListNew.end()){
                            delete lv->second;
                            layerListNew.erase(lv);
                            layerListNew.insert(std::pair<std::string,Layer*> (lay->getId(),lay));
                        } else {
                            //nouveau fichier
                            layerListNew.insert(std::pair<std::string,Layer*> (lay->getId(),lay));
                        }
                    } else {
                        LOGGER_ERROR("Impossible de charger le layer " << listOfFile[i]);

                        fileName = ConfLoader::getFileName(listOfFile[i],".lay");
                        lv = layerListNew.find(fileName);
                        if (lv != layerListNew.end()){
                            delete lv->second;
                            layerListNew.erase(lv);
                        } else {
                            //rien à faire
                        }
                    }

                } else {
                    //fichier non modifié
                    LOGGER_DEBUG("Fichier layer non modifie");
                    fileName = ConfLoader::getFileName(listOfFile[i],".lay");

                    lv = layerListNew.find(fileName);

                    if (lv == layerListNew.end()) {
                        LOGGER_DEBUG("Ajout de ce nouveau layer");
                        //on l'inclue
                        Layer* lay = ConfLoader::buildLayer ( listOfFile[i], tmsListNew, styleListNew, reprojectionCapabilityNew, sc, proxyNew );
                        if (lay != NULL) {
                            layerListNew.insert(std::pair<std::string,Layer*> (lay->getId(),lay));
                        } else {
                            LOGGER_ERROR("Impossible de charger le layer " << listOfFile[i]);
                        }
                    } else {

                        //on teste si le .pyr a changé
                        std::string strPyrFile = ConfLoader::getTagContentOfFile(listOfFile[i],"pyramid");

                        if (strPyrFile != "") {
                            LOGGER_DEBUG("Verification du fichier pyr" << strPyrFile);
                            lastMod = ConfLoader::getLastModifiedDate(strPyrFile);

                            if (lastMod > lastReload) {
                                //fichier modifié
                                Layer* lay = ConfLoader::buildLayer ( listOfFile[i], tmsListNew, styleListNew, reprojectionCapabilityNew, sc, proxyNew );

                                if (lay != NULL) {
                                    lv = layerListNew.find(lay->getId());
                                    if (lv != layerListNew.end()){
                                        layerListNew.erase(lv);
                                        layerListNew.insert(std::pair<std::string,Layer*> (lay->getId(),lay));
                                    } else {
                                        //nouveau fichier
                                        layerListNew.insert(std::pair<std::string,Layer*> (lay->getId(),lay));
                                    }
                                } else {
                                    LOGGER_ERROR("Impossible de charger le layer " << listOfFile[i]);
                                }

                            } else {
                                //fichier non modifié, super on ne fait rien
                                LOGGER_DEBUG("Rien a faire");
                            }


                        } else {
                            //impossible de récupérer le nom du fichier .pyr, tant pis...
                            LOGGER_ERROR("Impossible de charger la pyramide " << strPyrFile);
                        }

                    }



                }

            }
        } else {
            //aucun fichier dans le dossier
            LOGGER_FATAL ( "Aucun fichier .lay dans le dossier " << strLayerDirNew );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

        LOGGER_DEBUG("Verification des layers a supprimer");
        //pour chaque entrée du std::map on regarde s'il y a bien un fichier correspondant
        // si ce n'est pas le cas, on le supprime de la map
        std::vector<std::string> to_delete;

        for (lv = layerListNew.begin();lv != layerListNew.end(); lv++) {
            std::string name = strLayerDirNew + "/" + lv->first + ".lay";
            if (!ConfLoader::doesFileExist(name)) {
                //le fichier a été supprimé donc on supprime de la map
                to_delete.push_back(lv->first);
            } else {
                //tout va bien
            }
        }

        LOGGER_DEBUG("Suppression des layers");

        for (std::vector<int>::size_type i = 0; i != to_delete.size(); i++) {
            LOGGER_DEBUG("Suppression du layer " << to_delete[i]);
            lv = layerListNew.find(to_delete[i]);
            delete lv->second;
            lv->second = NULL;
            layerListNew.erase(lv);
        }

    }

    LOGGER_DEBUG("Arret du logger");
    Logger::stopLogger();
    LOGGER_DEBUG("Logger arrete");

    return new Rok4Server ( nbThreadNew, *sc, layerListNew, tmsListNew, styleListNew,
                            socketNew, backlogNew, proxyNew, strTmsDirNew, strStyleDirNew, strLayerDirNew, projDirstr,
                            supportWMTSNew, supportWMSNew, nbProcessNew,timeKillNew );

*/
    return server;
}

/**
* \brief Extinction du serveur
*/

void rok4KillServer ( Rok4Server* server ) {
    LOGGER_INFO ( _ ( "Extinction du serveur ROK4" ) );

    //Clear proj4 cache
    pj_clear_initcache();

    delete server;
}

/**
 * \brief Extinction du Logger
 */
void rok4KillLogger() {
    loggerInitialised = false;
    Accumulator* acc = NULL;
    for ( int i=0; i<= nbLogLevel ; i++ )
        if ( Logger::getAccumulator ( ( LogLevel ) i ) ) {
            acc = Logger::getAccumulator ( ( LogLevel ) i );
            break;
        }
    Logger::stopLogger();
    if ( acc ) {
        acc->stop();
        acc->destroy();
        delete acc;
    }

}

/**
 * \brief Fermeture des descripteurs de fichiers
 */
void rok4ReloadLogger() {
    Accumulator* acc = NULL;
    for ( int i=0; i<= nbLogLevel ; i++ )
        if ( Logger::getAccumulator ( ( LogLevel ) i ) ) {
            acc = Logger::getAccumulator ( ( LogLevel ) i );
            break;
        }
    if ( acc ) {
        acc->close();
    }
}


#if BUILD_OBJECT

/**
* \brief Connexion aux contexts contenus dans les contextBook du serveur
*/

int rok4ConnectObjectContext(Rok4Server* server) {

    int error = 1;

    ContextBook *book = server->getCephBook();

    if (book) {
        if (!book->connectAllContext()) {
            return error;
        }
    }

    book = server->getS3Book();

    if (book) {

        if (!book->connectAllContext()) {
            return error;
        }

    }

    book = server->getSwiftBook();

    if (book != NULL) {
        if (!book->connectAllContext()) {
            return error;
        }
    }

    return 0;

}

/**
* \brief Deconnexion aux contexts contenus dans les contextBook du serveur
*/

void rok4DisconnectObjectContext(Rok4Server* server) {

    ContextBook *book = server->getCephBook();

    if (book) {
        book->disconnectAllContext();
    }

    book = server->getSwiftBook();

    if (book) {
        book->disconnectAllContext();
    }

    book = server->getS3Book();

    if (book) {
        book->disconnectAllContext();
    }
}

#endif