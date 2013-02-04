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

#include "Message.h"
#include "Logger.h"
#include <iostream>



inline std::string tagName ( std::string t ) {
    return ( t=="wms"?"ServiceExceptionReport":"ExceptionReport" ) ;
}

inline std::string xmlnsUri ( std::string t ) {
    return ( t=="wms"?"http://www.opengis.net/ogc":"http://www.opengis.net/ows/1.1" ) ;
}

/**
 * Methode commune pour generer un Rapport d'exception.
 * @param sex une exception de service
 */
std::string genSER ( ServiceException *sex ) {

    LOGGER_DEBUG ( "service=["<<sex->getService() <<"]" ) ;

    std::string msg= "<"+tagName ( sex->getService() ) +" xmlns=\""+xmlnsUri ( sex->getService() ) +"\">\n" ;
    msg+= sex->toString() ;
    msg+= "</"+tagName ( sex->getService() ) +">" ;

    LOGGER_DEBUG ( "SERVICE EXCEPTION : " << msg );

    return msg ;
}

/**
 * Methode commune pour generer un Rapport d'exception.
 * @param sexcp un vecteur d'exception
 */
std::string genSER ( std::vector<ServiceException*> *sexcp ) {

   // LOGGER_DEBUG ( "sexcp:"<< sexcp << " - size: "<< sexcp->size() << " - [1] : "<< ( *sexcp ) [1] << " - [0] : "<< ( *sexcp ) [0] ) ;
   // LOGGER_DEBUG ( "[0].getService=["<<sexcp->at ( 0 )->getService() <<"]" ) ;

    //this->_init(&(sexcp->at(0)->getService())) ;

    std::string msg= "<"+tagName ( sexcp->at ( 0 )->getService() ) +" xmlns=\""+xmlnsUri ( sexcp->at ( 0 )->getService() ) +"\">\n" ;
    int i ;
    for ( i=0 ; i< sexcp->size() ; i++ ) {
        if ( sexcp->at ( i ) ==NULL ) continue ;
//              ServiceException *se= (*sexcp)[i] ;
        msg+= sexcp->at ( i )->toString() ;
    } // i
    msg+= "</"+tagName ( sexcp->at ( 0 )->getService() ) +">" ;

    LOGGER_DEBUG ( "SERVICE EXCEPTION : " << msg );

    return msg ;
}

SERDataSource::SERDataSource ( std::vector<ServiceException*> *sexcp ) : MessageDataSource ( "","text/xml" ) {
    this->message= genSER ( sexcp ) ;
    // le statut http est celui correspondant à la première exception
    this->httpStatus= ServiceException::getCodeAsStatusCode ( sexcp->at ( 0 )->getCode() ) ;
    for(int i = 0; i < sexcp->size(); ++i)
        delete sexcp->at(i);
    
}

SERDataSource::SERDataSource ( ServiceException *sex ) : MessageDataSource ( "","text/xml" ) {
    this->message= genSER ( sex ) ;
    this->httpStatus= ServiceException::getCodeAsStatusCode ( sex->getCode() ) ;
    delete sex;
}

SERDataStream::SERDataStream ( std::vector<ServiceException*> *sexcp ) : MessageDataStream ( "","text/xml" ) {
    this->message= genSER ( sexcp ) ;
    // le statut http est celui correspondant à la première exception
    this->httpStatus= ServiceException::getCodeAsStatusCode ( sexcp->at ( 0 )->getCode() ) ;
    for(int i = 0; i < sexcp->size(); ++i)
        delete sexcp->at(i);
    
}

SERDataStream::SERDataStream ( ServiceException *sex ) : MessageDataStream ( "","text/xml" ) {
    this->message= genSER ( sex ) ;
    this->httpStatus= ServiceException::getCodeAsStatusCode ( sex->getCode() ) ;
    delete sex;
}
