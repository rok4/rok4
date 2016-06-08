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

#ifndef SERVERXML_H
#define SERVERXML_H

#include <vector>
#include <string>

#include "ContextBook.h"
#include "Rok4Server.h"

#include "config.h"
#include "intl.h"

struct Proxy {
    std::string proxyName;
    std::string noProxy;
};

class ServerXML
{
    friend class Rok4Server;

    public:
        ServerXML(std::string serverConfigFile);
        ~ServerXML(){
        }

        bool isOk() { return ok; }

        LogOutput getLogOutput() {return logOutput;}
        int getLogFilePeriod() {return logFilePeriod;}
        std::string getLogFilePrefix() {return logFilePrefix;}
        LogLevel getLogLevel() {return logLevel;}

        std::string getServicesConfigFile() {return servicesConfigFile;}

        std::string getTmsDir() {return tmsDir;}
        std::string getStylesDir() {return styleDir;}
        std::string getLayersDir() {return layerDir;}

        ContextBook* getCephContextBook(){return cephBook;};
        ContextBook* getSwiftContextBook(){return swiftBook;};
        
        int getNbThreads() {return nbThread;}
        std::string getSocket() {return socket;}
        bool getSupportWMTS() {return supportWMTS;}
        bool getSupportWMS() {return supportWMS;}
        int getBacklog() {return backlog;}
        Proxy getProxy() {return proxy;}

    protected:

    std::string serverConfigFile;

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
     * \~french \brief Défini si le serveur doit honorer les requêtes WMS
     * \~english \brief Define whether WMS request should be honored
     */
    bool supportWMS;
    bool reprojectionCapability;

    std::string servicesConfigFile;
    std::string layerDir;
    std::string tmsDir;
    std::string styleDir;

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

    std::string cephName;
    std::string cephUser;
    std::string cephConf;

    std::string swiftAuthUrl;
    std::string swiftUserName;
    std::string swiftUserAccount;
    std::string swiftUserPassword;

    int nbProcess;

    /**
     * \~french \brief Proxy utilisé par défaut pour des requêtes WMS
     * \~english \brief Default proxy used for WMS requests
     */
    Proxy proxy;

    ContextBook* cephBook;
    ContextBook* swiftBook;

    bool ok;

    private:
};

#endif // SERVERXML_H

