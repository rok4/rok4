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

class ServerXML;

#ifndef SERVERXML_H
#define SERVERXML_H

#include <vector>
#include <string>
#include <map>
#include "TileMatrixSet.h"

#include "DocumentXML.h"
#include "Layer.h"
#include "Style.h"

#include "config.h"
#include "intl.h"
#include "Rok4Server.h"
#include "ContextBook.h"

class ServerXML : public DocumentXML
{
    friend class Rok4Server;

    public:
        ServerXML(std::string path);
        ~ServerXML();

        bool isOk() ;

        LogOutput getLogOutput() ;
        int getLogFilePeriod() ;
        std::string getLogFilePrefix() ;
        LogLevel getLogLevel() ;

        std::string getServicesConfigFile() ;

        std::string getTmsDir() ;
        std::map<std::string, TileMatrixSet*> getTmsList() ;
        void addTMS(TileMatrixSet* t) ;
        void removeTMS(std::string id) ;
        void cleanTMSs(std::vector<std::string> id) ;
        int getNbTMS() ;
        TileMatrixSet* getTMS(std::string id) ;

        std::string getStylesDir() ;
        std::map<std::string, Style*> getStylesList() ;
        void addStyle(Style* s) ;
        void removeStyle(std::string id) ;
        void cleanStyles(std::vector<std::string> id) ;
        int getNbStyles() ;
        Style* getStyle(std::string id) ;

        std::string getLayersDir() ;
        void addLayer(Layer* l) ;
        void removeLayer(std::string id) ;
        void cleanLayers(std::vector<std::string> id) ;
        int getNbLayers() ;
        Layer* getLayer(std::string id) ;

        ContextBook* getContextBook();
#if BUILD_OBJECT
        int getReconnectionFrequency() ;
#endif
        
        int getNbThreads() ;
        std::string getSocket() ;
        bool getSupportWMTS() ;
        bool getSupportTMS() ;
        bool getSupportWMS() ;
        bool getReprojectionCapability() ;
        int getBacklog() ;
        int getTimeKill() ;

    protected:

        std::string serverConfigFile;
        std::string servicesConfigFile;

        LogOutput logOutput;
        std::string logFilePrefix;
        int logFilePeriod;
        LogLevel logLevel;

        int nbThread;

        /**
         * \~french \brief Défini si le serveur doit honorer les requêtes WMTS
         * \~english \brief Define whether WMTS request should be honored
         */
        bool supportWMTS;
        /**
         * \~french \brief Défini si le serveur doit honorer les requêtes TMS
         * \~english \brief Define whether TMS request should be honored
         */
        bool supportTMS;
        /**
         * \~french \brief Défini si le serveur doit honorer les requêtes WMS
         * \~english \brief Define whether WMS request should be honored
         */
        bool supportWMS;
        bool reprojectionCapability;

        std::string layerDir;
        /**
         * \~french \brief Liste des couches disponibles
         * \~english \brief Available layers list
         */
        std::map<std::string, Layer*> layersList;

        std::string tmsDir;
        /**
         * \~french \brief Liste des TileMatrixSet disponibles
         * \~english \brief Available TileMatrixSet list
         */
        std::map<std::string,TileMatrixSet*> tmsList;

        std::string styleDir;
        /**
         * \~french \brief Liste des styles disponibles
         * \~english \brief Available styles list
         */
        std::map<std::string, Style*> stylesList;

        std::string projDir;

        /**
         * \~french \brief Adresse du socket d'écoute (vide si lancement géré par un tiers)
         * \~english \brief Listening socket address (empty if lauched in managed mode)
         */
        std::string socket;
        /**
         * \~french \brief Profondeur de la file d'attente du socket
         * \~english \brief Socket listen queue depth
         */
        int backlog;

        int timeKill;


        /**
         * \~french \brief Annuaire des contextes de stockage
         * \~english \brief Storage context's book
         */
        ContextBook* objectBook;


#if BUILD_OBJECT

        int reconnectionFrequency;

#endif

        int nbProcess;

    private:

        bool ok;
};

#endif // SERVERXML_H

