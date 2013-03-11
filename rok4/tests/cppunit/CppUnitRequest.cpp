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

#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <iostream>

#include <fstream>
#include <vector>
#include "Request.h"
#include "Request.cpp"



class CppUnitRequest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitRequest );
    // enregistrement des methodes de tests à jouer :
    CPPUNIT_TEST ( testhex2int );
    CPPUNIT_TEST ( testsplit );
    CPPUNIT_TEST ( testurl_decode );
    CPPUNIT_TEST ( testtoLowerCase );
    CPPUNIT_TEST ( testremoveNameSpace );
    CPPUNIT_TEST_SUITE_END();

protected:
    unsigned char monhaxedecimal_majuscule;
    Request* marequete;

public:
    // Cette fonction est toujours appelée lors du démarage des tests de ce fichier
    void setUp();

    void tearDown();

protected:
    void testhex2int();
    void testsplit();
    void testurl_decode();
    void testtoLowerCase();
    void testremoveNameSpace();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitRequest );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitRequest, "CppUnitRequest" );

void CppUnitRequest::setUp() {
    std::string strquerystring("www.marequete.com/adresse");
    char* strquery = new char[strquerystring.size()+1];
    memcpy(strquery,strquerystring.c_str(),strquerystring.size()+1);
    std::string hostNamestring("127.0.0.1");
    char* hostName = new char[hostNamestring.size()+1];
    memcpy(hostName,hostNamestring.c_str(),hostNamestring.size()+1);
    std::string pathNamestring("/chemin/chemin2");
    char* path = new char[pathNamestring.size()+1];
    memcpy(path,pathNamestring.c_str(),pathNamestring.size()+1);
    std::string httpsstring("https://");
    char* https = new char[httpsstring.size()+1];
    memcpy(https,httpsstring.c_str(),httpsstring.size()+1);
    marequete = new Request(strquery,hostName,path,https);
    
    monhaxedecimal_majuscule = 'E';
 }

void CppUnitRequest::testhex2int() {
    CPPUNIT_ASSERT_MESSAGE ( "conversion hex2int :\n", hex2int('0') == 0 ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion hex2int :\n", hex2int('1') == 1 ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion hex2int :\n", hex2int('1') != 0 ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion hex2int :\n", hex2int('f') == 15 ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion hex2int :\n", hex2int(monhaxedecimal_majuscule) == 14 ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion hex2int :\n", hex2int('G') != 16  ) ;
}

void CppUnitRequest::testsplit() {
    CPPUNIT_ASSERT_MESSAGE ( "split string :\n", split("salut les poulets",' ').size() == 3 ) ;
    CPPUNIT_ASSERT_MESSAGE ( "split string :\n", split("salut les poulets",' ')[0] == "salut" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "split string :\n", split("salut les poulets",'l').size() == 4 ) ;
    CPPUNIT_ASSERT_MESSAGE ( "split string :\n", split("salut les poulets",'l')[0] == "sa" ) ;
}

void CppUnitRequest::testurl_decode() {
    std::string mystring("Hello");
    char* myword = new char[mystring.size()+1];
    memcpy(myword,mystring.c_str(),mystring.size()+1);
    marequete->url_decode(myword);
    CPPUNIT_ASSERT_MESSAGE ( "use url_decode :\n", mystring.compare(myword) == 0 ) ;
    delete myword;
}

void CppUnitRequest::testtoLowerCase() {
    std::string mystring("TEST");
    char* myword = new char[mystring.size()+1];
    memcpy(myword,mystring.c_str(),mystring.size()+1);
    toLowerCase(myword);
    std::string mystring2("TeSt");
    char* myword2 = new char[mystring2.size()+1];
    memcpy(myword2,mystring2.c_str(),mystring2.size()+1);
    toLowerCase(myword2);
    std::string mystring3("test");
    char* myword3 = new char[mystring3.size()+1];
    memcpy(myword3,mystring3.c_str(),mystring3.size()+1);
    toLowerCase(myword3);
    std::string mystringresult("test");
    CPPUNIT_ASSERT_MESSAGE ( "conversion toLowerCase :\n", myword == mystringresult ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion toLowerCase :\n", myword2 == mystringresult ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion toLowerCase :\n", myword3 == mystringresult ) ;
    std::string mystring4("");
    char* myword4 = new char[mystring4.size()+1];
    memcpy(myword4,mystring4.c_str(),mystring4.size()+1);
    toLowerCase(myword4);
    std::string mystringresult2("");
    CPPUNIT_ASSERT_MESSAGE ( "conversion toLowerCase :\n", myword4 == mystringresult2 ) ;
    delete myword;
    delete myword2;
    delete myword3;
    delete myword4;
}

void CppUnitRequest::testremoveNameSpace() {
    std::string balise("");
    std::string result("");
    result = balise;
    removeNameSpace(result);
    CPPUNIT_ASSERT_MESSAGE ( "remove namespace :\n", result == "" ) ;
    balise = "oneelement";
    result = balise;
    removeNameSpace(result);
    CPPUNIT_ASSERT_MESSAGE ( "remove namespace :\n", result == "oneelement" ) ;
    balise = "oneelement:";
    result = balise;
    removeNameSpace(result);
    CPPUNIT_ASSERT_MESSAGE ( "remove namespace :\n", result == ":" ) ;
    balise = "left:right";
    result = balise;
    removeNameSpace(result);
    CPPUNIT_ASSERT_MESSAGE ( "remove namespace :\n", result == ":right" ) ;
}

void CppUnitRequest::tearDown() {
    delete marequete;
}