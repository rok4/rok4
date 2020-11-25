/*
 * Copyright © (2011-2013) Institut national de l'information
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

#define private public // give us access to private methods of the class we test
#include "CapabilitiesBuilder.cpp"
#undef private // stops this crazy hack

#include "Layer.h"
#include "Style.h"


class CppUnitCapabilitiesBuilder : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitCapabilitiesBuilder );
    // enregistrement des methodes de tests à jouer :
    CPPUNIT_TEST ( testnumToStr );
    CPPUNIT_TEST ( testdoubleToStr );
    CPPUNIT_TEST ( testGetDecimalPlaces );
    CPPUNIT_TEST_SUITE_END();

protected:


public:
    void setUp();

    void tearDown();

protected:
    void testnumToStr();
    void testdoubleToStr();
    void testGetDecimalPlaces();

    // For services conf
    std::string name;
    std::string title;
    std::string abstract;
    bool WMSAuthorized;
    bool WMTSAuthorized;
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
    std::vector<std::string> infoFormatList;
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
    bool doweuselistofequalsCRS;
    bool addEqualsCRS;
    bool dowerestrictCRSList;
    std::vector<std::string> listofequalsCRS;
    std::vector<std::string> restrictedCRSList;
    ServicesXML* services_conf;


    // One style
    std::string id0;
    std::vector<std::string> titles0;
    std::vector<std::string> abstracts0;
    LegendURL* legendURL0;
    std::vector<LegendURL> legendURLs0;
    std::map<double, Colour> colours;
    // One layer
    std::string idlayer;
    std::string titlelayer;
    std::string abstractlayer;
    Pyramid* dataPyramidlayer;
    std::vector<Style*> styleslayer;
    double minReslayer;
    double maxReslayer;
    std::vector<CRS> WMSCRSListlayer;
    bool opaquelayer;
    std::string authoritylayer;
    Interpolation::KernelType resamplinglayer;
    GeographicBoundingBoxWMS geographicBoundingBoxlayer;
    BoundingBoxWMS boundingBoxlayer;
    std::vector<MetadataURL> metadataURLslayer;
    std::map<std::string, Layer*> layerlist;

    // One tilematrix
    double res;
    double x0;
    double y0;
    int tileW;
    int tileH;
    long int matrixW;
    long int matrixH;
    TileMatrix* tm;
    std::map<std::string, TileMatrix> mytilematrixlist;
    std::string idset;
    std::string titleset;
    std::string abstractset;
    CRS* crs;
    std::map<std::string, TileMatrixSet*> mytilematrixset;

    // A list of styles
    std::map<std::string, Style*> stylelist;
    
    bool getFeatureInfoAvailability;
    std::string getFeatureInfoType;
    std::string getFeatureInfoBaseURL;
    std::string GFIVersion;
    std::string GFIService;
    std::string GFIQueryLayers;
    std::string GFILayers;
    bool GFIForceEPSG;

    std::string socket;
    int nbThread;
    int nbProcess;
    int backlog;
    bool supportWMS;
    bool supportWMTS;
    Palette* palette0;
    Style* style;
    Layer* layer;
    ContextBook *sbook;
    ContextBook *cbook;
    TileMatrixSet* onematrixset;
    Proxy proxy;
    Rok4Server* myrok4server;

};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitCapabilitiesBuilder );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitCapabilitiesBuilder, "CppUnitCapabilitiesBuilder" );

void CppUnitCapabilitiesBuilder::setUp() {
    // Prepare the Rok4Server test server

    // Load service conf - Rok4Server 2nd argument
    name, title, abstract, serviceProvider, fee, accessConstraint, providerSite, individualName  = "", "", "", "", "", "", "", "";
    individualPosition, voice, facsimile, addressType, deliveryPoint, city, administrativeArea, postCode  = "", "", "", "", "", "", "", "";
    country, electronicMailAddress, postMode, fullStyling, inspire, serviceType, serviceTypeVersion = "", "", false, false, false, "", "3.3.3";
    MetadataURL mtdMWS = MetadataURL ( "simple", metadataUrlWMS,metadataMediaTypeWMS );
    MetadataURL mtdWMTS = MetadataURL ( "simple", metadataUrlWMTS,metadataMediaTypeWMTS );
    services_conf = new ServicesXML ( "" );
    // Load Layerlist - Rok4Server 3rd argument
    id0 = "zero";
    titles0.push_back("title0");
    abstracts0.push_back("abstracts0");
    WMSAuthorized = true;
    WMTSAuthorized = true;
    keyWords.push_back( Keyword ("Lambert93", std::map<std::string, std::string>()));
    keyWords.push_back( Keyword ("10cm", std::map<std::string, std::string>()));
    legendURL0 = new LegendURL ("image/jpeg", "http://ign.fr", 400, 400, 25000, 100000);
    legendURLs0.push_back(*legendURL0);

    srand ( time ( NULL ) );
    for ( int i = 0 ; i < 255; ++i ) {
        colours.insert ( std::pair<double,Colour> ( i,Colour ( 256 * ( rand() / ( RAND_MAX +1.0 ) ),256 * ( rand() / ( RAND_MAX +1.0 ) ),256 * ( rand() / ( RAND_MAX +1.0 ) ),256 * ( rand() / ( RAND_MAX +1.0 ) ) ) ) );
    }
    palette0 = new Palette ( colours,false,false,false );
    palette0->buildPalettePNG();
    style = new Style ( id0,titles0,abstracts0,keyWords,legendURLs0,*palette0 );
    styleslayer.push_back(style);
    
    getFeatureInfoAvailability = false;
    getFeatureInfoType = "";
    getFeatureInfoBaseURL = "";
    GFIVersion = "";
    GFIService = "";
    GFIQueryLayers = "";
    GFILayers = "";
    GFIForceEPSG = true;
    
    layer = new Layer ( idlayer, titlelayer, abstractlayer, WMSAuthorized, WMTSAuthorized, keyWords, dataPyramidlayer, styleslayer, minReslayer, maxReslayer, WMSCRSListlayer, opaquelayer, authoritylayer, resamplinglayer, geographicBoundingBoxlayer, boundingBoxlayer, metadataURLslayer, getFeatureInfoAvailability, getFeatureInfoType, getFeatureInfoBaseURL, GFIVersion, GFIService, GFIQueryLayers, GFILayers, GFIForceEPSG );
    layerlist.insert(std::pair<std::string, Layer*> (layer->getId(), layer) );

    // Load TimeMatrixSet - Rok4Server 4th argument
    res = 209715.2;
    x0 = 0;
    y0 = 12000000;
    tileW = 256;
    tileH = 256;
    matrixW = 1;
    matrixH = 1;
    tm = new TileMatrix("1", res, x0, y0, tileW, tileH, matrixW, matrixH);
    idset = "idset";
    titleset = "titleset";
    abstractset = "abstractset";
    crs = new CRS ("IGNF:LAMB93");
    mytilematrixlist.insert(std::pair<std::string, TileMatrix> (tm->getId(), *tm) );
    onematrixset = new TileMatrixSet(idset, titleset, abstractset, keyWords, *crs, mytilematrixlist);
    mytilematrixset.insert(std::pair<std::string, TileMatrixSet*> (onematrixset->getId(), onematrixset) );
    
    // Load stylelist - Rok4Server 5th argument
    stylelist.insert(std::pair<std::string, Style*> (style->getId(), style) );

    nbThread = 2; // 1st arg
    socket = "127.0.0.1:9000"; // 6th arg
    backlog = 0; // 7th arg
    supportWMS = true; // 9th arg
    supportWMTS = false; // If true -> seg fault for the test 8th arg
    cbook = NULL;
    sbook = NULL;
    nbProcess = 1;
    proxy.proxyName = "";
    proxy.noProxy = "";
    myrok4server = new Rok4Server(nbThread, *services_conf, layerlist, mytilematrixset, stylelist, socket, backlog, cbook,sbook, proxy,supportWMTS, supportWMS, nbProcess);

}

void CppUnitCapabilitiesBuilder::testnumToStr() {
    CPPUNIT_ASSERT_MESSAGE ( "conversion numToStr :\n", numToStr ( 1 ) == "1" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion numToStr :\n", numToStr ( 10000 ) == "10000" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion numToStr :\n", numToStr ( -10000 ) == "-10000" ) ;
}

void CppUnitCapabilitiesBuilder::testdoubleToStr() {
    // TODO: Results of approximations depend on the processor (32-bit, 64-bit...)
    CPPUNIT_ASSERT_MESSAGE ( "conversion doubleToStr :\n", doubleToStr ( 101e-02 ).erase ( 5, std::string::npos ) == "1.010" || doubleToStr ( 101e-02 ).erase ( 5, std::string::npos ) == "1.009" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion doubleToStr :\n", doubleToStr ( 1.001 ).erase ( 6, std::string::npos ) == "1.0010" || doubleToStr ( 1.001 ).erase ( 6, std::string::npos ) == "1.0009" ) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion doubleToStr :\n", doubleToStr ( 100.001 ).erase ( 8, std::string::npos ) == "100.0010" || doubleToStr ( 100.001 ).erase ( 7, std::string::npos ) == "100.0009" ) ;
}

void CppUnitCapabilitiesBuilder::testGetDecimalPlaces() {
    CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(1.0) == 0) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(1.1) == 1) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(1.12) == 2) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(-1.12) == 2) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(1.123) == 3) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(1.1234) == 4) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(1.12345678) == 8) ;
    CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(0.001) == 3) ;
    // See the algorithm is not very robust
    // CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(1.0000000001) == 10) ;
    // CPPUNIT_ASSERT_MESSAGE ( "conversion GetDecimalPlaces :\n", myrok4server->GetDecimalPlaces(1123344.12345678901234) == 9) ;
}

void CppUnitCapabilitiesBuilder::tearDown() {
    delete services_conf;
    delete style;
    delete legendURL0;
    delete palette0;
    delete layer;
    delete tm;
    delete crs;
    delete onematrixset;
    delete myrok4server;
}
