#include <map>

#include "ServiceException.h"
#include "Logger.h"


std::string ServiceException::getCodeAsString(ExceptionCode code) {
	switch(code) {
		case OWS_MISSING_PARAMETER_VALUE: return "MissingParameterValue" ;
		case OWS_INVALID_PARAMETER_VALUE: return "InvalidParameterValue" ;
		case OWS_VERSION_NEGOTIATION_FAILED: return "VersionNegotiationFailed" ;
		case OWS_INVALID_UPDATESEQUENCE: return "InvalidUpdateSequence" ;
		case OWS_NOAPPLICABLE_CODE: return "NoApplicableCode" ;
		case WMS_INVALID_FORMAT: return "InvalidFormat" ;
		case WMS_INVALID_CRS: return "InvalidCRS" ;
		case WMS_LAYER_NOT_DEFINED: return "LayerNotDefined" ;
		case WMS_STYLE_NOT_DEFINED: return "StyleNotDefined" ;
		case WMS_LAYER_NOT_QUERYABLE: return "LayerNotQueryable" ;
		case WMS_INVALID_POINT: return "InvalidPoint" ;
		case WMS_CURRENT_UPDATESEQUENCE: return "CurrentUpdateSequence" ;
		case WMS_MISSING_DIMENSION_VALUE: return "MissingDimensionValue" ;
		case WMS_INVALID_DIMENSION_VALUE: return "InvalidDimensionValue" ;
		case OWS_OPERATION_NOT_SUPORTED: return "OperationNotSupported" ;
		case WMTS_TILE_OUT_OF_RANGE: return "TileOutOfRange" ;
		default: return "" ;
	}
}

int ServiceException::getCodeAsStatusCode(ExceptionCode code) {
	switch(code) {
		case OWS_INVALID_PARAMETER_VALUE: 
		case WMTS_TILE_OUT_OF_RANGE: 
		case OWS_INVALID_UPDATESEQUENCE: 
		case OWS_VERSION_NEGOTIATION_FAILED:
		case WMS_INVALID_FORMAT: 
		case WMS_INVALID_CRS: 
		case WMS_LAYER_NOT_DEFINED: 
		case WMS_STYLE_NOT_DEFINED: 
		case WMS_LAYER_NOT_QUERYABLE: 
		case WMS_INVALID_POINT: 
		case WMS_CURRENT_UPDATESEQUENCE: 
		case WMS_MISSING_DIMENSION_VALUE: 
		case WMS_INVALID_DIMENSION_VALUE: 
		case OWS_MISSING_PARAMETER_VALUE: return 400 ;
		case OWS_NOAPPLICABLE_CODE: return 500 ;
		case OWS_OPERATION_NOT_SUPORTED: return 501 ;
		default: return 200 ;
	}
}

std::string ServiceException::getStatusCodeAsReasonPhrase(int statusCode) {
	switch(statusCode) {
		case 200 : return "OK" ;
		case 400 : return "BadRequest" ;
		case 500 : return "Internal server error" ;
		case 501 : return "Not implemented" ;
		default : "No reason" ;
	}
}

ServiceException::ServiceException(std::string locator, ExceptionCode code, std::string message, std::string service) {
	// if (ServiceException::_errCodes.size()==0) ServiceException::_init() ;
	this->message=message ; 
	this->locator= locator ;
	if (ServiceException::getCodeAsString(code)=="") {
		this->code= OWS_NOAPPLICABLE_CODE ;
	} else {
		this->code= code ;
	}

	this->service= service ;
} // ServiceException::ServiceException()


/*
 * generation du message d'erreur correspondant à l'exception
 */
std::string ServiceException::toString() {
	std::string exTagName= (this->service=="wms"?"ServiceException":"Exception") ;
	std::string codeAttName= (this->service=="wms"?"code":"exceptionCode") ;

	return "<"+exTagName+" "+codeAttName+"=\""+ServiceException::getCodeAsString(this->code)+"\""+" "+(this->locator==""?"":" locator=\""+this->locator+"\"")+">\n  "+ (this->message==""?"":this->message)+"\n</"+(this->service=="wms"?"ServiceException":"Exception")+">\n" ;

} // ServiceException::toString()
