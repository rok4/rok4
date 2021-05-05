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
#include "Context.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

#if BUILD_OBJECT
#include "ContextBook.h"
#endif

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
        std::cerr<< "ERREUR FATALE : Impossible d'interpreter le fichier de configuration du serveur " <<strServerConfigFile<<std::endl;
        return NULL;
    }

    if ( ! loggerInitialised ) {
        /* Initialisation des Loggers */
        boost::log::core::get()->set_filter( boost::log::trivial::severity >= serverXML->getLogLevel() );
        logging::add_common_attributes();
        boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");

        if ( serverXML->getLogOutput() == "rolling_file") {
            logging::add_file_log (
                keywords::file_name = serverXML->getLogFilePrefix()+"-%Y-%m-%d-%H-%M-%S.log",
                keywords::time_based_rotation = sinks::file::rotation_at_time_interval(boost::posix_time::seconds(serverXML->getLogFilePeriod())),
                keywords::format = "%TimeStamp%\t%ProcessID%\t%Severity%\t%Message%",
                keywords::auto_flush = true
            );
        } else if ( serverXML->getLogOutput() == "static_file") {
            logging::add_file_log (
                keywords::file_name = serverXML->getLogFilePrefix(),
                keywords::format = "%TimeStamp%\t%ProcessID%\t%Severity%\t%Message%",
                keywords::auto_flush = true
            );
        } else if ( serverXML->getLogOutput() == "standard_output_stream_for_errors") {
            logging::add_console_log (
                std::cout,
                keywords::format = "%TimeStamp%\t%ProcessID%\t%Severity%\t%Message%"
            );
        }

        std::cout<<  "Envoi des messages dans la sortie du logger" << std::endl;
        BOOST_LOG_TRIVIAL(info) <<   "*** DEBUT DU FONCTIONNEMENT DU LOGGER ***" ;
        loggerInitialised = true;
    } else {
        BOOST_LOG_TRIVIAL(info) <<   "*** NOUVEAU CLIENT DU LOGGER ***" ;
    }

    // Construction des parametres de service
    ServicesXML* servicesXML = ConfLoader::buildServicesConf ( serverXML->getServicesConfigFile() );
    if ( ! servicesXML->isOk() ) {
        BOOST_LOG_TRIVIAL(fatal) <<   "Impossible d'interpreter le fichier de conf " << serverXML->getServicesConfigFile() ;
        BOOST_LOG_TRIVIAL(fatal) <<   "Extinction du serveur ROK4" ;
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    // Chargement des TMS
    if ( ! ConfLoader::buildTMSList ( serverXML ) ) {
        BOOST_LOG_TRIVIAL(fatal) <<   "Impossible de charger la conf des TileMatrix" ;
        BOOST_LOG_TRIVIAL(fatal) <<   "Extinction du serveur ROK4" ;
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    //Chargement des styles
    if ( ! ConfLoader::buildStylesList ( serverXML, servicesXML ) ) {
        BOOST_LOG_TRIVIAL(fatal) <<   "Impossible de charger la conf des Styles" ;
        BOOST_LOG_TRIVIAL(fatal) <<   "Extinction du serveur ROK4" ;
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    // Chargement des layers
    if ( ! ConfLoader::buildLayersList ( serverXML, servicesXML ) ) {
        BOOST_LOG_TRIVIAL(fatal) <<   "Impossible de charger la conf des Layers/pyramides" ;
        BOOST_LOG_TRIVIAL(fatal) <<   "Extinction du serveur ROK4" ;
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    // Instanciation du serveur
    return new Rok4Server ( serverXML, servicesXML );
}

Rok4Server* rok4ReloadServer (const char* serverConfigFile, Rok4Server* oldServer, time_t lastReload ) {

    std::string strServerConfigFile = serverConfigFile;

    BOOST_LOG_TRIVIAL(debug) << "Rechargement de la conf";
    //--- server.conf
    BOOST_LOG_TRIVIAL(debug) << "Rechargement du server.conf";

    time_t lastModServerConf = ConfLoader::getLastModifiedDate(strServerConfigFile);

    ServerXML* newServerXML = ConfLoader::buildServerConf(strServerConfigFile);
    if ( ! newServerXML->isOk() ) {
        std::cerr<< "ERREUR FATALE : Impossible d'interpreter le fichier de configuration du serveur " <<strServerConfigFile<<std::endl;
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    if (lastModServerConf > lastReload) {
        //fichier modifié, on recharge particulièrement le logger et on doit vérifier que les fichiers et dossiers
        //indiqués sont les mêmes qu'avant
        BOOST_LOG_TRIVIAL(debug) << "Server.conf modifie";
        // TODO: Reload du logger si nécessaire

    } else {
        //fichier non modifié, il n'y a rien à faire
        BOOST_LOG_TRIVIAL(debug) << "Server.conf non modifie";

    }

    //--- service.conf
    BOOST_LOG_TRIVIAL(debug) << "Rechargement du service.conf et des fichiers associes (listofequalcrs.txt et restrictedcrslist.txt)";
    // Construction des parametres de service
    ServicesXML* newServicesXML = ConfLoader::buildServicesConf ( newServerXML->getServicesConfigFile() );
    if ( ! newServicesXML->isOk() ) {
        BOOST_LOG_TRIVIAL(fatal) <<   "Impossible d'interpreter le fichier de conf " << newServerXML->getServicesConfigFile() ;
        BOOST_LOG_TRIVIAL(fatal) <<   "Extinction du serveur ROK4" ;
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }



    time_t lastMod;
    std::vector<std::string> listOfFile;
    std::string fileName;

    //--- TMS
    BOOST_LOG_TRIVIAL(debug) << "Rechargement des TMS";

    if (newServerXML->getTmsDir() != oldServer->getServerConf()->getTmsDir()) {
        // Le dossier des TMS a changé
        // on recharge tout comme à l'initialisation
        
        BOOST_LOG_TRIVIAL(debug) << "Rechargement complet du nouveau dossier" << newServerXML->getTmsDir();

        if ( ! ConfLoader::buildTMSList ( newServerXML ) ) {
            BOOST_LOG_TRIVIAL(fatal) <<   "Impossible de charger la conf des TileMatrix" ;
            BOOST_LOG_TRIVIAL(fatal) <<   "Extinction du serveur ROK4" ;
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
                        BOOST_LOG_TRIVIAL(error) << "Impossible de charger " << listOfFile[i];
                    }

                } else {
                    //fichier non modifié
                    if (newServerXML->getTMS(fileName) == NULL) {
                        // mais qui n'était pas là au dernier chargement
                        // ça peut arriver lors d'une copie ou d'un déplacement du fichier (les dates ne sont pas modifiées)
                        TileMatrixSet* tms = ConfLoader::buildTileMatrixSet ( listOfFile[i] );

                        if (tms != NULL) {
                            newServerXML->addTMS(tms);
                        } else {
                            BOOST_LOG_TRIVIAL(error) << "Impossible de charger " << listOfFile[i];
                        }
                    }
                }
            }
        } else {
            //aucun fichier dans le dossier
            BOOST_LOG_TRIVIAL(fatal) <<  "Aucun fichier .tms dans le dossier " << newServerXML->getTmsDir() ;
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

        // On supprime de la liste des TMS tous ceux dont l'id ne se retrouve pas dans la liste des noms de fichiers
        newServerXML->cleanTMSs(listOfFileNames);
    }

    //--- STYLES
    BOOST_LOG_TRIVIAL(debug) << "Rechargement des styles";

    if (newServerXML->getStylesDir() != oldServer->getServerConf()->getStylesDir()) {
        // Le dossier des styles a changé
        // on recharge tout comme à l'initialisation
        
        BOOST_LOG_TRIVIAL(debug) << "Rechargement complet du nouveau dossier" << newServerXML->getStylesDir();

        if ( ! ConfLoader::buildStylesList ( newServerXML, newServicesXML ) ) {
            BOOST_LOG_TRIVIAL(fatal) <<   "Impossible de charger la conf des styles" ;
            BOOST_LOG_TRIVIAL(fatal) <<   "Extinction du serveur ROK4" ;
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
                        BOOST_LOG_TRIVIAL(error) << "Impossible de charger " << listOfFile[i];
                    }

                } else {
                    //fichier non modifié
                    if (newServerXML->getStyle(fileName) == NULL) {
                        // mais qui n'était pas là au dernier chargement
                        // ça peut arriver lors d'une copie ou d'un déplacement du fichier (les dates ne sont pas modifiées)
                        Style* sty = ConfLoader::buildStyle ( listOfFile[i], newServicesXML );

                        if (sty != NULL) {
                            newServerXML->addStyle(sty);
                        } else {
                            BOOST_LOG_TRIVIAL(error) << "Impossible de charger " << listOfFile[i];
                        }
                    }
                }
            }
        } else {
            //aucun fichier dans le dossier
            BOOST_LOG_TRIVIAL(fatal) <<  "Aucun fichier .stl dans le dossier " << newServerXML->getStylesDir() ;
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

        // On supprime de la liste des styles tous ceux dont l'id ne se retrouve pas dans la liste des noms de fichiers
        newServerXML->cleanStyles(listOfFileNames);
    }


    //--- Layers
    BOOST_LOG_TRIVIAL(debug) << "Rechargement des Layers";

    if (newServerXML->getLayersDir() != oldServer->getServerConf()->getLayersDir()) {
        // Le dossier des layers a changé
        // on recharge tout comme à l'initialisation

        BOOST_LOG_TRIVIAL(debug) << "Rechargement complet du nouveau dossier" << newServerXML->getLayersDir();

        if ( ! ConfLoader::buildLayersList ( newServerXML, newServicesXML ) ) {
            BOOST_LOG_TRIVIAL(fatal) <<   "Impossible de charger la conf des Layers" ;
            BOOST_LOG_TRIVIAL(fatal) <<   "Extinction du serveur ROK4" ;
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

    } else {

        BOOST_LOG_TRIVIAL(debug) << "Copie des anciens layers";
        //Copie de l'ancien serveur
        std::map<std::string,Layer* >::iterator lv;

        for (lv = oldServer->getLayerList().begin(); lv != oldServer->getLayerList().end(); lv++) {
            Layer* lay = new Layer(lv->second, newServerXML );
            if (lay->getDataPyramid() == NULL) {
                BOOST_LOG_TRIVIAL(error) << "Impossible de cloner le layer " << lv->first;
                delete lay;
            } else {
                newServerXML->addLayer(lay);
            }
        }

        BOOST_LOG_TRIVIAL(debug) << "Lecture du dossier";

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
                        BOOST_LOG_TRIVIAL(error) << "Impossible de charger " << listOfFile[i];
                    }

                } else {
                    //fichier non modifié
                    BOOST_LOG_TRIVIAL(debug) << "Fichier layer non modifie";
                    Layer* lay = newServerXML->getLayer(fileName);
                    if (lay == NULL) {
                        // mais qui n'était pas là au dernier chargement
                        // ça peut arriver lors d'une copie ou d'un déplacement du fichier (les dates ne sont pas modifiées)
                        Layer* l = ConfLoader::buildLayer ( listOfFile[i], newServerXML, newServicesXML );

                        if (l != NULL) {
                            newServerXML->addLayer(l);
                        } else {
                            BOOST_LOG_TRIVIAL(error) << "Impossible de charger " << listOfFile[i];
                        }
                    } else {

                        //on teste si le .pyr a changé
                        std::string strPyrFile = lay->getDataPyramidFilePath();

                        BOOST_LOG_TRIVIAL(debug) << "Verification du fichier pyr" << strPyrFile;
                        lastMod = ConfLoader::getLastModifiedDate(strPyrFile);

                        if (lastMod > lastReload) {
                            // fichier modifié
                            Layer* lay = ConfLoader::buildLayer ( listOfFile[i], newServerXML, newServicesXML );

                            newServerXML->removeLayer(fileName);

                            if (lay != NULL) {
                                newServerXML->addLayer(lay);
                            } else {
                                BOOST_LOG_TRIVIAL(error) << "Impossible de charger le layer " << listOfFile[i];
                            }
                        }
                    }
                }

            }
        } else {
            //aucun fichier dans le dossier
            BOOST_LOG_TRIVIAL(fatal) <<  "Aucun fichier .lay dans le dossier " << newServerXML->getLayersDir() ;
            sleep ( 1 );    // Pour laisser le temps au logger pour se vider
            return NULL;
        }

        // On supprime de la liste des layers tous ceux dont l'id ne se retrouve pas dans la liste des noms de fichiers
        newServerXML->cleanLayers(listOfFileNames);
    }

    return new Rok4Server ( newServerXML, newServicesXML );
}

/**
* \brief Extinction du serveur
*/

void rok4KillServer ( Rok4Server* server ) {
    BOOST_LOG_TRIVIAL(info) <<   "Extinction du serveur ROK4" ;

    //Clear proj4 cache
    pj_clear_initcache();

    delete server;
}

