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

Rok4Server* rok4ReloadServer (const char* serverConfigFile, Rok4Server* oldServer, time_t lastReload ) {

    std::string strServerConfigFile = serverConfigFile;

    LOGGER_DEBUG("Rechargement de la conf");
    //--- server.conf
    LOGGER_DEBUG("Rechargement du server.conf");

    time_t lastModServerConf = ConfLoader::getLastModifiedDate(strServerConfigFile);

    ServerXML* newServerXML = ConfLoader::buildServerConf(strServerConfigFile);
    if ( ! newServerXML->isOk() ) {
        std::cerr<<_ ( "ERREUR FATALE : Impossible d'interpreter le fichier de configuration du serveur " ) <<strServerConfigFile<<std::endl;
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
    // Construction des parametres de service
    ServicesXML* newServicesXML = ConfLoader::buildServicesConf ( newServerXML->getServicesConfigFile() );
    if ( ! newServicesXML->isOk() ) {
        LOGGER_FATAL ( _ ( "Impossible d'interpreter le fichier de conf " ) << newServerXML->getServicesConfigFile() );
        LOGGER_FATAL ( _ ( "Extinction du serveur ROK4" ) );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }



    time_t lastMod;
    std::vector<std::string> listOfFile;
    std::string fileName;

    //--- TMS
    LOGGER_DEBUG("Rechargement des TMS");

    if (newServerXML->getTmsDir() != oldServer->getServerConf()->getTmsDir()) {
        // Le dossier des TMS a changé
        // on recharge tout comme à l'initialisation
        
        LOGGER_DEBUG("Rechargement complet du nouveau dossier" << newServerXML->getTmsDir());

        if ( ! ConfLoader::buildTMSList ( newServerXML ) ) {
            LOGGER_FATAL ( _ ( "Impossible de charger la conf des TileMatrix" ) );
            LOGGER_FATAL ( _ ( "Extinction du serveur ROK4" ) );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

    } else {

        //Copie de l'ancien serveur
        std::map<std::string,TileMatrixSet* >::iterator lv;

        for (lv = oldServer->getTmsList().begin(); lv != oldServer->getTmsList().end(); lv++) {
            TileMatrixSet* tms = new TileMatrixSet(lv->second);
            newServerXML->addTMS(tms);
        }

        //Lecture du dossier et chargement de ce qui a changé
        listOfFile = ConfLoader::listFileFromDir(newServerXML->getTmsDir(), ".tms");
        std::vector<std::string> listOfFileNames;

        if (listOfFile.size() != 0) {
            for (unsigned i=0; i<listOfFile.size(); i++) {
                lastMod = ConfLoader::getLastModifiedDate(listOfFile[i]);
                fileName = ConfLoader::getFileName(listOfFile[i],".tms");
                listOfFileNames.push_back(fileName);

                if (lastMod > lastReload) {
                    //fichier modifié, on le recharge
                    TileMatrixSet* tms = ConfLoader::buildTileMatrixSet ( listOfFile[i] );

                    // Que le TMS soit correct ou non, on supprime l'ancien de la liste
                    newServerXML->removeTMS(fileName);

                    if (tms != NULL) {
                        newServerXML->addTMS(tms);
                    } else {
                        LOGGER_ERROR("Impossible de charger " << listOfFile[i]);
                    }

                } else {
                    //fichier non modifié
                    if (newServerXML->getTMS(fileName) == NULL) {
                        // mais qui n'était pas là au dernier chargement, ce n'est pas normal
                        LOGGER_WARN("TMS nouveau alors que le fichier était là au dernier rechargement de conf ?!?!");
                    }
                }
            }
        } else {
            //aucun fichier dans le dossier
            LOGGER_FATAL ( "Aucun fichier .tms dans le dossier " << newServerXML->getTmsDir() );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

        // On supprime de la liste des TMS tous ceux dont l'id ne se retrouve pas dans la liste des noms de fichiers
        newServerXML->cleanTMSs(listOfFileNames);
    }

    //--- STYLES
    LOGGER_DEBUG("Rechargement des styles");

    if (newServerXML->getStylesDir() != oldServer->getServerConf()->getStylesDir()) {
        // Le dossier des styles a changé
        // on recharge tout comme à l'initialisation
        
        LOGGER_DEBUG("Rechargement complet du nouveau dossier" << newServerXML->getStylesDir());

        if ( ! ConfLoader::buildStylesList ( newServerXML, newServicesXML ) ) {
            LOGGER_FATAL ( _ ( "Impossible de charger la conf des styles" ) );
            LOGGER_FATAL ( _ ( "Extinction du serveur ROK4" ) );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

    } else {

        //Copie de l'ancien serveur
        std::map<std::string,Style* >::iterator lv;

        for (lv = oldServer->getStylesList().begin(); lv != oldServer->getStylesList().end(); lv++) {
            Style* sty = new Style(lv->second);
            newServerXML->addStyle(sty);
        }

        //Lecture du dossier et chargement de ce qui a changé
        listOfFile = ConfLoader::listFileFromDir(newServerXML->getStylesDir(), ".stl");
        std::vector<std::string> listOfFileNames;

        if (listOfFile.size() != 0) {
            for (unsigned i=0; i<listOfFile.size(); i++) {
                lastMod = ConfLoader::getLastModifiedDate(listOfFile[i]);
                fileName = ConfLoader::getFileName(listOfFile[i],".stl");
                listOfFileNames.push_back(fileName);

                if (lastMod > lastReload) {
                    //fichier modifié, on le recharge
                    Style* sty = ConfLoader::buildStyle ( listOfFile[i], newServicesXML );

                    // Que le style soit correct ou non, on supprime l'ancien de la liste
                    newServerXML->removeStyle(fileName);

                    if (sty != NULL) {
                        newServerXML->addStyle(sty);
                    } else {
                        LOGGER_ERROR("Impossible de charger " << listOfFile[i]);
                    }

                } else {
                    //fichier non modifié
                    if (newServerXML->getStyle(fileName) == NULL) {
                        // mais qui n'était pas là au dernier chargement, ce n'est pas normal
                        LOGGER_WARN("Style nouveau alors que le fichier était là au dernier rechargement de conf ?!?!");
                    }
                }
            }
        } else {
            //aucun fichier dans le dossier
            LOGGER_FATAL ( "Aucun fichier .stl dans le dossier " << newServerXML->getStylesDir() );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

        // On supprime de la liste des styles tous ceux dont l'id ne se retrouve pas dans la liste des noms de fichiers
        newServerXML->cleanStyles(listOfFileNames);
    }


    //--- Layers
    LOGGER_DEBUG("Rechargement des Layers");

    if (newServerXML->getLayersDir() != oldServer->getServerConf()->getLayersDir()) {
        // Le dossier des layers a changé
        // on recharge tout comme à l'initialisation

        LOGGER_DEBUG("Rechargement complet du nouveau dossier" << newServerXML->getLayersDir());

        if ( ! ConfLoader::buildLayersList ( newServerXML, newServicesXML ) ) {
            LOGGER_FATAL ( _ ( "Impossible de charger la conf des Layers" ) );
            LOGGER_FATAL ( _ ( "Extinction du serveur ROK4" ) );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

    } else {

        LOGGER_DEBUG("Copie des anciens layers");
        //Copie de l'ancien serveur
        std::map<std::string,Layer* >::iterator lv;

        for (lv = oldServer->getLayerList().begin(); lv != oldServer->getLayerList().end(); lv++) {
            Layer* lay = new Layer(lv->second, newServerXML->getStylesList(), newServerXML->getTmsList() );
            newServerXML->addLayer(lay);
        }

        LOGGER_DEBUG("Lecture du dossier");

        //Lecture du dossier et chargement de ce qui a changé
        listOfFile = ConfLoader::listFileFromDir(newServerXML->getLayersDir(), ".lay");
        std::vector<std::string> listOfFileNames;

        if (listOfFile.size() != 0) {
            for (unsigned i=0; i<listOfFile.size(); i++) {
                lastMod = ConfLoader::getLastModifiedDate(listOfFile[i]);
                fileName = ConfLoader::getFileName(listOfFile[i],".lay");
                listOfFileNames.push_back(fileName);

                if (lastMod > lastReload) {
                    //fichier modifié, on le recharge
                    Layer* lay = ConfLoader::buildLayer ( listOfFile[i], newServerXML, newServicesXML );

                    // Que le layer soit correct ou non, on supprime l'ancien de la liste
                    newServerXML->removeLayer(fileName);

                    if (lay != NULL) {
                        newServerXML->addLayer(lay);
                    } else {
                        LOGGER_ERROR("Impossible de charger " << listOfFile[i]);
                    }

                } else {
                    //fichier non modifié
                    LOGGER_DEBUG("Fichier layer non modifie");
                    Layer* lay = newServerXML->getLayer(fileName);
                    if (lay == NULL) {
                        // mais qui n'était pas là au dernier chargement, ce n'est pas normal
                        LOGGER_WARN("Layer nouveau alors que le fichier était là au dernier rechargement de conf ?!?!");
                    } else {

                        //on teste si le .pyr a changé
                        std::string strPyrFile = lay->getDataPyramidFilePath();

                        LOGGER_DEBUG("Verification du fichier pyr" << strPyrFile);
                        lastMod = ConfLoader::getLastModifiedDate(strPyrFile);

                        if (lastMod > lastReload) {
                            // fichier modifié
                            Layer* lay = ConfLoader::buildLayer ( listOfFile[i], newServerXML, newServicesXML );

                            newServerXML->removeLayer(fileName);

                            if (lay != NULL) {
                                newServerXML->addLayer(lay);
                            } else {
                                LOGGER_ERROR("Impossible de charger le layer " << listOfFile[i]);
                            }
                        }
                    }
                }

            }
        } else {
            //aucun fichier dans le dossier
            LOGGER_FATAL ( "Aucun fichier .lay dans le dossier " << newServerXML->getLayersDir() );
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

        // On supprime de la liste des layers tous ceux dont l'id ne se retrouve pas dans la liste des noms de fichiers
        newServerXML->cleanLayers(listOfFileNames);
    }

    LOGGER_DEBUG("Arret du logger");
    Logger::stopLogger();
    LOGGER_DEBUG("Logger arrete");

    return new Rok4Server ( newServerXML, newServicesXML );
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