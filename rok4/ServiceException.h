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
 * \file ServiceException.h
 * \~french
 * \brief Définition de la gestion des exceptions des services
 * \~english
 * \brief Define services exceptions
 */

#ifndef SERVICE_EXCEPTION_H
#define SERVICE_EXCEPTION_H

#include <string>
#include <map>


/**
 * \~french
 * \brief Codes d'exceptions definis par les specifications WMS, OWS et HTTP
 * \~english
 * \brief Exception codes defined by WMS, OWS & HTTP specifications
 */
typedef enum {
    /**
     * \~french L'operation demandés nécessite un paramètre supplémentaire, et celui-ci ne possède pas de valeur par défaut
     * \~english Operation request does not include a parameter value, and this server did not declare a default value for that parameter
     */
    OWS_MISSING_PARAMETER_VALUE = 0,
    /**
     * \~french L'opération demandée inclut un paramètre avec une valeur invalide
     * \~english Operation request includes a parameter with an invalid value
     */
    OWS_INVALID_PARAMETER_VALUE = 1,
    /**
     * \~french La liste des versions dans le paramètre "AcceptVersions", de la requête "GetCapabilities", n'inclut pas de valeur supportée par ce serveur
     * \~english List of versions in “AcceptVersions” parameter value, in GetCapabilities operation request, did not include any version supported by this server.
     */
    OWS_VERSION_NEGOTIATION_FAILED= 2,
    /**
     * \~french La valeur du paramètre (optionel) "updateSequence", de la requête "GetCapabilities", est plus grand que la valeur du service
     * \~english Value of (optional) updateSequence parameter, in GetCapabilities operation request, is greater parameter than current value of service metadata updateSequence number
     */
    OWS_INVALID_UPDATESEQUENCE= 3,
    /**
     * \~french Pas d'autre code spécifié par ce service ne s'applique à cette exception
     * \~english No other exceptionCode specified by this service and server applies to this exception
     */
    OWS_NOAPPLICABLE_CODE= 4,
    /**
     * \~french La requête demande un format non supporté par ce serveur
     * \~english Request contains a Format not offered by the server
     */
    WMS_INVALID_FORMAT= 5,
    /**
     * \~french La requête demande un CRS non supporté par ce serveur pour au moins l'une des couches de la requête
     * \~english Request contains a CRS not offered by the server for one or more of the Layers in the request
     */
    WMS_INVALID_CRS= 6,
    /**
     * \~french La requête "GetMap" porte sur une couche non disponible sur ce serveur, ou la requête "GetFeatureInfo" porte sur une couche non visible sur la carte
     * \~english GetMap request is for a Layer not offered by the server, or GetFeatureInfo request is for a Layer not shown on the map
     */
    WMS_LAYER_NOT_DEFINED= 7,
    /**
     * \~french La requête porte sur une couche avec un Style non défini par ce serveur
     * \~english Request is for a Layer in a Style not offered by the server
     */
    WMS_STYLE_NOT_DEFINED= 8,
    /**
     * \~french La requête "GetFeatureInfo" porte sur une couche non requêtable
     * \~english GetFeatureInfo request is applied to a Layer which is not declared queryable
     */
    WMS_LAYER_NOT_QUERYABLE= 9,
    /**
     * \~french La requête "GetFeatureInfo" contient des coordonnées invalides
     * \~english GetFeatureInfo request contains invalid X or Y value
     */
    WMS_INVALID_POINT= 10,
    /**
     * \~french La valeur du paramètre (optionel) "updateSequence", de la requête "GetCapabilities", est plus grand que la valeur du service
     * \~english Value of (optional) UpdateSequence parameter in GetCapabilities request is equal to current value of service metadata update sequence number
     */
    WMS_CURRENT_UPDATESEQUENCE= 11,
    /**
     * \~french La requête ne contient pas une valeur pour le paramètre "dimension", et le serveur ne définit pas de valeur par défaut pour celui-ci
     * \~english Request does not include a sample dimension value, and the server did not declare a default value for that dimension
     */
    WMS_MISSING_DIMENSION_VALUE= 12,
    /**
     * \~french Le paramètre "dimension" de la requête contient une valeur invalide
     * \~english Request contains an invalid sample dimension value
     */
    WMS_INVALID_DIMENSION_VALUE= 13,
    /**
     * \~french La requête porte sur une opération optionnelle non supportée par ce serveur
     * \~english Request is for an optional operation that is not supported by the server
     */
    OWS_OPERATION_NOT_SUPORTED= 14,
    /**
     * \~french  L'indice de colonne ou de ligne est en dehors de sa zone de définition
     * \~english TileRow or TileCol out of range
     */
    WMTS_TILE_OUT_OF_RANGE= 15,
    /**
     * \~french Implémentation de l'erreur HTTP 404
     * \~english HTTP 404 implementation
     */
    HTTP_NOT_FOUND = 16

} ExceptionCode;


