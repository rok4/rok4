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
#include "Layer.h"
#include <stdio.h>
#include "TileMatrixSet.h"
#include "DocumentXML.h"
#include "ProcessFactory.h"
#include "fcgiapp.h"
#include <csignal>
#include "ServerXML.h"
#include "ServicesXML.h"
#include "GetFeatureInfoEncoder.h"

#if BUILD_OBJECT
#include "ContextBook.h"
#endif

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Un serveur Rok4 stocke les informations de configurations des services
 * Il définit aussi la boucle d'évènement nécessaire pour répondre aux requêtes transmises via FCGI
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

#if BUILD_OBJECT
    /**
     * \~french \brief Thread de reconnexion des contextes
     * \~english \brief Contexts reconnection thread
     */
    pthread_t reco_thread;
#endif

    /**
     * \~french \brief Connecteur sur le flux FCGI
     * \~english \brief FCGI stream connector
     */
    ResponseSender S;

    /**
     * \~french \brief Défini si le serveur est en cours d'éxécution
     * \~english \brief Define whether the server is running
     */
    volatile bool running;

    /**
     * \~french \brief Identifiant du socket
     * \~english \brief Socket identifier
     */
    int sock;

    /**
     * \~french \brief Configurations globales des services
     * \~english \brief Global services configuration
     */
    ServicesXML* servicesConf;

    /**
     * \~french \brief Configurations globales du serveur
     * \~english \brief Global server configuration
     */
    ServerXML* serverConf;

    /**
     * \~french \brief Liste des fragments invariants de capabilities prets à être concaténés avec les infos de la requête.
     * \~english \brief Invariant GetCapabilities fragments ready to be concatained with request informations
     */
    std::map<std::string,std::vector<std::string> > wmsCapaFrag;
    /**
     * \~french \brief Liste des fragments invariants de capabilities prets à être concaténés avec les infos de la requête.
     * \~english \brief Invariant GetCapabilities fragments ready to be concatained with request informations
     */
    std::vector<std::string> wmtsCapaFrag;
    /**
     * \~french \brief Liste des fragments invariants de capabilities prets à être concaténés avec les infos de la requête.
     * \~english \brief Invariant GetCapabilities fragments ready to be concatained with request informations
     */
    std::vector<std::string> tmsCapaFrag;


    /**
     * \~french \brief Gestion des processus créés
     * \~english \brief Management of created process
     */
    ProcessFactory *parallelProcess;

    /**
     * \~french
     * \brief Boucle principale exécutée par chaque thread à l'écoute des requêtes des utilisateurs.
     * \param[in] arg pointeur vers l'instance de Rok4Server
     * \return true si présent
     * \~english
     * \brief Main event loop executed by each thread, listening to user request
     * \param[in] arg pointer to the Rok4Server instance
     * \return true if present
     */
    static void* thread_loop ( void* arg );


#if BUILD_OBJECT
    /**
     * \~french
     * \brief Boucle principale exécutée par le thread de reconnexion des contextes de lecture #reco_thread
     * \param[in] arg pointeur vers l'instance de Rok4Server
     * \return true si présent
     * \~english
     * \brief Main event loop executed by reconnection thread #reco_thread
     * \param[in] arg pointer to the Rok4Server instance
     * \return true if present
     */
    static void* thread_reconnection_loop ( void* arg );
