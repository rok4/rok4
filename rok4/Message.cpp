#include "Message.h"
#include "Logger.h"
#include <iostream>


	
inline std::string tagName(std::string t) {
	return (t=="wms"?"ServiceExceptionReport":"ExceptionReport") ;
}

inline std::string xmlnsUri(std::string t) {
	return (t=="wms"?"http://www.opengis.net/ogc":"http://www.opengis.net/ows/1.1") ;
}

/**
 * Methode commune pour generer un Rapport d'exception.
 * @param sex une exception de service
 */
std::string genSER(ServiceException *sex) {
	
	LOGGER_DEBUG("service=["<<sex->getService()<<"]") ;

	std::string msg= "<"+tagName(sex->getService())+" xmlns=\""+xmlnsUri(sex->getService())+"\">\n" ;
	msg+= sex->toString() ;
	msg+= "</"+tagName(sex->getService())+">" ;

  LOGGER_DEBUG("SERVICE EXCEPTION : " << msg);

  return msg ;
}

/**
 * Methode commune pour generer un Rapport d'exception.
 * @param sexcp un vecteur d'exception
 */
std::string genSER(std::vector<ServiceException*> *sexcp) {
	
	LOGGER_DEBUG("sexcp:"<< sexcp << " - size: "<< sexcp->size() << " - [1] : "<< (*sexcp)[1] << " - [0] : "<< (*sexcp)[0]) ;
	LOGGER_DEBUG("[0].getService=["<<sexcp->at(0)->getService()<<"]") ;

	//this->_init(&(sexcp->at(0)->getService())) ;

	std::string msg= "<"+tagName(sexcp->at(0)->getService())+" xmlns=\""+xmlnsUri(sexcp->at(0)->getService())+"\">\n" ;
	int i ;
	for (i=0 ; i< sexcp->size() ; i++) {
		if (sexcp->at(i)==NULL) continue ;
//		ServiceException *se= (*sexcp)[i] ;
		msg+= sexcp->at(i)->toString() ;
	} // i
	msg+= "</"+tagName(sexcp->at(0)->getService())+">" ;

  LOGGER_DEBUG("SERVICE EXCEPTION : " << msg);

  return msg ;
}

SERDataSource::SERDataSource(std::vector<ServiceException*> *sexcp) : MessageDataSource("","text/xml") {
	this->message= genSER(sexcp) ;
}

SERDataSource::SERDataSource(ServiceException *sex) : MessageDataSource("","text/xml") {
	this->message= genSER(sex) ;
}

SERDataStream::SERDataStream(std::vector<ServiceException*> *sexcp) : MessageDataStream("","text/xml") {
	this->message= genSER(sexcp) ;
}

SERDataStream::SERDataStream(ServiceException *sex) : MessageDataStream("","text/xml") {
	this->message= genSER(sex) ;
}
