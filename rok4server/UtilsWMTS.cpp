/*
 * Copyright © (2011-2013) Institut national de l'information
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
 * \file CapabilitiesBuilder.cpp
 * \~french
 * \brief Implémentation des fonctions de générations des GetCapabilities
 * \~english
 * \brief Implement the GetCapabilities generation function
 */

#include "Rok4Server.h"
#include "tinyxml.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include "TileMatrixSet.h"
#include "Pyramid.h"
#include "intl.h"


DataSource* Rok4Server::getTileParamWMTS ( Request* request, Layer*& layer, std::string& str_tileMatrix, int& tileCol, int& tileRow, std::string& format, Style*& style) {
    // VERSION
    std::string version = request->getParam ( "version" );
    if ( version == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre VERSION absent." ),"wmts" ) );
    if ( version.find ( servicesConf->getServiceTypeVersion() ) == std::string::npos )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Valeur du parametre VERSION invalide (doit contenir " ) +servicesConf->getServiceTypeVersion() +_ ( ")" ),"wmts" ) );

    // LAYER
    std::string str_layer = request->getParam ( "layer" );
    if ( str_layer == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre LAYER absent." ),"wmts" ) );
    
    if ( Request::containForbiddenChars(str_layer)) {
        LOGGER_WARN("Forbidden char detected in WMTS layer: " << str_layer);
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Layer inconnu." ),"wmts" ) );
    }

    layer = serverConf->getLayer(str_layer);
    if ( layer == NULL || ! layer->getWMTSAuthorized() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Layer " ) +str_layer+_ ( " inconnu." ),"wmts" ) );


    // TILEMATRIXSET
    std::string str_tms = request->getParam ( "tilematrixset" );
    if ( str_tms == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre TILEMATRIXSET absent." ),"wmts" ) );

    if ( Request::containForbiddenChars(str_tms)) {
        LOGGER_WARN("Forbidden char detected in WMTS tilematrixset: " << str_tms);
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "TileMatrixSet inconnu." ),"wmts" ) );
    }

    TileMatrixSet* tms = serverConf->getTMS(str_tms);
    if ( tms == NULL )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "TileMatrixSet " ) +str_tms+_ ( " inconnu." ),"wmts" ) );

    if ( tms->getId() != layer->getDataPyramid()->getTms()->getId() )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre TILEMATRIXSET différent de celui de la couche demandée. TILEMATRIXSET devrait être " ) +layer->getDataPyramid()->getTms()->getId(),"wmts" ) );
    
    // TILEMATRIX
    str_tileMatrix = request->getParam ( "tilematrix" );
    if ( str_tileMatrix == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre TILEMATRIX absent." ),"wmts" ) );

    if ( Request::containForbiddenChars(str_tileMatrix)) {
        LOGGER_WARN("Forbidden char detected in WMTS tilematrix: " << str_tileMatrix);
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "TileMatrix inconnu pour le TileMatrixSet " ) +str_tms,"wmts" ) );
    }

    if ( tms->getTm(str_tileMatrix) == NULL )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "TileMatrix " ) +str_tileMatrix+_ ( " inconnu pour le TileMatrixSet " ) +str_tms,"wmts" ) );

    // TILEROW
    std::string str_TileRow = request->getParam ( "tilerow" );
    if ( str_TileRow == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre TILEROW absent." ),"wmts" ) );
    if ( sscanf ( str_TileRow.c_str(),"%d",&tileRow ) !=1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre TILEROW est incorrecte." ),"wmts" ) );

    // TILECOL
    std::string str_TileCol = request->getParam ( "tilecol" );
    if ( str_TileCol == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre TILECOL absent." ),"wmts" ) );
    if ( sscanf ( str_TileCol.c_str(),"%d",&tileCol ) !=1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre TILECOL est incorrecte." ),"wmts" ) );

    // FORMAT
    format = request->getParam ( "format" );

    LOGGER_DEBUG ( _ ( "format requete : " ) << format << _ ( " format pyramide : " ) << Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) );
    if ( format == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre FORMAT absent." ),"wmts" ) );

    if ( Request::containForbiddenChars(format) ) {
        // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
        LOGGER_WARN("Forbidden char detected in WMTS format: " << format);
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le format n'est pas gere pour la couche " ) +str_layer,"wmts" ) );
    }

    if ( format.compare ( Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) ) !=0 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le format " ) +format+_ ( " n'est pas gere pour la couche " ) +str_layer,"wmts" ) );

    //Style

    std::string str_style = request->getParam ( "style" );
    if ( str_style == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre STYLE absent." ),"wmts" ) );
    // TODO : Nom de style : inspire_common:DEFAULT en mode Inspire sinon default

    if (Rok4Format::isRaster(layer->getDataPyramid()->getFormat())) {
        style = layer->getStyleByIdentifier(str_style);

        if ( ! ( style ) )
            return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le style " ) +str_style+_ ( " n'est pas gere pour la couche " ) +str_layer,"wmts" ) );
    }

    return NULL;
}

// Prepare WMTS GetCapabilities fragments
//   Done only 1 time (during server initialization)
void Rok4Server::buildWMTSCapabilities() {
    std::string pathTag = "]HOSTNAME/PATH[";  ///Tag à remplacer par le chemin complet avant le ?.

    std::map<std::string,TileMatrixSet*> usedTMSList;

    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration ( "1.0", "UTF-8", "" );
    doc.LinkEndChild ( decl );

    TiXmlElement * capabilitiesEl = new TiXmlElement ( "Capabilities" );
    capabilitiesEl->SetAttribute ( "version","1.0.0" );
    // attribut UpdateSequence à ajouter quand on en aura besoin
    capabilitiesEl->SetAttribute ( "xmlns","http://www.opengis.net/wmts/1.0" );
    capabilitiesEl->SetAttribute ( "xmlns:ows","http://www.opengis.net/ows/1.1" );
    capabilitiesEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    capabilitiesEl->SetAttribute ( "xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance" );
    capabilitiesEl->SetAttribute ( "xmlns:gml","http://www.opengis.net/gml" );
    capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd" );
    if ( servicesConf->isInspire() ) {
        capabilitiesEl->SetAttribute ( "xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
        capabilitiesEl->SetAttribute ( "xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0" );
        capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0/inspire_vs_ows_11.xsd" );
    }


    //----------------------------------------------------------------------
    // ServiceIdentification
    //----------------------------------------------------------------------
    TiXmlElement * serviceEl = new TiXmlElement ( "ows:ServiceIdentification" );

    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Title", servicesConf->getTitle() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Abstract", servicesConf->getAbstract() ) );
    //KeywordList
    if ( servicesConf->getKeyWords()->size() != 0 ) {
        TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
        TiXmlElement * kwEl;
        for ( unsigned int i=0; i < servicesConf->getKeyWords()->size(); i++ ) {
            kwEl = new TiXmlElement ( "ows:Keyword" );
            kwEl->LinkEndChild ( new TiXmlText ( servicesConf->getKeyWords()->at ( i ).getContent() ) );
            const std::map<std::string,std::string>* attributes = servicesConf->getKeyWords()->at ( i ).getAttributes();
            for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
            }

            kwlEl->LinkEndChild ( kwEl );
        }
        //kwlEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Keyword", ROK4_INFO ) );
        serviceEl->LinkEndChild ( kwlEl );
    }
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:ServiceType", servicesConf->getServiceType() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:ServiceTypeVersion", servicesConf->getServiceTypeVersion() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Fees", servicesConf->getFee() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:AccessConstraints", servicesConf->getAccessConstraint() ) );


    capabilitiesEl->LinkEndChild ( serviceEl );

    //----------------------------------------------------------------------
    // serviceProvider (facultatif)
    //----------------------------------------------------------------------
    TiXmlElement * serviceProviderEl = new TiXmlElement ( "ows:ServiceProvider" );

    serviceProviderEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:ProviderName",servicesConf->getServiceProvider() ) );
    TiXmlElement * providerSiteEl = new TiXmlElement ( "ows:ProviderSite" );
    providerSiteEl->SetAttribute ( "xlink:href",servicesConf->getProviderSite() );
    serviceProviderEl->LinkEndChild ( providerSiteEl );

    TiXmlElement * serviceContactEl = new TiXmlElement ( "ows:ServiceContact" );

    serviceContactEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:IndividualName",servicesConf->getIndividualName() ) );
    serviceContactEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:PositionName",servicesConf->getIndividualPosition() ) );

    TiXmlElement * contactInfoEl = new TiXmlElement ( "ows:ContactInfo" );
    TiXmlElement * contactInfoPhoneEl = new TiXmlElement ( "ows:Phone" );

    contactInfoPhoneEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Voice",servicesConf->getVoice() ) );
    contactInfoPhoneEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Facsimile",servicesConf->getFacsimile() ) );

    contactInfoEl->LinkEndChild ( contactInfoPhoneEl );

    TiXmlElement * contactAddressEl = new TiXmlElement ( "ows:Address" );
    //contactAddressEl->LinkEndChild(DocumentXML::buildTextNode("AddressType","type"));
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:DeliveryPoint",servicesConf->getDeliveryPoint() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:City",servicesConf->getCity() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:AdministrativeArea",servicesConf->getAdministrativeArea() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:PostalCode",servicesConf->getPostCode() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Country",servicesConf->getCountry() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:ElectronicMailAddress",servicesConf->getElectronicMailAddress() ) );
    contactInfoEl->LinkEndChild ( contactAddressEl );

    serviceContactEl->LinkEndChild ( contactInfoEl );

    serviceProviderEl->LinkEndChild ( serviceContactEl );
    capabilitiesEl->LinkEndChild ( serviceProviderEl );




    //----------------------------------------------------------------------
    // OperationsMetadata
    //----------------------------------------------------------------------
    TiXmlElement * opMtdEl = new TiXmlElement ( "ows:OperationsMetadata" );
    TiXmlElement * opEl = new TiXmlElement ( "ows:Operation" );
    opEl->SetAttribute ( "name","GetCapabilities" );
    TiXmlElement * dcpEl = new TiXmlElement ( "ows:DCP" );
    TiXmlElement * httpEl = new TiXmlElement ( "ows:HTTP" );
    TiXmlElement * getEl = new TiXmlElement ( "ows:Get" );
    getEl->SetAttribute ( "xlink:href","]HOSTNAME/PATH[" );
    TiXmlElement * constraintEl = new TiXmlElement ( "ows:Constraint" );
    constraintEl->SetAttribute ( "name","GetEncoding" );
    TiXmlElement * allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
    allowedValuesEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    if ( servicesConf->isPostEnabled() ) {
        TiXmlElement * postEl = new TiXmlElement ( "ows:Post" );
        postEl->SetAttribute ( "xlink:href","]HOSTNAME/PATH[" );
        constraintEl = new TiXmlElement ( "ows:Constraint" );
        constraintEl->SetAttribute ( "name","PostEncoding" );
        allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
        allowedValuesEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Value", "XML" ) );
        //TODO Implement SOAP like request
        //allowedValuesEl->LinkEndChild(DocumentXML::buildTextNode("ows:Value", "SOAP"));
        constraintEl->LinkEndChild ( allowedValuesEl );
        postEl->LinkEndChild ( constraintEl );
        httpEl->LinkEndChild ( postEl );
    }
    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );

    opEl = new TiXmlElement ( "ows:Operation" );
    opEl->SetAttribute ( "name","GetTile" );
    dcpEl = new TiXmlElement ( "ows:DCP" );
    httpEl = new TiXmlElement ( "ows:HTTP" );
    getEl = new TiXmlElement ( "ows:Get" );
    getEl->SetAttribute ( "xlink:href","]HOSTNAME/PATH[" );
    constraintEl = new TiXmlElement ( "ows:Constraint" );
    constraintEl->SetAttribute ( "name","GetEncoding" );
    allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
    allowedValuesEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    if ( servicesConf->isPostEnabled() ) {
        TiXmlElement * postEl = new TiXmlElement ( "ows:Post" );
        postEl->SetAttribute ( "xlink:href","]HOSTNAME/PATH[" );
        constraintEl = new TiXmlElement ( "ows:Constraint" );
        constraintEl->SetAttribute ( "name","PostEncoding" );
        allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
        allowedValuesEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Value", "XML" ) );
        //TODO Implement SOAP like request
        //allowedValuesEl->LinkEndChild(DocumentXML::buildTextNode("ows:Value", "SOAP"));
        constraintEl->LinkEndChild ( allowedValuesEl );
        postEl->LinkEndChild ( constraintEl );
        httpEl->LinkEndChild ( postEl );
    }
    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );
    
    opEl = new TiXmlElement ( "ows:Operation" );
    opEl->SetAttribute ( "name","GetFeatureInfo" );
    dcpEl = new TiXmlElement ( "ows:DCP" );
    httpEl = new TiXmlElement ( "ows:HTTP" );
    getEl = new TiXmlElement ( "ows:Get" );
    getEl->SetAttribute ( "xlink:href","]HOSTNAME/PATH[" );
    constraintEl = new TiXmlElement ( "ows:Constraint" );
    constraintEl->SetAttribute ( "name","GetEncoding" );
    allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
    allowedValuesEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    if ( servicesConf->isPostEnabled() ) {
        TiXmlElement * postEl = new TiXmlElement ( "ows:Post" );
        postEl->SetAttribute ( "xlink:href","]HOSTNAME/PATH[" );
        constraintEl = new TiXmlElement ( "ows:Constraint" );
        constraintEl->SetAttribute ( "name","PostEncoding" );
        allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
        allowedValuesEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Value", "XML" ) );
        //TODO Implement SOAP like request
        //allowedValuesEl->LinkEndChild(DocumentXML::buildTextNode("ows:Value", "SOAP"));
        constraintEl->LinkEndChild ( allowedValuesEl );
        postEl->LinkEndChild ( constraintEl );
        httpEl->LinkEndChild ( postEl );
    }
    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );


    // Inspire (extended Capability)
    // TODO : en dur. A mettre dans la configuration du service (prevoir differents profils d'application possibles)
    if ( servicesConf->isInspire() ) {
        TiXmlElement * extendedCapabilititesEl = new TiXmlElement ( "inspire_vs:ExtendedCapabilities" );

        // MetadataURL
        TiXmlElement * metadataUrlEl = new TiXmlElement ( "inspire_common:MetadataUrl" );
        metadataUrlEl->LinkEndChild ( DocumentXML::buildTextNode ( "inspire_common:URL", servicesConf->getWMTSMetadataURL()->getHRef() ) );
        metadataUrlEl->LinkEndChild ( DocumentXML::buildTextNode ( "inspire_common:MediaType", servicesConf->getWMTSMetadataURL()->getType() ) );
        extendedCapabilititesEl->LinkEndChild ( metadataUrlEl );

        // Languages
        TiXmlElement * supportedLanguagesEl = new TiXmlElement ( "inspire_common:SupportedLanguages" );
        TiXmlElement * defaultLanguageEl = new TiXmlElement ( "inspire_common:DefaultLanguage" );
        TiXmlElement * languageEl = new TiXmlElement ( "inspire_common:Language" );
        TiXmlText * lfre = new TiXmlText ( "fre" );
        languageEl->LinkEndChild ( lfre );
        defaultLanguageEl->LinkEndChild ( languageEl );
        supportedLanguagesEl->LinkEndChild ( defaultLanguageEl );
        extendedCapabilititesEl->LinkEndChild ( supportedLanguagesEl );
        // Responselanguage
        TiXmlElement * responseLanguageEl = new TiXmlElement ( "inspire_common:ResponseLanguage" );
        responseLanguageEl->LinkEndChild ( DocumentXML::buildTextNode ( "inspire_common:Language","fre" ) );
        extendedCapabilititesEl->LinkEndChild ( responseLanguageEl );

        opMtdEl->LinkEndChild ( extendedCapabilititesEl );
    }
    capabilitiesEl->LinkEndChild ( opMtdEl );

    //----------------------------------------------------------------------
    // Contents
    //----------------------------------------------------------------------
    TiXmlElement * contentsEl=new TiXmlElement ( "Contents" );

    // Layer
    //------------------------------------------------------------------
    std::map<std::string, Layer*>::iterator itLay ( serverConf->layersList.begin() ), itLayEnd ( serverConf->layersList.end() );
    for ( ; itLay!=itLayEnd; ++itLay ) {
        //Look if the layer is published in WMTS
        if (itLay->second->getWMTSAuthorized()) {
            TiXmlElement * layerEl=new TiXmlElement ( "Layer" );
            Layer* layer = itLay->second;

            layerEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Title", layer->getTitle() ) );
            layerEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Abstract", layer->getAbstract() ) );
            if ( layer->getKeyWords()->size() != 0 ) {
                TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
                TiXmlElement * kwEl;
                for ( unsigned int i=0; i < layer->getKeyWords()->size(); i++ ) {
                    kwEl = new TiXmlElement ( "ows:Keyword" );
                    kwEl->LinkEndChild ( new TiXmlText ( layer->getKeyWords()->at ( i ).getContent() ) );
                    const std::map<std::string,std::string>* attributes = layer->getKeyWords()->at ( i ).getAttributes();
                    for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                        kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
                    }

                    kwlEl->LinkEndChild ( kwEl );
                }
                layerEl->LinkEndChild ( kwlEl );
            }
            //ows:WGS84BoundingBox (0,n)


            TiXmlElement * wgsBBEl = new TiXmlElement ( "ows:WGS84BoundingBox" );
            std::ostringstream os;
            os.str ( "" );
            os<<layer->getGeographicBoundingBox().minx;
            os<<" ";
            os<<layer->getGeographicBoundingBox().miny;
            wgsBBEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:LowerCorner", os.str() ) );
            os.str ( "" );
            os<<layer->getGeographicBoundingBox().maxx;
            os<<" ";
            os<<layer->getGeographicBoundingBox().maxy;
            wgsBBEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:UpperCorner", os.str() ) );
            os.str ( "" );
            layerEl->LinkEndChild ( wgsBBEl );


            layerEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Identifier", layer->getId() ) );

            //Style
            if ( layer->getStyles().size() != 0 ) {
                for ( unsigned int i=0; i < layer->getStyles().size(); i++ ) {
                    TiXmlElement * styleEl= new TiXmlElement ( "Style" );
                    if ( i==0 ) styleEl->SetAttribute ( "isDefault","true" );
                    Style* style = layer->getStyles() [i];
                    int j;
                    for ( j=0 ; j < style->getTitles().size(); ++j ) {
                        LOGGER_DEBUG ( _ ( "Title : " ) << style->getTitles() [j].c_str() );
                        styleEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Title", style->getTitles() [j].c_str() ) );
                    }
                    for ( j=0 ; j < style->getAbstracts().size(); ++j ) {
                        LOGGER_DEBUG ( _ ( "Abstract : " ) << style->getAbstracts() [j].c_str() );
                        styleEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Abstract", style->getAbstracts() [j].c_str() ) );
                    }

                    if ( style->getKeywords()->size() != 0 ) {
                        TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
                        TiXmlElement * kwEl;
                        for ( unsigned int i=0; i < style->getKeywords()->size(); i++ ) {
                            kwEl = new TiXmlElement ( "ows:Keyword" );
                            kwEl->LinkEndChild ( new TiXmlText ( style->getKeywords()->at ( i ).getContent() ) );
                            const std::map<std::string,std::string>* attributes = style->getKeywords()->at ( i ).getAttributes();
                            for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                                kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
                            }

                            kwlEl->LinkEndChild ( kwEl );
                        }
                        //kwlEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Keyword", ROK4_INFO ) );
                        styleEl->LinkEndChild ( kwlEl );
                    }

                    styleEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Identifier", style->getIdentifier() ) );
                    for ( j=0 ; j < style->getLegendURLs().size(); ++j ) {
                        LegendURL legendURL = style->getLegendURLs() [j];
                        TiXmlElement* legendURLEl = new TiXmlElement ( "LegendURL" );
                        legendURLEl->SetAttribute ( "format", legendURL.getFormat() );
                        legendURLEl->SetAttribute ( "xlink:href", legendURL.getHRef() );
                        if ( legendURL.getWidth() !=0 )
                            legendURLEl->SetAttribute ( "width", legendURL.getWidth() );
                        if ( legendURL.getHeight() !=0 )
                            legendURLEl->SetAttribute ( "height", legendURL.getHeight() );
                        if ( legendURL.getMinScaleDenominator() !=0.0 )
                            legendURLEl->SetAttribute ( "minScaleDenominator", legendURL.getMinScaleDenominator() );
                        if ( legendURL.getMaxScaleDenominator() !=0.0 )
                            legendURLEl->SetAttribute ( "maxScaleDenominator", legendURL.getMaxScaleDenominator() );
                        styleEl->LinkEndChild ( legendURLEl );
                    }
                    layerEl->LinkEndChild ( styleEl );
                }
            }

            // Contrainte : 1 layer = 1 pyramide = 1 format
            layerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format",Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) ) );
            if (layer->isGetFeatureInfoAvailable()){
                for ( unsigned int i=0; i<servicesConf->getInfoFormatList()->size(); i++ ) {
                    layerEl->LinkEndChild ( DocumentXML::buildTextNode ( "InfoFormat",servicesConf->getInfoFormatList()->at ( i ) ) );
                }
            }

            /* on suppose qu'on a qu'un TMS par layer parce que si on admet avoir un TMS par pyramide
             *  il faudra contrôler la cohérence entre le format, la projection et le TMS... */
            TiXmlElement * tmsLinkEl = new TiXmlElement ( "TileMatrixSetLink" );
            tmsLinkEl->LinkEndChild ( DocumentXML::buildTextNode ( "TileMatrixSet",layer->getDataPyramid()->getTms()->getId() ) );
            usedTMSList.insert ( std::pair<std::string,TileMatrixSet*> ( layer->getDataPyramid()->getTms()->getId() , layer->getDataPyramid()->getTms()) );
            //tileMatrixSetLimits
            TiXmlElement * tmsLimitsEl = new TiXmlElement ( "TileMatrixSetLimits" );

            // Niveaux
            std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = layer->getDataPyramid()->getOrderedLevels(false);
            for (std::pair<std::string, Level*> element : orderedLevels) {
                Level * level = element.second;
                TiXmlElement * tmLimitsEl = new TiXmlElement ( "TileMatrixLimits" );
                tmLimitsEl->LinkEndChild ( DocumentXML::buildTextNode ( "TileMatrix",level->getTm()->getId() ) );

                tmLimitsEl->LinkEndChild ( DocumentXML::buildTextNode ( "MinTileRow",numToStr ( ( level->getMinTileRow() <0?0:level->getMinTileRow() ) ) ) );
                tmLimitsEl->LinkEndChild ( DocumentXML::buildTextNode ( "MaxTileRow",numToStr ( ( level->getMaxTileRow() <0?level->getTm()->getMatrixW() :level->getMaxTileRow() ) ) ) );
                tmLimitsEl->LinkEndChild ( DocumentXML::buildTextNode ( "MinTileCol",numToStr ( ( level->getMinTileCol() <0?0:level->getMinTileCol() ) ) ) );
                tmLimitsEl->LinkEndChild ( DocumentXML::buildTextNode ( "MaxTileCol",numToStr ( ( level->getMaxTileCol() <0?level->getTm()->getMatrixH() :level->getMaxTileCol() ) ) ) );
                tmsLimitsEl->LinkEndChild ( tmLimitsEl );
            }

            tmsLinkEl->LinkEndChild ( tmsLimitsEl );
            layerEl->LinkEndChild ( tmsLinkEl );
            contentsEl->LinkEndChild ( layerEl );
        }

    }

    // TileMatrixSet
    //--------------------------------------------------------
    std::map<std::string,TileMatrixSet*>::iterator itTms ( usedTMSList.begin() ), itTmsEnd ( usedTMSList.end() );
    for ( ; itTms!=itTmsEnd; ++itTms ) {

        TileMatrixSet* tms = itTms->second;

        TiXmlElement * tmsEl=new TiXmlElement ( "TileMatrixSet" );
        tmsEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Identifier",tms->getId() ) );
        if ( ! ( tms->getTitle().empty() ) ) {
            tmsEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Title", tms->getTitle().c_str() ) );
        }

        if ( ! ( tms->getAbstract().empty() ) ) {
            tmsEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Abstract", tms->getAbstract().c_str() ) );
        }

        if ( tms->getKeyWords()->size() != 0 ) {
            TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
            TiXmlElement * kwEl;
            for ( unsigned int i=0; i < tms->getKeyWords()->size(); i++ ) {
                kwEl = new TiXmlElement ( "ows:Keyword" );
                kwEl->LinkEndChild ( new TiXmlText ( tms->getKeyWords()->at ( i ).getContent() ) );
                const std::map<std::string,std::string>* attributes = tms->getKeyWords()->at ( i ).getAttributes();
                for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                    kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
                }

                kwlEl->LinkEndChild ( kwEl );
            }
            //kwlEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Keyword", ROK4_INFO ) );
            tmsEl->LinkEndChild ( kwlEl );
        }


        tmsEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:SupportedCRS",tms->getCrs().getRequestCode() ) );
        
        // TileMatrix
        std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix> orderedTM = tms->getOrderedTileMatrix(false);
        for (std::pair<std::string, TileMatrix*> element : orderedTM) {
            TileMatrix* tm = element.second;
            TiXmlElement * tmEl = new TiXmlElement ( "TileMatrix" );
            tmEl->LinkEndChild ( DocumentXML::buildTextNode ( "ows:Identifier",tm->getId() ) );
            tmEl->LinkEndChild ( DocumentXML::buildTextNode ( "ScaleDenominator",doubleToStr ( ( long double ) ( tm->getRes() * tms->getCrs().getMetersPerUnit() ) /0.00028 ) ) );
            tmEl->LinkEndChild ( DocumentXML::buildTextNode ( "TopLeftCorner",doubleToStr ( tm->getX0() ) + " " + doubleToStr ( tm->getY0() ) ) );
            tmEl->LinkEndChild ( DocumentXML::buildTextNode ( "TileWidth",numToStr ( tm->getTileW() ) ) );
            tmEl->LinkEndChild ( DocumentXML::buildTextNode ( "TileHeight",numToStr ( tm->getTileH() ) ) );
            tmEl->LinkEndChild ( DocumentXML::buildTextNode ( "MatrixWidth",numToStr ( tm->getMatrixW() ) ) );
            tmEl->LinkEndChild ( DocumentXML::buildTextNode ( "MatrixHeight",numToStr ( tm->getMatrixH() ) ) );
            tmsEl->LinkEndChild ( tmEl );
        }

        contentsEl->LinkEndChild ( tmsEl );
    }

    capabilitiesEl->LinkEndChild ( contentsEl );

    doc.LinkEndChild ( capabilitiesEl );

    //doc.Print();      // affichage formaté sur stdout

    std::string wmtsCapaTemplate;
    wmtsCapaTemplate << doc;  // ecriture non formatée dans un std::string
    doc.Clear();

    // Découpage en fragments constants.
    size_t beginPos;
    size_t endPos;
    beginPos= 0;
    endPos  = wmtsCapaTemplate.find ( pathTag );
    while ( endPos != std::string::npos ) {
        wmtsCapaFrag.push_back ( wmtsCapaTemplate.substr ( beginPos,endPos-beginPos ) );
        beginPos = endPos + pathTag.length();
        endPos=wmtsCapaTemplate.find ( pathTag,beginPos );
    }
    wmtsCapaFrag.push_back ( wmtsCapaTemplate.substr ( beginPos ) );

    /*//debug: affichage des fragments.
        for (int i=0; i<wmtsCapaFrag.size();i++){
        std::cout << "(" << wmtsCapaFrag[i] << ")" << std::endl;
        }
        */

}