#endif
    
    /**
     * \~french
     * \brief Donne le nombre de chiffres après la virgule
     * \details 3.14 -> 2, 1.0001 -> 4, le maximum est 10
     * \param[in] arg un double
     * \return int valeur
     * \~english
     * \brief Give the number of decimal places
     * \details 3.14 -> 2, 1.0001 -> 4, maximum is 10
     * \param[in] arg a double
     * \return int value
     */
    int GetDecimalPlaces ( double number );

    //---- WMS 1.1.1
    /**
     * \~french
     * \brief Construit les fragments invariants du getCapabilities WMS 1.1.1
     * \~english
     * \brief Build the invariant fragments of the WMS GetCapabilities 1.1.1
     */
    void buildWMS111Capabilities();
    //----
    /**
     * \~french
     * \brief Construit les fragments invariants du getCapabilities WMS
     * \~english
     * \brief Build the invariant fragments of the WMS GetCapabilities
     */
    void buildWMS130Capabilities();
    /**
     * \~french
     * \brief Construit les fragments invariants du getCapabilities WMS
     * \~english
     * \brief Build the invariant fragments of the WMTS GetCapabilities
     */
    void buildWMTSCapabilities();
    /**
     * \~french
     * \brief Construit les fragments invariants du getCapabilities TMS
     * \~english
     * \brief Build the invariant fragments of the TMS GetCapabilities
     */
    void buildTMSCapabilities();

    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetTile
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating GetTile request parameters
     * \return NULL or an error message if something went wrong
     */
    DataSource* getTileParamWMTS ( Request* request, Layer*& layer, std::string& str_tileMatrix, int& tileCol, int& tileRow, std::string& format, Style*& style);

    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête de tuile en TMS
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating get tile TMS request parameters
     * \return NULL or an error message if something went wrong
     */
    DataSource* getTileParamTMS ( Request* request, Layer*& layer, std::string& str_tileMatrix, int& tileCol, int& tileRow, std::string& format, Style*& style);

    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetMap
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating GetTile request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* getMapParamWMS ( Request* request, std::vector<Layer*>& layers, BoundingBox< double >& bbox, int& width, int& height, CRS& crs, std::string& format, std::vector<Style*>& styles, std::map< std::string, std::string >& format_option,int& dpi);
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetCapabilities WMS
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating GetTile request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* getCapParamWMS ( Request* request, std::string& version );
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetTile WMTS
     * \return message d'erreur en caspa d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating GetTile request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* getCapParamWMTS ( Request* request, std::string& version );
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetLayer TMS
     * \return message d'erreur en caspa d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating GetLayer TMS request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* getLayerParamTMS ( Request* request, Layer*& layer, std::string& url );

    //Greg
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetFeatureInfoParam WMS
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating WMS GetFeatureInfoParam request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* getFeatureInfoParamWMS (
        Request* request, std::vector<Layer*>& layers, std::vector<Layer*>& query_layers,
        BoundingBox< double >& bbox, int& width, int& height, CRS& crs, std::string& format,
        std::vector<Style*>& styles, std::string& info_format, int& X, int& Y, int& feature_count,std::map <std::string, std::string >& format_option
    );
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetFeatureInfoParam WMTS
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating WMTS GetFeatureInfoParam request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* getFeatureInfoParamWMTS ( Request* request, Layer*& layer, std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format, Style* &style, std::string& info_format, int& X, int& Y);


    /**
     * \~french
     * \brief Applique un style à une image
     * \param[in] image à styliser
     * \param[in] pyrType format des tuiles de la pyramide
     * \param[in] style style demandé par le client
     * \param[in] format demandé par le client
     * \param[in] size nombre d'images concernées par le processus global où est appelé cette fonction
     * \return image stylisée
     * \~english
     * \brief Apply a style to an image
     * \param[in] image
     * \param[in] pyrType tile format of the pyramid
     * \param[in] style asked style by the client
     * \param[in] format asked format by the client
     * \param[in] size number of images used in the global process where this function is called
     * \return requested and styled image
     */
    Image *styleImage(Image *curImage, Rok4Format::eformat_data pyrType, Style *style, std::string format, int size, Pyramid *pyr);
    /**
     * \~french
     * \brief Fond un groupe d'image en une seule
     * \param[in] images groupe d'images
     * \param[in] pyrType format des tuiles de la pyramide
     * \param[in] style style demandé par le client
     * \param[in] crs crs de la projection finale demandée par le client
     * \param[in] bbox bbox de l'image demandé par le client
     * \return image demandée ou un message d'erreur
     * \~english
     * \brief Merge a vector of images
     * \param[in] images vector of images
     * \param[in] pyrType tile format of the pyramid
     * \param[in] style asked style by the client
     * \param[in] crs crs of the final projection asked by the client
     * \param[in] bbox bbox of the image asked by the client
     * \return requested image or an error message
     */
    Image *mergeImages(std::vector<Image*> images, Rok4Format::eformat_data &pyrType, Style *style, CRS crs, BoundingBox<double> bbox);
    /**
     * \~french
     * \brief Convertie une image dans un format donné
     * \param[in] image image à formater
     * \param[in] format demandé par le client
     * \param[in] pyrType format des tuiles de la pyramide
     * \param[in] format_option contient des spécifications sur le format
     * \param[in] size nombre d'images concerné par le processus global où est appelé cette fonction
     * \param[in] style style demandé par le client
     * \return image demandé ou un message d'erreur sous forme de stream
     * \~english
     * \brief Apply a format to an image
     * \param[in] image image to format
     * \param[in] format asked format by the client
     * \param[in] pyrType tile format of the pyramid
     * \param[in] format_option contain specifications on the format
     * \param[in] size number of images used in the global process where this function is called
     * \param[in] style asked style by the client
     * \return requested image or an error message by a stream
     */
    DataStream *formatImage(Image *image, std::string format, Rok4Format::eformat_data pyrType, std::map<std::string, std::string> format_option, int size, Style *style);
    /**
     * \~french
     * \brief Renvoit une tuile déjà pré-calculée
     * \param[in] L couche de la requête
     * \param[in] format format de la requête
     * \param[in] tileCol indice de colonne de la requête
     * \param[in] tileRow indice de ligne de la requete
     * \param[in] tileMatrix tilematrix de la requête
     * \param[in] errorResp paramètre d'erreur
     * \param[in] style style de la requête
     * \return image demandé ou un message d'erreur
     * \~english
     * \brief Give a tile computed before
     * \param[in] L layer of the request
     * \param[in] format format of the request
     * \param[in] tileCol column index of the request
     * \param[in] tileRow row index of the request
     * \param[in] tileMatrix tilematrix of the request
     * \param[in] errorResp error parameter
     * \param[in] style style of the resquest
     * \return requested tile
     */
    DataSource *getTileUsual(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style, std::string format);
    /**
     * \~french
     * \brief Renvoit une tuile qui vient d'être calculée
     * \param[in] L couche de la requête
     * \param[in] tileMatrix tilematrix de la requête
     * \param[in] tileCol indice de colonne de la requête
     * \param[in] tileRow indice de ligne de la requete
     * \param[in] style style de la requête
     * \param[in] format format de la requête
     * \return Tuile demandée
     * \~english
     * \brief Give a tile compute for the request
     * \param[in] L layer of the request
     * \param[in] tileMatrix tilematrix of the request
     * \param[in] tileCol column index of the request
     * \param[in] tileRow row index of the request
     * \param[in] style style of the resquest
     * \param[in] format format of the request
     * \return requested tile
     */
    DataSource *getTileOnDemand(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style, std::string format);
    /**
     * \~french
     * \brief Renvoit une tuile qui vient d'être calculée
     * \param[in] L couche de la requête
     * \param[in] tileMatrix tilematrix de la requête
     * \param[in] tileCol indice de colonne de la requête
     * \param[in] tileRow indice de ligne de la requete
     * \param[in] style style de la requête
     * \param[in] format format de la requête
     * \param[in] errorResp erreur
     * \return Tuile demandée
     * \~english
     * \brief Give a tile compute for the request
     * \param[in] L layer of the request
     * \param[in] tileMatrix tilematrix of the request
     * \param[in] tileCol column index of the request
     * \param[in] tileRow row index of the request
     * \param[in] style style of the resquest
     * \param[in] format format of the request
     * \param[in] errorResp error
     * \return requested tile
     */
    DataSource *getTileOnFly(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style, std::string format);
    /**
     * \~french
     * \brief Creation d'une dalle concernée par la tuile qui vient d'être calculée
     * \param[in] L couche de la requête
     * \param[in] tileMatrix tilematrix de la requête
     * \param[in] tileCol indice de colonne de la requête
     * \param[in] tileRow indice de ligne de la requete
     * \param[in] style style de la requête
     * \param[in] format format de la requete
     * \param[in] path chemin pour sauvegarder la dalle
     * \return 0 si ok, 1 sinon
     * \~english
     * \brief Create a slab concerned by the tile compute for the request
     * \param[in] L layer of the request
     * \param[in] tileMatrix tilematrix of the request
     * \param[in] tileCol column index of the request
     * \param[in] tileRow row index of the request
     * \param[in] style style of the resquest
     * \param[in] format format of the request
     * \param[in] path path to save the slab
     * \return 0 if ok, else 1
     */
    int createSlabOnFly(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style, std::string format, std::string path);


    /**
     * \~french
     * \brief Traitement d'une requête GetCapabilities WMS
     * \param[in] request représentation de la requête
     * \return flux de la réponse
     * \~english
     * \brief Process a GetCapabilities WMS request
     * \param[in] request request representation
     * \return response stream
     */
    DataStream* WMSGetCapabilities ( Request* request );
    /**
     * \~french
     * \brief Traitement d'une requête GetCapabilities WMTS
     * \param[in] request représentation de la requête
     * \return flux de la réponse
     * \~english
     * \brief Process a GetCapabilities WMTS request
     * \param[in] request request representation
     * \return response stream
     */
    DataStream* WMTSGetCapabilities ( Request* request );
    /**
     * \~french
     * \brief Traitement d'une requête GetCapabilities TMS
     * \param[in] request représentation de la requête
     * \return flux de la réponse
     * \~english
     * \brief Process a GetCapabilities TMS request
     * \param[in] request request representation
     * \return response stream
     */
    DataStream* TMSGetCapabilities ( Request* request );
    /**
     * \~french
     * \brief Traitement d'une requête GetServices TMS
     * \param[in] request représentation de la requête
     * \return flux de la réponse
     * \~english
     * \brief Process a GetServices TMS request
     * \param[in] request request representation
     * \return response stream
     */
    DataStream* TMSGetServices ( Request* request );
    /**
     * \~french
     * \brief Traitement d'une requête GetLayer TMS
     * \param[in] request représentation de la requête
     * \return flux de la réponse
     * \~english
     * \brief Process a GetLayer TMS request
     * \param[in] request request representation
     * \return response stream
     */
    DataStream* TMSGetLayer ( Request* request );
    /**
     * \~french
     * \brief Traitement d'une requête GetLayerMetadata TMS
     * \param[in] request représentation de la requête
     * \return flux de la réponse
     * \~english
     * \brief Process a GetLayerMetadata TMS request
     * \param[in] request request representation
     * \return response stream
     */
    DataStream* TMSGetLayerMetadata ( Request* request );


    /**
     * \~french
     * \brief Traitement d'une requête GetMap
     * \param[in] request représentation de la requête
     * \return image demandé ou un message d'erreur
     * \~english
     * \brief Process a GetMap request
     * \param[in] request request representation
     * \return requested image or an error message
     */
    DataStream* getMap ( Request* request );

    /**
     * \~french
     * \brief Traitement d'une requête GetTile
     * \param[in] request représentation de la requête
     * \return image demandé ou un message d'erreur
     * \~english
     * \brief Process a GetTile request
     * \param[in] request request representation
     * \return requested image or an error message
     */
    DataSource* getTile ( Request* request );

    /**
     * \~french
     * \brief Traitement d'une requête GetFeatureInfo WMS
     * \param[in] request représentation de la requête
     * \return flux de la réponse
     * \~english
     * \brief Process a GetFeatureInfo WMS request
     * \param[in] request request representation
     * \return response stream
     */
    DataStream* WMSGetFeatureInfo ( Request* request );
    /**
     * \~french
     * \brief Traitement d'une requête GetFeatureInfo WMTS
     * \param[in] request représentation de la requête
     * \return flux de la réponse
     * \~english
     * \brief Process a GetFeatureInfo WMTS request
     * \param[in] request request representation
     * \return response stream
     */
    DataStream* WMTSGetFeatureInfo ( Request* request );

    DataStream* CommonGetFeatureInfo ( std::string service, Layer* layer, BoundingBox<double> bbox, int width, int height, CRS crs, std::string info_format , int X, int Y, std::string format, int feature_count);

    /**
     * \~french Traite les requêtes de type WMS
     * \~english Process WMS request
     */
    void processWMS ( Request *request, FCGX_Request&  fcgxRequest );
    /**
     * \~french Traite les requêtes de type WMTS
     * \~english Process WMTS request
     */
    void processWMTS ( Request *request, FCGX_Request&  fcgxRequest );
    /**
     * \~french Traite les requêtes de type TMS
     * \~english Process TMS request
     */
    void processTMS ( Request *request, FCGX_Request&  fcgxRequest );
    /**
     * \~french Sépare les requêtes de type WMS et WMTS
     * \~english Route WMS and WMTS request
     */
    void processRequest ( Request *request, FCGX_Request&  fcgxRequest );

    /**
     * \~french
     * \brief Découpe une chaîne de caractères selon un délimiteur
     * \param[in] s la chaîne à découper
     * \param[in] delim le délimiteur
     * \return la liste contenant les parties de la chaîne
     * \~english
     * \brief Split a string using a specified delimitor
     * \param[in] s the string to split
     * \param[in] delim the delimitor
     * \return the list with the splited string
     */
    static std::vector<std::string> split ( const std::string &s, char delim ) {
        std::vector<std::string> elems;
        std::stringstream ss ( s );
        std::string item;
        while ( std::getline ( ss, item, delim ) ) {
            elems.push_back ( item );
        }
        return elems;
    }

    /**
     * \~french Conversion d'un entier en une chaîne de caractère
     * \~english Convert an integer in a character string
     */
    static std::string numToStr ( long int i ) {
        std::ostringstream strstr;
        strstr << i;
        return strstr.str();
    }

    /**
     * \~french Conversion d'un flottant en une chaîne de caractères
     * \~english Convert a float in a character string
     */
    static std::string doubleToStr ( long double d ) {
        std::ostringstream strstr;
        strstr.setf ( std::ios::fixed,std::ios::floatfield );
        strstr.precision ( 16 );
        strstr << d;
        return strstr.str();
    }


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
    static bool hasParam ( std::map<std::string, std::string>& option, std::string paramName ) {
        std::map<std::string, std::string>::iterator it = option.find ( paramName );
        if ( it == option.end() ) {
            return false;
        }
        return true;
    }

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
    static std::string getParam ( std::map<std::string, std::string>& option, std::string paramName ) {
        std::map<std::string, std::string>::iterator it = option.find ( paramName );
        if ( it == option.end() ) {
            return "";
        }
        return it->second;
    }
