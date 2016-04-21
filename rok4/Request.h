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

#ifndef REQUEST_H_
#define REQUEST_H_

#include <map>
#include <vector>
#include "BoundingBox.h"
#include "Data.h"
#include "CRS.h"
#include "Layer.h"
#include "ServicesConf.h"

/**
 * \file Request.h
 * \~french
 * \brief Définition de la classe Request, analysant les requêtes HTTP
 * \~english
 * \brief Define the Request Class analysing HTTP requests
 */

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Classe décodant les requêtes HTTP envoyé au serveur.
 * Elle supporte les types de requête suivant :
 *  - HTTP GET de type KVP
 *  - HTTP POST de type XML (OGC SLD)
 * \todo HTTP POST de type KVP
 * \todo HTTP GET de type REST
 * \todo HTTP POST de type XML/Soap
 * \brief Gestion des requêtes HTTP
 * \~english
 * HTTP request decoder class.
 * It support the following request type
 *  - HTTP GET, KVP style
 *  - HTTP POST , XML style (OGC SLD)
 * \todo HTTP POST, KVP style
 * \todo HTTP GET, REST style
 * \todo HTTP POST, XML/Soap style
 * \brief HTTP requests handler
 */
class Request {
    friend class CppUnitRequest;
private:
    /**
     * \~french
     * \brief Décodage de l'URL correspondant à la requête
     * \param[in,out] src URL
     * \~english
     * \brief URL decoding
     * \param[in,out] src URLs
     */
    void url_decode ( char *src );
    /**
     * \~french
     * \brief Test de la présence d'un paramètre dans la requête
     * \param[in] paramName nom du paramètre à tester
     * \return true si présent
     * \~english
     * \brief Test if the request contain a specific parameter
     * \param[in] paramName parameter to test
     * \return true if present
     */
    bool hasParam ( std::string paramName );
    /**
     * \~french
     * \brief Récupération de la valeur d'un paramètre dans la requête
     * \param[in] paramName nom du paramètre
     * \return valeur du parametre ou "" si non présent
     * \~english
     * \brief Fetch a specific parameter value in the request
     * \param[in] paramName parameter name
     * \return parameter value or "" if not availlable
     */
    std::string getParam ( std::string paramName );

public:
    /**
     * \~french \brief Nom de domaine de la requête
     * \~english \brief Request domain name
     */
    std::string hostName;
    /**
     * \~french \brief Chemin du serveur web pour accèder au service
     * \~english \brief Web Server path of the service
     */
    std::string path;
    /**
     * \~french \brief Type de service (WMS,WMTS)
     * \~english \brief Service type (WMS,WMTS)
     */
    std::string service;
    /**
     * \~french \brief Nom au sens OGC de la requête effectuée
     * \~english \brief OGC request name
     */
    std::string request;
    /**
     * \~french \brief Protocole de la requête (http,https)
     * \~english \brief Request protocol (http,https)
     */
    std::string scheme;
    /**
     * \~french \brief Liste des paramètres de la requête
     * \~english \brief Request parameters list
     */
    std::map<std::string, std::string> params;
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetTile
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating GetTile request parameters
     * \return NULL or an error message if something went wrong
     */
    DataSource* getTileParam ( ServicesConf& servicesConf,  std::map<std::string,TileMatrixSet*>& tmsList, std::map<std::string, Layer*>& layerList, Layer*& layer, std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format, Style* &style, bool& noDataError );
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetMap
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating GetTile request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* getMapParam ( ServicesConf& servicesConf, std::map< std::string, Layer* >& layerList, std::vector<Layer*>& layers, BoundingBox< double >& bbox, int& width, int& height, CRS& crs, std::string& format, std::vector<Style*>& styles,std::map <std::string, std::string >& format_option );
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetCapabilities WMS
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating GetTile request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* getCapWMSParam ( ServicesConf& servicesConf, std::string& version );
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetTile WMTS
     * \return message d'erreur en caspa d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating GetTile request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* getCapWMTSParam ( ServicesConf& servicesConf, std::string& version );

    //Greg
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetFeatureInfoParam WMS
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating WMS GetFeatureInfoParam request parameters
     * \return NULL or an error message if something went wrong
     */
    DataStream* WMSGetFeatureInfoParam (ServicesConf& servicesConf, std::map< std::string, Layer* >& layerList, std::vector<Layer*>& layers,
                                     std::vector<Layer*>& query_layers,
                                     BoundingBox< double >& bbox, int& width, int& height, CRS& crs, std::string& format,
                                     std::vector<Style*>& styles, std::string& info_format, int& X, int& Y, int& feature_count,std::map <std::string, std::string >& format_option);
    /**
     * \~french
     * \brief Récuperation et vérifications des paramètres d'une requête GetFeatureInfoParam WMTS
     * \return message d'erreur en cas d'erreur, NULL sinon
     * \~english
     * \brief Fetching and validating WMTS GetFeatureInfoParam request parameters
     * \return NULL or an error message if something went wrong
     */
    DataSource* WMTSGetFeatureInfoParam (ServicesConf& servicesConf,  std::map<std::string,TileMatrixSet*>& tmsList, std::map<std::string, Layer*>& layerList,
                                         Layer*& layer, std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format, Style* &style,
                                         bool& noDataError, std::string& info_format, int& X, int& Y);
    //

    /**
     * \~french
     * \brief Précise si le path contient un mot
     * \param word
     * \return bool
     * \~english
     * \brief Precise if the path contains a word
     * \param word
     * \return bool
     */
    bool doesPathContain(std::string word);
    /**
     * \~french
     * \brief Précise si le path finit avec un mot précisé
     * \param word
     * \return bool
     * \~english
     * \brief Precise if the path finish with a word
     * \param word
     * \return bool
     */
    bool doesPathFinishWith(std::string word);
    /**
     * \~french
     * \brief Constructeur d'une requête de type GET
     * \param strquery chaîne de caractères représentant la requête
     * \param hostName nom de domaine déclaré dans la requête
     * \param path chemin du serveur web pour accèder au service
     * \param https requête de type https si défini
     * \~english
     * \brief Get Request Constructor
     * \param strquery http query arguments
     * \param hostName hostname declared in the request
     * \param path webserver path to the ROK4 Server
     * \param https https request if defined
     */
    Request ( char* strquery, char* hostName, char* path, char* https );
    /**
     * \~french
     * \brief Constructeur d'une requête de type POST
     * \param strquery chaîne de caractères représentant la requête
     * \param hostName nom de domaine déclaré dans la requête
     * \param path chemin du serveur web pour accèder au service
     * \param https requête de type https si défini
     * \param postContent contenu envoyé en POST
     * \~english
     * \brief POST Request Constructor
     * \param strquery http query arguments
     * \param hostName hostname declared in the request
     * \param path webserver path to the ROK4 Server
     * \param https https request if defined
     * \param postContent the http POST Content
     */
    Request ( char* strquery, char* hostName, char* path, char* https, std::string postContent );
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Request();
};

#endif /* REQUEST_H_ */
