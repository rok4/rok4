/*
 * Copyright © (2011) Institut national de l'information
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
 * \file Rok4Server.h
 * \~french
 * \brief Definition de la classe Rok4Server et du programme principal
 * \~english
 * \brief Define the Rok4Server class, handling the event loop
 */

#ifndef _ROK4SERVER_
#define _ROK4SERVER_

#include "config.h"
#include "ResponseSender.h"
#include "Data.h"
#include "Request.h"
#include <pthread.h>
#include <map>
#include <vector>
#include "ServicesConf.h"
#include "Layer.h"
#include "TileMatrixSet.h"
#include "fcgiapp.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Un serveur Rok4 stocke les informations de configurations des services
 * Il définit aussi la boucle d'évènement nécessaire pour répondre aux requête transmises via FCGI
 * \brief Gestion du programme principal, lien entre les modules
 * \~english
 * The Rok4 Server stores services configuration.
 * It also define the event loop to handle the FCGI request
 * \brief Handle the main program (event loop) and links
 */
class Rok4Server {
private:
    /**
     * \~french \brief Liste des processus léger 
     * \~english \brief Threads liste
     */
    std::vector<pthread_t> threads;
    /**
     * \~french \brief Connecteur sur le flux FCGI
     * \~english \brief FCGI stream connector
     */
    ResponseSender S;
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
    /**
     * \~french \brief Défini si le serveur est en cours d'éxécution 
     * \~english \brief Define whether the server is running
     */
    volatile bool running;
    /**
     * \~french \brief Adresse du socket d'écoute (vide si lancement géré par un tiers)
     * \~english \brief Listening socket adress (empty if lauched in managed mode)
     */
    std::string socket;
    /**
     * \~french \brief Profondeur de la file d'attente du socket
     * \~english \brief Socket listen queue depth
     */
    int backlog;
    /**
     * \~french \brief Identifiant du socket
     * \~english \brief Socket identifier
     */
    int sock;

    /**
     * \~french \brief Configurations globales du services
     * \~english \brief Global services configuration
     */
    ServicesConf servicesConf;
    /**
     * \~french \brief Liste des couches disponibles
     * \~english \brief Available layers list
     */
    std::map<std::string, Layer*> layerList;
    /**
     * \~french \brief Liste des TileMatrixSet disponibles
     * \~english \brief Available TileMatrixSet list
     */
    std::map<std::string, TileMatrixSet*> tmsList;
    /**
     * \~french \brief Liste des styles disponibles
     * \~english \brief Available styles list
     */
    std::map<std::string, Style*> styleList;
    /**
     * \~french \brief Liste des fragments invariants de capabilities prets à être concaténés avec les infos de la requête.
     * \~english \brief Invariant GetCapabilities fragments ready to be concatained with request informations
     */
    std::vector<std::string> wmsCapaFrag; 
    /**
     * \~french \brief Liste des fragments invariants de capabilities prets à être concaténés avec les infos de la requête.
     * \~english \brief Invariant GetCapabilities fragments ready to be concatained with request informations
     */
    std::vector<std::string> wmtsCapaFrag; 
    
    /**
     * \~french \brief Erreur à retourner en cas de tuile non trouvée (http 404)
     * \~english \brief Error response in case data tiel is not found (http 404)
     */
    DataSource* notFoundError;
    
    /**
     * \~french
     * \brief Boucle principale exécuté par chaque thread à l'écoute des requêtes des utilisateurs.
     * \param[in] arg pointeur vers l'instance de Rok4Server
     * \return true si présent
     * \~english
     * \brief Main event loop executed by each thread, listening to user request
     * \param[in] arg pointer to the Rok4Server instance
     * \return true if present
     */
    static void* thread_loop ( void* arg );

    int GetDecimalPlaces ( double number );
    void buildWMSCapabilities();
    void buildWMTSCapabilities();
    
    /**
     * \~french
     * \brief Test de la présence d'un paramètre dans la requête
     * \param[in] option liste des paramètres
     * \param[in] paramName nom du paramètre à tester
     * \return true si présent
     * \~english
     * \brief Test if the request contain a specific parameter
     * \param[in] option parameter list
     * \param[in] paramName parameter to test
     * \return true if present
     */
    bool hasParam ( std::map<std::string, std::string>& option, std::string paramName );
    /**
     * \~french
     * \brief Récupération de la valeur d'un paramètre dans la requête
     * \param[in] option liste des paramètres
     * \param[in] paramName nom du paramètre
     * \return valeur du parametre ou "" si non présent
     * \~english
     * \brief Fetch a specific parameter value in the request
     * \param[in] option parameter list
     * \param[in] paramName parameter name
     * \return parameter value or "" if not availlable
     */
    std::string getParam ( std::map<std::string, std::string>& option, std::string paramName );
    
    
    /**
 * Traitement d'une requete GetMap
 * @return Un pointeur sur le flux de donnees resultant
 * @return Un message d'erreur en cas d'erreur
 */
    DataStream* getMap ( Request* request );
    DataStream* WMSGetCapabilities ( Request* request );
    /** Traite les requêtes de type WMS */
    void        processWMS ( Request *request, FCGX_Request&  fcgxRequest );
    /** Traite les requêtes de type WMTS */
    void        processWMTS ( Request *request, FCGX_Request&  fcgxRequest );
    /** Separe les requetes WMS et WMTS */
    void        processRequest ( Request *request, FCGX_Request&  fcgxRequest );

public:
    
    ServicesConf& getServicesConf() {
        return servicesConf;
    }
    std::map<std::string, Layer*>& getLayerList() {
        return layerList;
    }
    std::map<std::string, TileMatrixSet*>& getTmsList() {
        return tmsList;
    }
    std::map<std::string, Style*>& getStyleList() {
        return styleList;
    }
    std::vector<std::string>& getWmsCapaFrag() {
        return wmsCapaFrag;
    }
    std::vector<std::string>& getWmtsCapaFrag() {
        return wmtsCapaFrag;
    }
    
    /**
 * Traitement d'une requete GetTile
 * @return Un pointeur sur la source de donnees de la tuile requetee
 * @return Un message d'erreur en cas d'erreur
 */
    DataSource* getTile ( Request* request );
    DataStream* WMTSGetCapabilities ( Request* request );

    /**
 * Lancement des threads du serveur
 */
    void run();
    /**
     * Initialize the FastCGI Socket 
     */
    void initFCGI();

    /**
     * Destroy the FastCGI Socket 
     */
    void killFCGI();
    
    /**
     * Get the internal FastCGI socket representation, usefull for configuration reloading.
     * @return the internal FastCGI socket representation
     */
    int getFCGISocket() {return sock;}
    
    /**
     * Set the internal FastCGI socket representation, usefull for configuration reloading
     * @param sockFCGI the internal FastCGI socket representation
     */
    void setFCGISocket(int sockFCGI) {sock = sockFCGI;}
    
    
    bool isRunning() { return running ;}
    void terminate();
    
    
    /**
     * \brief Construction du serveur
     */
    Rok4Server ( int nbThread, ServicesConf& servicesConf, std::map<std::string,Layer*> &layerList,
                 std::map<std::string,TileMatrixSet*> &tmsList, std::map<std::string,Style*> &styleList, std::string socket, int backlog, bool supportWMTS = true, bool supportWMS = true);
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Rok4Server ();

};

#endif