public:
    /**
     * \~french Retourne la configuration des services
     * \~english Return the services configurations
     */
    ServicesXML* getServicesConf() ;
    /**
     * \~french Retourne la configuration du serveur
     * \~english Return the server configuration
     */
    ServerXML* getServerConf() ;

    /**
     * \~french Retourne la liste des couches
     * \~english Return the layers list
     */
    std::map<std::string, Layer*>& getLayerList() ;
    /**
     * \~french Retourne la liste des TileMatrixSets
     * \~english Return the TileMatrixSets list
     */
    std::map<std::string, TileMatrixSet*>& getTmsList() ;
    /**
     * \~french Retourne la liste des styles
     * \~english Return the styles list
     */
    std::map<std::string, Style*>& getStylesList() ;
    /**
     * \~french Retourne les fragments du GetCapabilities WMS
     * \~english Return WMS GetCapabilities fragments
     */
    std::map<std::string,std::vector<std::string> >& getWmsCapaFrag() ;
    /**
     * \~french Retourne les fragments du GetCapabilities WMTS
     * \~english Return WMTS GetCapabilities fragments
     */
    std::vector<std::string>& getWmtsCapaFrag() ;

#if BUILD_OBJECT
    /**
     * \~french Retourne l'annuaire de contextes ceph
     */
    ContextBook* getCephBook() ;
    /**
     * \~french Retourne l'annuaire de contextes s3
     */
    ContextBook* getS3Book() ;
    /**
     * \~french Retourne l'annuaire de contextes swift
     */
    ContextBook* getSwiftBook() ;
