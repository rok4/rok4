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
#include "Layer.h"


class CppUnitLayer : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitLayer );
    CPPUNIT_TEST ( getId );
    CPPUNIT_TEST ( getTitle );
    CPPUNIT_TEST ( getAbstract );
    CPPUNIT_TEST ( getKeyWords );
    CPPUNIT_TEST ( getStyles );
    CPPUNIT_TEST ( getMinRes );
    CPPUNIT_TEST ( getMaxRes );
    CPPUNIT_TEST ( getOpaque );
    CPPUNIT_TEST ( getAuthority );
    CPPUNIT_TEST_SUITE_END();

protected:
    std::vector<Keyword> keyWords;
    // One style
    std::string id0;
    std::vector<std::string> titles0;
    std::vector<std::string> abstracts0;
    bool WMSAuthorized = true;
    bool WMTSAuthorized = true;
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

    bool getFeatureInfoAvailability;
    std::string getFeatureInfoType;
    std::string getFeatureInfoBaseURL;
    std::string GFIVersion;
    std::string GFIService;
    std::string GFIQueryLayers;
    std::string GFILayers;
    bool GFIForceEPSG;
    
    Palette* palette0;
    Style* style;
    Layer* layer;

public:
    void setUp();
    void tearDown();

protected:
    void getId();
    void getTitle();
    void getAbstract();
    void getWMSAuthorized();
    void getWMTSAuthorized();
    void getKeyWords();
    void getStyles();
    void getMinRes();
    void getMaxRes();
    void getOpaque();
    void getAuthority();
};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitLayer );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitLayer, "CppUnitLayer" );

void CppUnitLayer::setUp() {
    // Prepare the layer
    id0 = "zero";
    titles0.push_back("title0");
    abstracts0.push_back("abstracts0");
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
    Pente pente;
    Aspect aspect;
    Estompage estompage;
    style = new Style ( id0,titles0,abstracts0,keyWords,legendURLs0,*palette0,pente,aspect,estompage  );
    styleslayer.push_back(style);
    idlayer = "Id of the layer";
    titlelayer = "Title of the layer";
    abstractlayer = "This is the abstract of the layer";
    minReslayer = 102.4;
    maxReslayer = 209715.2;
    opaquelayer = true;
    authoritylayer = "IGNF";
    
    getFeatureInfoAvailability = false;
    getFeatureInfoType = "";
    getFeatureInfoBaseURL = "";
    GFIVersion = "";
    GFIService = "";
    GFIQueryLayers = "";
    GFILayers = "";
    GFIForceEPSG = true;
    
    layer = new Layer ( idlayer, titlelayer, abstractlayer, WMSAuthorized, WMTSAuthorized,keyWords, dataPyramidlayer, styleslayer, minReslayer, maxReslayer, WMSCRSListlayer, opaquelayer, authoritylayer, resamplinglayer, geographicBoundingBoxlayer, boundingBoxlayer, metadataURLslayer, getFeatureInfoAvailability, getFeatureInfoType, getFeatureInfoBaseURL, GFIVersion, GFIService, GFIQueryLayers, GFILayers, GFIForceEPSG );
}

void CppUnitLayer::getId() {
    CPPUNIT_ASSERT_MESSAGE ( "layer getId:\n", layer->getId() == "Id of the layer" ) ;
}

void CppUnitLayer::getTitle() {
    CPPUNIT_ASSERT_MESSAGE ( "layer getTitle:\n", layer->getTitle() == "Title of the layer" ) ;
}

void CppUnitLayer::getAbstract() {
    CPPUNIT_ASSERT_MESSAGE ( "layer getAbstract\n", layer->getAbstract() == "This is the abstract of the layer" ) ;
}

void CppUnitLayer::getKeyWords() {
    CPPUNIT_ASSERT_MESSAGE ( "layer number of keywords:\n", layer->getKeyWords()->size() == 2 ) ;
    CPPUNIT_ASSERT_MESSAGE ( "layer content of keywords:\n", layer->getKeyWords()->at(1).getContent() == "10cm" ) ;
}

void CppUnitLayer::getStyles() {
    CPPUNIT_ASSERT_MESSAGE ( "layer content of styles:\n", layer->getStyles() [0]->getId() == "zero" ) ;
}

void CppUnitLayer::getMinRes() {
    CPPUNIT_ASSERT_MESSAGE ( "layer getMinRes:\n", layer->getMinRes() == 102.4 ) ;
}

void CppUnitLayer::getMaxRes() {
    CPPUNIT_ASSERT_MESSAGE ( "layer getMaxRes:\n", layer->getMaxRes() == 209715.2 ) ;
}

void CppUnitLayer::getOpaque() {
    CPPUNIT_ASSERT_MESSAGE ( "layer getOpaque:\n", layer->getOpaque() == true ) ;
}

void CppUnitLayer::getWMSAuthorized() {
    CPPUNIT_ASSERT_MESSAGE ( "layer getWMSAuthorized:\n", layer->getWMSAuthorized() == true ) ;
}

void CppUnitLayer::getWMTSAuthorized() {
    CPPUNIT_ASSERT_MESSAGE ( "layer getWMTSAuthorized:\n", layer->getWMTSAuthorized() == true ) ;
}

void CppUnitLayer::getAuthority() {
    CPPUNIT_ASSERT_MESSAGE ( "layer getAuthority:\n", layer->getAuthority() == "IGNF" ) ;
}

void CppUnitLayer::tearDown() {
    delete style;
    delete legendURL0;
    delete palette0;
    delete layer;
}
