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
#include "ConfLoader.h"
#include "ConfLoader.cpp"


class CppUnitRequest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitRequest );
    // enregistrement des methodes de tests à jouer :
    CPPUNIT_TEST ( testhex2int );
    CPPUNIT_TEST ( testsplit );
    CPPUNIT_TEST ( testurl_decode );
    CPPUNIT_TEST ( testtoLowerCase );
    CPPUNIT_TEST ( testremoveNameSpace );
    CPPUNIT_TEST ( testhasParam );
    CPPUNIT_TEST ( testgetParam );
    CPPUNIT_TEST ( testgetCapWMSParam );
    CPPUNIT_TEST ( testgetCapWMTSParam );
    CPPUNIT_TEST_SUITE_END();

protected:
    unsigned char monhaxedecimal_majuscule;
    // For services conf
    std::string name;
    std::string title;
    std::string abstract;
    std::vector<Keyword> keyWords;
    std::string serviceProvider;
    std::string fee;
    std::string accessConstraint;
    unsigned int layerLimit;
    unsigned int maxWidth;
    unsigned int maxHeight;
    unsigned int maxTileX;
    unsigned int maxTileY;
    bool postMode;
    //Contact Info
    std::string providerSite;
    std::string individualName;
    std::string individualPosition;
    std::string voice;
    std::string facsimile;
    std::string addressType;
    std::string deliveryPoint;
    std::string city;
    std::string administrativeArea;
    std::string postCode;
    std::string country;
    std::string electronicMailAddress;
    //WMS
    std::vector<std::string> formatList;
    std::vector<CRS> globalCRSList;
    bool fullStyling;
    //WMTS
    std::string serviceType;
    std::string serviceTypeVersion;
    //INSPIRE
    bool inspire;
    std::vector<std::string> applicationProfileList;
    std::string metadataUrlWMS;
    std::string metadataMediaTypeWMS;
    std::string metadataUrlWMTS;
    std::string metadataMediaTypeWMTS;
    ServicesConf * services_conf;

public:
    void setUp();

    void tearDown();

protected:
    void testhex2int();
    void testsplit();
    void testurl_decode();
    void testtoLowerCase();
    void testremoveNameSpace();
    void testhasParam();
    void testgetParam();
    void testgetCapWMSParam();
    void testgetCapWMTSParam();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitRequest );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitRequest, "CppUnitRequest" );

void CppUnitRequest::setUp() {
    monhaxedecimal_majuscule = 'E';
    
    // Load service conf
    name = "";
    title = "";
    abstract = "";
    serviceProvider = "";
    fee = "";
    accessConstraint = "";
    providerSite="";
    individualName="";
    individualPosition="";
    voice="";
    facsimile="";
    addressType="";
    deliveryPoint="";
    city="";
    administrativeArea="";
    postCode="";
    country="";
    electronicMailAddress="";
    postMode = false;
    fullStyling = false;
    inspire = false;
    serviceType="";
    serviceTypeVersion="3.3.3";
    MetadataURL mtdMWS = MetadataURL ( "simple", metadataUrlWMS,metadataMediaTypeWMS );
    MetadataURL mtdWMTS = MetadataURL ( "simple", metadataUrlWMTS,metadataMediaTypeWMTS );
    services_conf = new ServicesConf ( name, title, abstract, keyWords,serviceProvider, fee,
                                      accessConstraint, layerLimit, maxWidth, maxHeight, maxTileX, maxTileY, formatList, globalCRSList , serviceType, serviceTypeVersion,
                                      providerSite, individualName, individualPosition, voice, facsimile,
                                      addressType, deliveryPoint, city, administrativeArea, postCode, country,
                                      electronicMailAddress, mtdMWS, mtdWMTS, postMode, fullStyling, inspire );
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
    Request* marequete;
    marequete->url_decode(myword);
    CPPUNIT_ASSERT_MESSAGE ( "use url_decode :\n", mystring.compare(myword) == 0 ) ;
    delete myword;
    delete marequete;
}