/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Gère les informations permettant de générer la partie du message d'erreur XML suivante:
 * \brief Gestion des exceptions de service
 * \~english
 * Handle information used to generate the next part of the XML error message :
 * \brief Handle service exception
 * \~ \details \code{.xml}
 * <ServiceException code='__CODE__' locator='__LOCATOR__' >
 *              __MESSAGE__
 * </ServiceException>
 * \endcode
 */
class ServiceException {
private:
    /**
     * \~french \brief La valeur de l'attribut "locator" selon les specifications OGC
     * \~english \brief The "locator" attribute value as define by OGC specifications
     */
    std::string locator;
    /**
     * \~french \brief La valeur de l'attribut "code" selon les specifications OGC
     * \~english \brief The "code" attribute value as define by OGC specifications
     */
    ExceptionCode code ;
    /**
     * \~french \brief Le texte du message d'erreur
     * \~english \brief The error message
     */
    std::string message;
    /**
     * \~french \brief Le type de service ("wms" ou "wmts")
     * \~english \brief The service type ("wms" or "wmts")
     */
    std::string service;
//      static void _init() ;
//      static std::map<ExceptionCode, std::string> _errCodes ;

public:
    /**
     * \~french
     * constructeur d'une exception
     * \param[in] locator la valeur de l'attribut locator (selon specs OGC)
     * \param[in] code la valeur de l'attribut code (selon specs OGC)
     * \param[in] message le texte du message d'erreur
     * \param[in] service le type de service ("wms" ou "wmts")
     * \~english
     * constructeur d'une exception
     * \param[in] locator the "locator" attribute value as define by OGC specifications
     * \param[in] code the "code" attribute value as define by OGC specifications
     * \param[in] message the error message
     * \param[in] service the service type ("wms" or "wmts")
     */
    ServiceException ( std::string locator, ExceptionCode code, std::string message, std::string service ) ;

    /**
     * \~french
     * \brief Retourne le code d'erreur sous forme de texte.
     * \param[in] code le code d'erreur énuméré
     * \return réprésentation textuelle
     * \~english
     * \brief Return the string representation of the error code
     * \param[in] code the error code
     * \return text representation
     */
    static std::string getCodeAsString ( ExceptionCode code );

    /**
     * \~french
     * \brief Retourne le status code HTTP associe à l'exception.
     * \param[in] code le code d'erreur énuméré
     * \return code de status HTTP
     * \~english
     * \brief Return the style's identifier
     * \param[in] code the error code
     * \return HTTP status code
     */
    static int getCodeAsStatusCode ( ExceptionCode code );

    /**
     * \~french
     * \brief Retourne la phrase explicative associée au status code http
     * \param[in] statusCode code de status HTTP
     * \return phrase explicative
     * \~english
     * \brief Return the reason phrase associated with the http status code
     * \param[in] statusCode HTTP status code
     * \return reason phrase
     */
    static std::string getStatusCodeAsReasonPhrase ( int statusCode );

    /**
     * \~french
     * \brief Génère la chaîne de caracteres relative à l'exception décrite par l'objet
     * \return représentation textuelle de l'exception
     * \~english
     * \brief Generate the string representation of the exception
     * \return string representation
     */
    std::string toString();

    /**
     * \~french
     * \brief Retourne le nom du service
     * \return service
     * \~english
     * \brief Return the service name
     * \return service
     */
    std::string getService() {
        return this->service ;
    } ;

    /**
     * \~french
     * \brief Retourne le code de l'erreur
     * \return code
     * \~english
     * \brief Return the error code
     * \return code
     */
    ExceptionCode getCode() {
        return this->code;
    } ;
};


#endif // SERVICE_EXCEPTION_H
