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
 * \file ServiceException.cpp
 * \~french
 * \brief Implémentation de la gestion des exceptions des services
 * \~english
 * \brief Implement services exceptions
 */

#include <map>

#include "ServiceException.h"
#include "Logger.h"
#include "config.h"

std::string ServiceException::getCodeAsString ( ExceptionCode code ) {
    switch ( code ) {
    case OWS_MISSING_PARAMETER_VALUE:
        return "MissingParameterValue" ;
    case OWS_INVALID_PARAMETER_VALUE:
        return "InvalidParameterValue" ;
    case OWS_VERSION_NEGOTIATION_FAILED:
        return "VersionNegotiationFailed" ;
    case OWS_INVALID_UPDATESEQUENCE:
        return "InvalidUpdateSequence" ;
    case OWS_NOAPPLICABLE_CODE:
        return "NoApplicableCode" ;
    case WMS_INVALID_FORMAT:
        return "InvalidFormat" ;
    case WMS_INVALID_CRS:
        return "InvalidCRS" ;
    case WMS_LAYER_NOT_DEFINED:
        return "LayerNotDefined" ;
    case WMS_STYLE_NOT_DEFINED:
        return "StyleNotDefined" ;
    case WMS_LAYER_NOT_QUERYABLE:
        return "LayerNotQueryable" ;
    case WMS_INVALID_POINT:
        return "InvalidPoint" ;
    case WMS_CURRENT_UPDATESEQUENCE:
        return "CurrentUpdateSequence" ;
    case WMS_MISSING_DIMENSION_VALUE:
        return "MissingDimensionValue" ;
    case WMS_INVALID_DIMENSION_VALUE:
        return "InvalidDimensionValue" ;
    case OWS_OPERATION_NOT_SUPORTED:
        return "OperationNotSupported" ;
    case WMTS_TILE_OUT_OF_RANGE:
        return "TileOutOfRange" ;
    case HTTP_NOT_FOUND:
        return "Not Found" ;
    default:
        return "" ;
    }
}

int ServiceException::getCodeAsStatusCode ( ExceptionCode code ) {
    switch ( code ) {
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
    case OWS_MISSING_PARAMETER_VALUE:
        return 400 ;
    case OWS_NOAPPLICABLE_CODE:
        return 500 ;
    case OWS_OPERATION_NOT_SUPORTED:
        return 501 ;
    case HTTP_NOT_FOUND:
        return 404 ;
    default:
        return 200 ;
    }
}

std::string ServiceException::getStatusCodeAsReasonPhrase ( int statusCode ) {
    switch ( statusCode ) {
    case 200 :
        return "OK" ;
    case 400 :
        return "BadRequest" ;
    case 404 :
        return "Not Found" ;
    case 500 :
        return "Internal server error" ;
    case 501 :
        return "Not implemented" ;
    default : "No reason"
        ;
    }
}

ServiceException::ServiceException ( std::string locator, ExceptionCode code, std::string message, std::string service ) {
    // if (ServiceException::_errCodes.size()==0) ServiceException::_init() ;
    this->message=message ;
    this->locator= locator ;
    if ( ServiceException::getCodeAsString ( code ) =="" ) {
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
    std::string exTagName= ( this->service=="wms"?"ServiceException":"Exception" ) ;
    std::string codeAttName= ( this->service=="wms"?"code":"exceptionCode" ) ;

    return "<"+exTagName+" "+codeAttName+"=\""+ServiceException::getCodeAsString ( this->code ) +"\""+" "+ ( this->locator==""?"":" locator=\""+this->locator+"\"" ) +">\n  "+ ( this->message==""?"":this->message ) +"\n</"+ ( this->service=="wms"?"ServiceException":"Exception" ) +">\n" ;

} // ServiceException::toString()
