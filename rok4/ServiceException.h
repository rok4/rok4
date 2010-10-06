#ifndef SERVICE_EXCEPTION_H
#define SERVICE_EXCEPTION_H

#include <string>
#include <map>


/**
 * Codes d'exceptions definis par les specs WMS et OWS (reprise texto par WMTS)
 */
typedef enum {
	OWS_MISSING_PARAMETER_VALUE = 0, /*!< Request is for an operation that is 
                                        not supported by this server */
	OWS_INVALID_PARAMETER_VALUE = 1, /*!< Operation request does not include a 
                                        parameter value, and this server did 
                                        not declare a default value for that 
                                        parameter */
	OWS_VERSION_NEGOTIATION_FAILED= 2, /*!< List of versions in “AcceptVersions” 
                                          parameter value, in GetCapabilities
                                          operation request, did not include
                                          any version supported by this server.*/
	OWS_INVALID_UPDATESEQUENCE= 3, /*!< Value of (optional) updateSequence 
                                      parameter, in GetCapabilities operation 
                                      request, is greater  parameter than 
                                      current value of service metadata 
                                      updateSequence number */
	OWS_NOAPPLICABLE_CODE= 4,      /*! < No other exceptionCode specified by this 
                                       service and server applies to this 
                                       exception */
	WMS_INVALID_FORMAT= 5,         /*!< Request contains a Format not offered by 
                                      the server.*/
	WMS_INVALID_CRS= 6,            /*!< Request contains a CRS not offered by the 
                                    server for one or more of the Layers in 
                                    the request. */
	WMS_LAYER_NOT_DEFINED= 7,      /*!< GetMap request is for a Layer not offered 
                                    by the server, or GetFeatureInfo request is 
                                    for a Layer not shown on the map. */
	WMS_STYLE_NOT_DEFINED= 8,      /*!< Request is for a Layer in a Style not 
                                    offered by the server. */
	WMS_LAYER_NOT_QUERYABLE= 9,      /*!< GetFeatureInfo request is applied to a 
                                        Layer which is not declared queryable. */
	WMS_INVALID_POINT= 10,           /*!< GetFeatureInfo request contains invalid 
                                        X or Y value. */
	WMS_CURRENT_UPDATESEQUENCE= 11,  /*!< Value of (optional) UpdateSequence 
                                        parameter in GetCapabilities request is 
                                        equal to current value of service 
                                        metadata update sequence number.*/
	WMS_MISSING_DIMENSION_VALUE= 12, /*<! Request does not include a sample
                                        dimension value, and the server did not
                                        declare a default value for that 
                                        dimension.*/
	WMS_INVALID_DIMENSION_VALUE= 13, /*!< Request contains an invalid sample 
                                        dimension value. */
	WMS_OPERATION_NOT_SUPORTED= 14,  /*!< Request is for an optional operation 
                                        that is not supported by the server.*/
	WMTS_TILE_OUT_OF_RANGE= 15       /*!< TileRow or TileCol out of range */

} ExceptionCode;


/**
 * @class ServiceException
 * Gere les informations permettant de generer la partie du message d'erreur XML suivante:
 * <ServiceException code='__CODE__' locator='__LOCATOR__' >
 * 		__MESSAGE__
 * </ServiceException>
 *
 */
class ServiceException {
private:
	std::string locator;
	std::string code ;
	std::string message;
	std::string service;
//	static void _init() ;
//	static std::map<ExceptionCode, std::string> _errCodes ;

public:
/**
	* constructeur d'une exception
	* @param locator la valeur de l'attribut locator (selon specs OGC)
	* @param code la valeur de l'attribut code (selon specs OGC)
	* @param message le texte du message d'erreur
	* @param service le type de service ("wms" ou "wmts")
	*/
	ServiceException(std::string locator, ExceptionCode code, std::string message, std::string service) ;

	/**
	 * Retourne le code d'erreur sous forme de string.
	 * @param code le code d'erreur enumere
	 */
	static std::string getCodeAsString(ExceptionCode code);

	/**
	 * Genere la chaine de caracteres relative a l'exception decrite par l'objet
	 */
	std::string toString();
	/**
		* acces au champ service
		*/
	std::string getService() {return this->service ;} ;
};


#endif // SERVICE_EXCEPTION_H
