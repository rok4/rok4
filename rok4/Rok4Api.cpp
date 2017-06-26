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

    ServerXML* serverXML = ConfLoader::getTechnicalParam(strServerConfigFile);
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

}

#endif


