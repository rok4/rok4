#include <map>

#include "ServiceException.h"
#include "Logger.h"


/*
 std::map<ExceptionCode, std::string> ServiceException::_errCodes= std::map<ExceptionCode, std::string>() ;


void ServiceException::_init() {
	ServiceException::_errCodes[OWS_MISSING_PARAMETER_VALUE]= "MissingParameterValue" ;
	ServiceException::_errCodes[OWS_INVALID_PARAMETER_VALUE]= "InvalidParameterValue" ;
	ServiceException::_errCodes[OWS_VERSION_NEGOTIATION_FAILED]= "VersionNegotiationFailed" ;
	ServiceException::_errCodes[OWS_INVALID_UPDATESEQUENCE]= "InvalidUpdateSequence" ;
	ServiceException::_errCodes[OWS_NOAPPLICABLE_CODE]= "NoApplicableCode" ;
	ServiceException::_errCodes[WMS_INVALID_FORMAT]= "InvalidFormat" ;
	ServiceException::_errCodes[WMS_INVALID_CRS]= "InvalidCRS" ;
	ServiceException::_errCodes[WMS_LAYER_NOT_DEFINED]= "LayerNotDefined" ;
	ServiceException::_errCodes[WMS_STYLE_NOT_DEFINED]= "StyleNotDefined" ;
	ServiceException::_errCodes[WMS_LAYER_NOT_QUERYABLE]= "LayerNotQueryable" ;
	ServiceException::_errCodes[WMS_INVALID_POINT]= "InvalidPoint" ;
	ServiceException::_errCodes[WMS_CURRENT_UPDATESEQUENCE]= "CurrentUpdateSequence" ;
	ServiceException::_errCodes[WMS_MISSING_DIMENSION_VALUE]= "MissingDimensionValue" ;
	ServiceException::_errCodes[WMS_INVALID_DIMENSION_VALUE]= "InvalidDimensionValue" ;
	ServiceException::_errCodes[WMS_OPERATION_NOT_SUPORTED]= "OperationNotSuported" ;
	ServiceException::_errCodes[WMTS_TILE_OUT_OF_RANGE]= "TileOutOfRange" ;
}

*/

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
		case WMS_OPERATION_NOT_SUPORTED: return "OperationNotSuported" ;
		case WMTS_TILE_OUT_OF_RANGE: return "TileOutOfRange" ;
		default: return "" ;
	}
}


ServiceException::ServiceException(std::string locator, ExceptionCode code, std::string message, std::string service) {
	// if (ServiceException::_errCodes.size()==0) ServiceException::_init() ;
	this->message=message ; 
	this->locator= locator ;
/*	std::map<ExceptionCode, std::string>::iterator it=_errCodes.find(code) ;
	if (it == ServiceException::_errCodes.end()) {
		it= ServiceException::_errCodes.find(OWS_NOAPPLICABLE_CODE) ; // par defaut
	}
	this->code= it->second ;
*/
	this->code= ServiceException::getCodeAsString(code) ;
	if (this->code=="") this->code= ServiceException::getCodeAsString(OWS_NOAPPLICABLE_CODE) ;

	this->service= service ;
} // ServiceException::ServiceException()


/*
 * generation du message d'erreur correspondant à l'exception
 */
std::string ServiceException::toString() {
	std::string exTagName= (this->service=="wms"?"ServiceException":"Exception") ;
	std::string codeAttName= (this->service=="wms"?"code":"exceptionCode") ;

	return "<"+exTagName+" "+codeAttName+"=\""+this->code+"\""+" "+(this->locator==""?"":" locator=\""+this->locator+"\"")+">\n  "+ (this->message==""?"":this->message)+"\n</"+(this->service=="wms"?"ServiceException":"Exception")+">\n" ;

} // ServiceException::toString()