#endif

    /**
     * \~french
     * \brief Lancement des threads du serveur
     * \~english
     * \brief Start server's thread
     */
    void run(sig_atomic_t signal_pending = 0);
    /**
     * \~french
     * \brief Initialise le socket FastCGI
     * \~english
     * \brief Initialize the FastCGI Socket
     */
    void initFCGI();
    /**
     * \~french
     * \brief Détruit le socket FastCGI
     * \~english
     * \brief Destroy the FastCGI Socket
     */
    void killFCGI();
    /**
     * \~french
     * Utilisé pour le rechargement de la configuration du serveur
     * \brief Retourne la représentation interne du socket FastCGI
     * \return la représentation interne du socket
     * \~english
     * \brief Get the internal FastCGI socket representation, usefull for configuration reloading.
     * \return the internal FastCGI socket representation
     */
    int getFCGISocket() ;

    /**
     * \~french
     * Utilisé pour le rechargement de la configuration du serveur
     * \brief Restaure le socket FastCGI
     * \param sockFCGI la représentation interne du socket
     * \~english
     * Useful for configuration reloading
     * \brief Set the internal FastCGI socket representation
     * \param sockFCGI the internal FastCGI socket representation
     */
    void setFCGISocket ( int sockFCGI ) ;
    
     /**
     * \~french
     * \brief Demande l'arrêt du serveur
     * \~english
     * \brief Ask for server shutdown
     */
    void terminate();

    /**
     * \~french
     * \brief Retourne l'état du serveur
     * \return true si en fonctionnement
     * \~english
     * \brief Return the server state
     * \return true if running
     */
    bool isRunning() ;
    
    /**
     * \~french
     * \brief Pour savoir si le server honore les requêtes WMTS
     * \~english
     * \brief to know if the server responde to WMTS request
     */
    bool isWMTSSupported();

    /**
     * \~french
     * \brief Pour savoir si le server honore les requêtes TMS
     * \~english
     * \brief to know if the server responde to TMS request
     */
    bool isTMSSupported();
    
    /**
     * \~french
     * \brief Pour savoir si le server honore les requêtes WMS
     * \~english
     * \brief to know if the server responde to WMS request
     */
    bool isWMSSupported();
    /**
     * \~french
     * \brief Pour savoir si le server honore les requêtes WMTS
     * \~english
     * \brief to know if the server responde to WMTS request
     */
    void setProxy(Proxy pr);

    /**
     * \~french
     * \brief Retourne le proxy par defaut
     * \~english
     * \brief Return default proxy
     */
    Proxy getProxy();

    /**
     * \brief Construction du serveur
     */
    Rok4Server ( ServerXML* serverXML, ServicesXML* servicesXML);
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Rok4Server ();

};

#endif