DataStream* Rok4Server::WMTSGetCapabilities ( Request* request ) {

    std::string version;
    DataStream* errorResp = getCapParamWMTS ( request, version );
    if ( errorResp ) {
        LOGGER_ERROR ( _ ( "Probleme dans les parametres de la requete getCapabilities" ) );
        return errorResp;
    }

    /* concaténation des fragments invariant de capabilities en intercalant les
      * parties variables dépendantes de la requête */
    std::string capa = "";
    for ( int i=0; i < wmtsCapaFrag.size()-1; i++ ) {
        capa = capa + wmtsCapaFrag[i] + request->scheme + request->hostName + request->path +"?";
    }
    capa = capa + wmtsCapaFrag.back();

    return new MessageDataStream ( capa,"application/xml" );
}

// Parameters for WMTS GetCapabilities
DataStream* Rok4Server::getCapParamWMTS ( Request* request, std::string& version ) {

    std::string location = request->getParam("location");
    if (location != "") {
        request->path = location;
    }
    version = request->getParam ( "version" );
    if ( version == "" ) {
        version = servicesConf->getServiceTypeVersion();
        return NULL;
    }
    if ( version.compare ( servicesConf->getServiceTypeVersion() ) !=0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Valeur du parametre VERSION invalide (1.0.0 disponible seulement)" ),"wmts" ) );

    return NULL;
}

DataStream* Rok4Server::getFeatureInfoParamWMTS ( Request* request, Layer*& layer, std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format, Style* &style, std::string& info_format, int& X, int& Y) {

    DataSource* getTileError = getTileParamWMTS(request, layer, tileMatrix, tileCol, tileRow, format, style);

    
    if (getTileError) {
        return new DataStreamFromDataSource(getTileError);
    }
    if (layer->isGetFeatureInfoAvailable() == false) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Layer " ) +layer->getId()+_ ( " non interrogeable." ),"wmts" ) );   
    }

    
    // INFO_FORMAT
    info_format = request->getParam ( "infoformat" );
    if ( info_format == "" ){
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre INFOFORMAT vide." ),"wmts" ) );
    }else{
        if ( Request::containForbiddenChars(info_format)) {
            LOGGER_WARN("Forbidden char detected in WMTS info_format: " << info_format);
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Info_Format non gere par le service." ),"wmts" ) );
        }

        if ( ! servicesConf->isInInfoFormatList(info_format) )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Info_Format " ) +info_format+_ ( " non gere par le service." ),"wmts" ) );
    }


    // Comme on va utiliser le niveau requêté pour contrôler les valeurs de I et J, on vérifie bien son existence
    if (layer->getDataPyramid()->getLevel(tileMatrix) == NULL) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre TILEMATRIX n'est pas valide pour cette couche." ),"wmts" ) );
    }

    // X
    std::string strX = request->getParam ( "i" );
    if ( strX == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre I absent." ),"wmts" ) );
    char c;
    if (sscanf(strX.c_str(), "%d%c", &X, &c) != 1) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre I n'est pas un entier." ),"wmts" ) );
    }
    if ( X<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre I est negative." ),"wmts" ) );
    if ( X> layer->getDataPyramid()->getLevel(tileMatrix)->getTm()->getTileW()-1 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre I est superieure a la largeur de la tuile (width)." ),"wmts" ) );


    // Y
    std::string strY = request->getParam ( "j" );
    if ( strY == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre J absent." ),"wmts" ) );

    if (sscanf(strY.c_str(), "%d%c", &Y, &c) != 1) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre J n'est pas un entier." ),"wmts" ) );
    }
    if ( Y<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre J est negative." ),"wmts" ) );
    if ( Y> layer->getDataPyramid()->getLevel(tileMatrix)->getTm()->getTileH()-1 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre J est superieure a la hauteur de la tuile (height)." ),"wmts" ) );

    return NULL;
}