void CppUnitRequest::testtoLowerCase() {
    // all upper case
    std::string mystring("TEST");
    char* myword = new char[mystring.size()+1];
    memcpy(myword,mystring.c_str(),mystring.size()+1);
    toLowerCase(myword);
    // mixed upper and lower
    std::string mystring2("TeSt");
    char* myword2 = new char[mystring2.size()+1];
    memcpy(myword2,mystring2.c_str(),mystring2.size()+1);
    toLowerCase(myword2);
    // lower shouldn't change
    std::string mystring3("test");
    char* myword3 = new char[mystring3.size()+1];
    memcpy(myword3,mystring3.c_str(),mystring3.size()+1);
    toLowerCase(myword3);
    std::string mystringresult("test");
    CPPUNIT_ASSERT_MESSAGE ( "conversion toLowerCase :\n", myword == mystringresult ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion toLowerCase :\n", myword2 == mystringresult ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion toLowerCase :\n", myword3 == mystringresult ) ;
    // special case for empty string
    std::string mystring4("");
    char* myword4 = new char[mystring4.size()+1];
    memcpy(myword4,mystring4.c_str(),mystring4.size()+1);
    toLowerCase(myword4);
    std::string mystringresult2("");
    CPPUNIT_ASSERT_MESSAGE ( "conversion toLowerCase :\n", myword4 == mystringresult2 ) ;
    // clean
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

void CppUnitRequest::testhasParam() {
    std::string hostNamestring("127.0.0.1");
    char* hostName = new char[hostNamestring.size()+1];
    memcpy(hostName,hostNamestring.c_str(),hostNamestring.size()+1);
    std::string pathNamestring("/chemin/chemin2");
    char* path = new char[pathNamestring.size()+1];
    memcpy(path,pathNamestring.c_str(),pathNamestring.size()+1);
    std::string httpsstring("https://");
    char* https = new char[httpsstring.size()+1];
    memcpy(https,httpsstring.c_str(),httpsstring.size()+1);

    // param1 is not here
    std::string strquerystring("www.marequete.com/adresse");
    char* strquery = new char[strquerystring.size()+1];
    memcpy(strquery,strquerystring.c_str(),strquerystring.size()+1);
    Request* marequete = new Request(strquery,hostName,path,https);
    CPPUNIT_ASSERT_MESSAGE ( "hasParam :\n", marequete->hasParam("param1") == false ) ;

    // param1 is here
    marequete->params.insert( std::pair<std::string, std::string> ("param1","value1"));
    CPPUNIT_ASSERT_MESSAGE ( "hasParam :\n", marequete->hasParam("param1") == true ) ;
    // but param2 is still not here
    CPPUNIT_ASSERT_MESSAGE ( "hasParam :\n", marequete->hasParam("param2") == false ) ;

    // It also works when a param is inserted multiple times
    marequete->params.insert( std::pair<std::string, std::string> ("param2","value1"));
    marequete->params.insert( std::pair<std::string, std::string> ("param2","value2"));
    marequete->params.insert( std::pair<std::string, std::string> ("param2","value3"));
    marequete->params.insert( std::pair<std::string, std::string> ("param2","value4"));
    CPPUNIT_ASSERT_MESSAGE ( "hasParam :\n", marequete->hasParam("param2") == true ) ;

    // If the param is empty
    CPPUNIT_ASSERT_MESSAGE ( "hasParam :\n", marequete->hasParam("") == false ) ;

    delete hostName;
    delete path;
    delete https;
    delete strquery;
    delete marequete;
}

void CppUnitRequest::testgetParam() {
    std::string hostNamestring("127.0.0.1");
    char* hostName = new char[hostNamestring.size()+1];
    memcpy(hostName,hostNamestring.c_str(),hostNamestring.size()+1);
    std::string pathNamestring("/chemin/chemin2");
    char* path = new char[pathNamestring.size()+1];
    memcpy(path,pathNamestring.c_str(),pathNamestring.size()+1);
    std::string httpsstring("https://");
    char* https = new char[httpsstring.size()+1];
    memcpy(https,httpsstring.c_str(),httpsstring.size()+1);

    // param1 is not here
    std::string strquerystring("www.marequete.com/adresse");
    char* strquery = new char[strquerystring.size()+1];
    memcpy(strquery,strquerystring.c_str(),strquerystring.size()+1);
    Request* marequete = new Request(strquery,hostName,path,https);
    CPPUNIT_ASSERT_MESSAGE ( "getParam :\n", (marequete->getParam("param1")).compare("value1") != 0 ) ;

    // param1 is here
    marequete->params.insert( std::pair<std::string, std::string> ("param1","value1"));
    CPPUNIT_ASSERT_MESSAGE ( "getParam :\n", (marequete->getParam("param1")).compare("value1") == 0 ) ;
    // but param2 is still not here
    CPPUNIT_ASSERT_MESSAGE ( "getParam :\n", (marequete->getParam("param2")).compare("value1") != 0 ) ;

    // It also works when a param is inserted multiple times
    // Note: the first value is returned
    marequete->params.insert( std::pair<std::string, std::string> ("param2","value1"));
    marequete->params.insert( std::pair<std::string, std::string> ("param2","value2"));
    marequete->params.insert( std::pair<std::string, std::string> ("param2","value3"));
    marequete->params.insert( std::pair<std::string, std::string> ("param2","value4"));
    CPPUNIT_ASSERT_MESSAGE ( "getParam :\n", (marequete->getParam("param2")).compare("value1") == 0 ) ;

    // If the param is empty or non existing it returns ""
    CPPUNIT_ASSERT_MESSAGE ( "getParam :\n", (marequete->getParam("")).compare("") == 0 ) ;
    CPPUNIT_ASSERT_MESSAGE ( "getParam :\n", (marequete->getParam("param3")).compare("") == 0 ) ;

    delete hostName;
    delete path;
    delete https;
    delete strquery;
    delete marequete;
}

void CppUnitRequest::testgetCapWMSParam() {
    // Create request
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
    Request* marequete = new Request(strquery,hostName,path,https);
    // Create version, useless arg? for now
    //   This is used as a return value
    std::string versionstring("useless");

    // If the request is not for a WMS service there is an error
    CPPUNIT_ASSERT_MESSAGE ( "getCapWMSParam :\n", (marequete->getCapWMSParam(*services_conf, versionstring)) != NULL ) ;

    // Add the service WMS
    marequete->service = "wms";

    // If there is no version in the param, the version is put to 1.3.0
    //   and no error is returned (NULL is returned)
    CPPUNIT_ASSERT_MESSAGE ( "getCapWMSParam :\n", (marequete->getCapWMSParam(*services_conf, versionstring)) == NULL ) ;
    
    // Add an invalid version param
    //   This time there is a value (the error) returned
    marequete->params.insert( std::pair<std::string, std::string> ("version","1.2.3"));
    CPPUNIT_ASSERT_MESSAGE ( "getCapWMSParam :\n", (marequete->getCapWMSParam(*services_conf, versionstring)) != NULL ) ;

    // We now add a valid version param value
    //   and no error is returned (NULL is returned)
    Request* marequete2 = new Request(strquery,hostName,path,https);
    marequete2->params.insert( std::pair<std::string, std::string> ("version","1.3.0"));
    marequete2->service = "wms";
    CPPUNIT_ASSERT_MESSAGE ( "getCapWMSParam :\n", (marequete2->getCapWMSParam(*services_conf, versionstring)) == NULL ) ;

    delete hostName;
    delete path;
    delete https;
    delete strquery;
    delete marequete;
    delete marequete2;
}

void CppUnitRequest::testgetCapWMTSParam() {
    // Create request
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
    Request* marequete = new Request(strquery,hostName,path,https);
    // This is used as a return value
    std::string versionstring("usefull");

    // If the request is not for a WMS service there is an error
    CPPUNIT_ASSERT_MESSAGE ( "getCapWMTSParam :\n", (marequete->getCapWMTSParam(*services_conf, versionstring)) != NULL ) ;

    // Add the service WMS
    marequete->service = "wmts";

    // If there is no version in the param, the version is put to 1.3.0
    //   and no error is returned (NULL is returned)
    CPPUNIT_ASSERT_MESSAGE ( "getCapWMTSParam :\n", (marequete->getCapWMTSParam(*services_conf, versionstring)) == NULL ) ;
    
    // Add an invalid version param
    //   This time there is a value (the error) returned
    marequete->params.insert( std::pair<std::string, std::string> ("version","1.2.3"));
    CPPUNIT_ASSERT_MESSAGE ( "getCapWMTSParam :\n", (marequete->getCapWMTSParam(*services_conf, versionstring)) != NULL ) ;

    // We now add a valid version param value
    //   and no error is returned (NULL is returned)
    //   The valid param is defined at the begining of this test file (serviceTypeVersion)
    Request* marequete2 = new Request(strquery,hostName,path,https);
    marequete2->params.insert( std::pair<std::string, std::string> ("version","3.3.3"));
    marequete2->service = "wmts";
    CPPUNIT_ASSERT_MESSAGE ( "getCapWMTSParam :\n", (marequete2->getCapWMTSParam(*services_conf, versionstring)) == NULL ) ;

    delete hostName;
    delete path;
    delete https;
    delete strquery;
    delete marequete;
    delete marequete2;
}

void CppUnitRequest::tearDown() {
    delete services_conf;
}