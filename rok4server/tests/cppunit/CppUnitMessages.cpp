/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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

#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <iostream>

#include <fstream>
#include <vector>
#include "Message.h"
#include "ServiceException.h"



class CppUnitMessages : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitMessages );
    // enregistrement des methodes de tests à jouer :
    CPPUNIT_TEST ( test1WMSException );
    CPPUNIT_TEST ( test2WMTSException );
    CPPUNIT_TEST ( test3ExceptionReport );
    CPPUNIT_TEST ( test31ExceptionReport );
    CPPUNIT_TEST ( test4ExceptionReport );
    CPPUNIT_TEST ( test41ExceptionReport );
    CPPUNIT_TEST_SUITE_END();

protected:
    std::string locator;
    ExceptionCode code;
    std::string message;
    ServiceException *wmsSE ;
    ServiceException *wmtsSE ;


public:
    void setUp();

protected:
    void test1WMSException();
    void test2WMTSException();
    void test3ExceptionReport();
    void test31ExceptionReport();
    void test4ExceptionReport();
    void test41ExceptionReport();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitMessages );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitMessages, "CppUnitMessages" );

void CppUnitMessages::setUp() {
    locator= "noLocator" ;
    code= OWS_INVALID_UPDATESEQUENCE ;
    message= "Message d'erreur !!!" ;
    wmsSE= NULL ;
    wmtsSE= NULL ;
    LogOutput logOutput;
    logOutput = STANDARD_OUTPUT_STREAM_FOR_ERRORS;

}

