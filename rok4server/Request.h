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

class Request;

#ifndef REQUEST_H_
#define REQUEST_H_

#include <map>
#include <vector>
#include "BoundingBox.h"
#include "Data.h"
#include "CRS.h"
#include "Layer.h"
#include "ServicesXML.h"

/**
 * \file Request.h
 * \~french
 * \brief Définition de la classe Request, analysant les requêtes HTTP
 * \details Définition de la classe Request et des namespaces RequestType et ServiceType
 * \~english
 * \brief Define the Request Class analysing HTTP requests
 * \details Define class Request and namespaces RequestType and ServiceType
 */

namespace RequestType {
    /**
     * \~french \brief Énumération des types de requête
     * \~english \brief Available request type
     */
    enum eRequestType {
        REQUEST_UNKNOWN,
        REQUEST_MISSING,
        GETSERVICES,
        GETCAPABILITIES,
        GETLAYER,
        GETLAYERMETADATA,
        GETMAP,
        GETTILE,
        GETFEATUREINFO,
        GETVERSION
    };


    /**
     * \~french \brief Conversion d'un type de requête vers une chaîne de caractères
     * \param[in] rt type de requête à convertir
     * \return la chaîne de caractère nommant le type de requête
     * \~english \brief Convert a request type to a string
     * \param[in] rt request type to convert
     * \return string namming the request type
     */
    std::string toString ( eRequestType rt );

}


namespace ServiceType {
    /**
     * \~french \brief Énumération des services
     * \~english \brief Available services
     */
    enum eServiceType {
        SERVICE_UNKNOWN,
        SERVICE_MISSING,
        WMTS,
        WMS,
        TMS
    };

    /**
     * \~french \brief Conversion d'un type de service vers une chaîne de caractères
     * \param[in] st type de service à convertir
     * \return la chaîne de caractère nommant le type de service
     * \~english \brief Convert a service type to a string
     * \param[in] st service type to convert
     * \return string namming the service type
     */
    std::string toString ( eServiceType st );
}


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
     * \brief Identification du service et de la requête
     * \~english
     * \brief Service and request type identification
     */
    void determineServiceAndRequest();

public:

    /**
     * \~french
     * \brief Transforme la chaîne de caractères en minuscule
     * \param[in,out] str la chaîne
     * \~english
     * \brief Translate the string to lower case
     * \param[in,out] str the string
     */
    static void toLowerCase ( char* str ) {
        if ( str ) for ( int i = 0; str[i]; i++ ) str[i] = tolower ( str[i] );
    }


    /**
     * \~french
     * \brief Recherche des caractères interdits pour éviter l'injection
     * \param[in] str la chaîne
     * \~english
     * \brief Search forbidden chars to avoid code injection
     * \param[in] str the string
     */
    static bool containForbiddenChars ( std::string str ) {
        const char* forbidden = "<>";
        
        for ( int i = 0; forbidden[i]; i++ )
            if (str.find(forbidden[i]) != std::string::npos)
                return true;

        return false;
    }

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
     * \~french \brief Protocole de la requête (http,https)
     * \~english \brief Request protocol (http,https)
     */
    std::string scheme;
    /**
     * \~french \brief Nom au sens OGC de la requête effectuée
     * \~english \brief OGC request name
     */
    RequestType::eRequestType request;
    /*
     * \~french \brief Type de service (WMS,WMTS,TMS)
     * \~english \brief Service type (WMS,WMTS,TMS)
     */
    ServiceType::eServiceType service;

    /**
     * \~french \brief Liste des paramètres de la requête
     * \~english \brief Request parameters list
     */
    std::map<std::string, std::string> params;

    void print() {
        LOGGER_INFO("hostName = " << hostName);
        LOGGER_INFO("path = " << path);
        LOGGER_INFO("scheme = " << scheme);
        LOGGER_INFO("service = " << ServiceType::toString(service));
        LOGGER_INFO("request = " << RequestType::toString(request));
    }
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