void CppUnitMessages::test1WMSException() {
    // creation d'un objet ServiceException
    ServiceException *se= new ServiceException ( locator,code,message,"wms" ) ;
    if ( se==NULL ) CPPUNIT_FAIL ( "Impossible de créer l'objet ServiceException !" ) ;
    std::string exTxt= se->toString() ;
    if ( exTxt.length() <=0 ) CPPUNIT_FAIL ( "Message de longueur nulle pour l'exception !" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "ServiceException absent du message :\n"+exTxt,exTxt.find ( "ServiceException",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "attribut code absent du message :\n"+exTxt,exTxt.find ( " code=\""+ServiceException::getCodeAsString ( code ) +"\"",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "attribut locator absent du message :\n"+exTxt,exTxt.find ( " locator=\""+locator+"\"",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "texte absent du message :\n"+exTxt,exTxt.find ( message,0 ) !=std::string::npos ) ;
    // TODO : validation du XML
    delete se;
    //wmsSE= se ;
} // test1WMSException


void CppUnitMessages::test2WMTSException() {

    // creation d'un objet ServiceException
    ServiceException *se= new ServiceException ( locator,code,message,"wmts" ) ;
    if ( se==NULL ) CPPUNIT_FAIL ( "Impossible de créer l'objet ServiceException !" ) ;
    std::string exTxt= se->toString() ;
    if ( exTxt.length() <=0 ) CPPUNIT_FAIL ( "Message de longueur nulle pour l'exception !" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "Exception absent du message :\n"+exTxt,exTxt.find ( "<Exception",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "attribut exceptionCode absent du message :\n"+exTxt,exTxt.find ( " exceptionCode=\""+ServiceException::getCodeAsString ( code ) +"\"",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "attribut locator absent du message :\n"+exTxt,exTxt.find ( " locator=\""+locator+"\"",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "texte absent du message :\n"+exTxt,exTxt.find ( message,0 ) !=std::string::npos ) ;
    // TODO : validation du XML
    delete se;

} // test2WMTSException

/**
 * test d'SERDataSource
 */
void CppUnitMessages::test3ExceptionReport() {
    std::vector<ServiceException*> exs ( 1, new ServiceException ( locator,code,message,"wmts" ) ) ;
    exs.push_back ( new ServiceException ( locator,OWS_NOAPPLICABLE_CODE,"Autre message!!","wmts" ) ) ;
    SERDataSource *serDSC= new SERDataSource ( &exs ) ;

    if ( serDSC==NULL ) CPPUNIT_FAIL ( "Impossible de créer l'objet SERDataSource !" ) ;
    std::string exTxt= serDSC->getMessage() ;
    if ( exTxt.length() <=0 ) CPPUNIT_FAIL ( "Message de longueur nulle pour le rapport d'exception !" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "attribut code absent du message :\n"+exTxt,exTxt.find ( " exceptionCode=\""+ServiceException::getCodeAsString ( OWS_NOAPPLICABLE_CODE ) +"\"",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "Exception absent du message :\n"+exTxt,exTxt.find ( "<Exception ",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "ExceptionReport absent du message :\n"+exTxt,exTxt.find ( "<ExceptionReport ",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "attribut xmlns absent du message ou incorrect (xmlns=\"http://opengis.net/ows/1.1\" attendu) :\n"+exTxt,exTxt.find ( "xmlns=\"http://www.opengis.net/ows/1.1\"",0 ) !=std::string::npos ) ;
    // TODO : validation du XML

    delete serDSC ;
    exs.clear() ;
} // test3ExceptionReport

/**
 * test d'SERDataSource
 */
void CppUnitMessages::test31ExceptionReport() {
    SERDataSource *serDSC= new SERDataSource ( new ServiceException ( locator,code,message,"wmts" ) ) ;

    if ( serDSC==NULL ) CPPUNIT_FAIL ( "Impossible de créer l'objet SERDataSource !" ) ;
    std::string exTxt= serDSC->getMessage() ;
    if ( exTxt.length() <=0 ) CPPUNIT_FAIL ( "Message de longueur nulle pour le rapport d'exception !" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "Exception absent du message :\n"+exTxt,exTxt.find ( "<Exception ",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "ExceptionReport absent du message :\n"+exTxt,exTxt.find ( "<ExceptionReport ",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "attribut xmlns absent du message ou incorrect (xmlns=\"http://opengis.net/ows/1.1\" attendu) :\n"+exTxt,exTxt.find ( "xmlns=\"http://www.opengis.net/ows/1.1\"",0 ) !=std::string::npos ) ;
    // TODO : validation du XML

    delete serDSC ;
} // test3ExceptionReport


/**
 * test d'1 SERDataStream
 */
void CppUnitMessages::test4ExceptionReport() {
    std::vector<ServiceException*> exs ( 1,new ServiceException ( locator,code,message,"wms" ) ) ;
    SERDataStream *serDST= new SERDataStream ( &exs ) ;

    if ( serDST==NULL ) CPPUNIT_FAIL ( "Impossible de créer l'objet SERDataStream !" ) ;
    std::string exTxt= serDST->getMessage() ;
    if ( exTxt.length() <=0 ) CPPUNIT_FAIL ( "Message de longueur nulle pour le rapport d'exception !" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "ServiceException absent du message :\n"+exTxt,exTxt.find ( "<ServiceException ",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "ServiceExceptionReport absent du message :\n"+exTxt,exTxt.find ( "<ServiceExceptionReport ",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "attribut xmlns absent du message ou incorrect (xmlns=\"http://opengis.net/ogc\" attendu) :\n"+exTxt,exTxt.find ( "xmlns=\"http://www.opengis.net/ogc\"",0 ) !=std::string::npos ) ;
    // TODO : validation du XML
    exs.clear() ;
    delete serDST;

} // test4ExceptionReport

/**
 * test d'1 SERDataStream
 */
void CppUnitMessages::test41ExceptionReport() {
    ServiceException * se = new ServiceException ( locator,code,message,"wms" );
    SERDataStream *serDST= new SERDataStream ( se ) ;

    if ( serDST==NULL ) CPPUNIT_FAIL ( "Impossible de créer l'objet SERDataStream !" ) ;
    std::string exTxt= serDST->getMessage() ;
    if ( exTxt.length() <=0 ) CPPUNIT_FAIL ( "Message de longueur nulle pour le rapport d'exception !" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "ServiceException absent du message :\n"+exTxt,exTxt.find ( "<ServiceException ",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "ServiceExceptionReport absent du message :\n"+exTxt,exTxt.find ( "<ServiceExceptionReport ",0 ) !=std::string::npos ) ;
    CPPUNIT_ASSERT_MESSAGE ( "attribut xmlns absent du message ou incorrect (xmlns=\"http://opengis.net/ogc\" attendu) :\n"+exTxt,exTxt.find ( "xmlns=\"http://www.opengis.net/ogc\"",0 ) !=std::string::npos ) ;
    // TODO : validation du XML
    delete serDST;

} // test41ExceptionReport

