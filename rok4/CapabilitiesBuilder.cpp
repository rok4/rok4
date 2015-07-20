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
#include "tinystr.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <map>
#include <cmath>
#include "TileMatrixSet.h"
#include "Pyramid.h"
#include "intl.h"

/**
 * \~french Conversion d'un entier en une chaîne de caractère
 * \~english Convert an integer in a character string
 */
std::string numToStr ( long int i ) {
    std::ostringstream strstr;
    strstr << i;
    return strstr.str();
}

/**
 * \~french Conversion d'un flottant en une chaîne de caractères
 * \~english Convert a float in a character string
 */
std::string doubleToStr ( long double d ) {
    std::ostringstream strstr;
    strstr.setf ( std::ios::fixed,std::ios::floatfield );
    strstr.precision ( 16 );
    strstr << d;
    return strstr.str();
}

/**
 * \~french Construit un noeud xml simple (de type text)
 * \~english Create a simple XML text node
 */
TiXmlElement * buildTextNode ( std::string elementName, std::string value ) {
    TiXmlElement * elem = new TiXmlElement ( elementName );
    TiXmlText * text = new TiXmlText ( value );
    elem->LinkEndChild ( text );
    return elem;
}

// Prepare WMS GetCapabilities fragments
//   Done only 1 time (during server initialization)
void Rok4Server::buildWMS130Capabilities() {
    std::vector <std::string> wms130CapaFrag;
    std::string hostNameTag="]HOSTNAME[";   ///Tag a remplacer par le nom du serveur
    std::string pathTag="]HOSTNAME/PATH[";  ///Tag à remplacer par le chemin complet avant le ?.
    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration ( "1.0", "UTF-8", "" );
    doc.LinkEndChild ( decl );
    std::ostringstream os;

    TiXmlElement * capabilitiesEl = new TiXmlElement ( "WMS_Capabilities" );
    capabilitiesEl->SetAttribute ( "version","1.3.0" );
    capabilitiesEl->SetAttribute ( "xmlns","http://www.opengis.net/wms" );
    capabilitiesEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    capabilitiesEl->SetAttribute ( "xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance" );
    capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd" );

    // Pour Inspire. Cf. remarque plus bas.
    if ( servicesConf.isInspire() ) {
        capabilitiesEl->SetAttribute ( "xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs/1.0" );
        capabilitiesEl->SetAttribute ( "xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
        capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd  http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd http://inspire.ec.europa.eu/schemas/common/1.0 http://inspire.ec.europa.eu/schemas/common/1.0/common.xsd" );
    }



    // Traitement de la partie service
    //----------------------------------
    TiXmlElement * serviceEl = new TiXmlElement ( "Service" );
    serviceEl->LinkEndChild ( buildTextNode ( "Name",servicesConf.getName() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "Title",servicesConf.getTitle() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "Abstract",servicesConf.getAbstract() ) );
    //KeywordList
    if ( servicesConf.getKeyWords()->size() != 0 ) {
        TiXmlElement * kwlEl = new TiXmlElement ( "KeywordList" );
        TiXmlElement * kwEl;
        for ( unsigned int i=0; i < servicesConf.getKeyWords()->size(); i++ ) {
            kwEl = new TiXmlElement ( "Keyword" );
            kwEl->LinkEndChild ( new TiXmlText ( servicesConf.getKeyWords()->at ( i ).getContent() ) );
            const std::map<std::string,std::string>* attributes = servicesConf.getKeyWords()->at ( i ).getAttributes();
            for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
            }
            kwlEl->LinkEndChild ( kwEl );
        }
        //kwlEl->LinkEndChild ( buildTextNode ( "Keyword", ROK4_INFO ) );
        serviceEl->LinkEndChild ( kwlEl );
    }
    //OnlineResource
    TiXmlElement * onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href",hostNameTag );
    serviceEl->LinkEndChild ( onlineResourceEl );
    // ContactInformation
    TiXmlElement * contactInformationEl = new TiXmlElement ( "ContactInformation" );

    TiXmlElement * contactPersonPrimaryEl = new TiXmlElement ( "ContactPersonPrimary" );
    contactPersonPrimaryEl->LinkEndChild ( buildTextNode ( "ContactPerson",servicesConf.getIndividualName() ) );
    contactPersonPrimaryEl->LinkEndChild ( buildTextNode ( "ContactOrganization",servicesConf.getServiceProvider() ) );

    contactInformationEl->LinkEndChild ( contactPersonPrimaryEl );

    contactInformationEl->LinkEndChild ( buildTextNode ( "ContactPosition",servicesConf.getIndividualPosition() ) );

    TiXmlElement * contactAddressEl = new TiXmlElement ( "ContactAddress" );
    contactAddressEl->LinkEndChild ( buildTextNode ( "AddressType",servicesConf.getAddressType() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "Address",servicesConf.getDeliveryPoint() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "City",servicesConf.getCity() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "StateOrProvince",servicesConf.getAdministrativeArea() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "PostCode",servicesConf.getPostCode() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "Country",servicesConf.getCountry() ) );

    contactInformationEl->LinkEndChild ( contactAddressEl );

    contactInformationEl->LinkEndChild ( buildTextNode ( "ContactVoiceTelephone",servicesConf.getVoice() ) );

    contactInformationEl->LinkEndChild ( buildTextNode ( "ContactFacsimileTelephone",servicesConf.getFacsimile() ) );

    contactInformationEl->LinkEndChild ( buildTextNode ( "ContactElectronicMailAddress",servicesConf.getElectronicMailAddress() ) );

    serviceEl->LinkEndChild ( contactInformationEl );

    serviceEl->LinkEndChild ( buildTextNode ( "Fees",servicesConf.getFee() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "AccessConstraints",servicesConf.getAccessConstraint() ) );

    os << servicesConf.getLayerLimit();
    serviceEl->LinkEndChild ( buildTextNode ( "LayerLimit",os.str() ) );
    os.str ( "" );
    serviceEl->LinkEndChild ( buildTextNode ( "MaxWidth",numToStr ( servicesConf.getMaxWidth() ) ) );
    serviceEl->LinkEndChild ( buildTextNode ( "MaxHeight",numToStr ( servicesConf.getMaxHeight() ) ) );

    capabilitiesEl->LinkEndChild ( serviceEl );



    // Traitement de la partie Capability
    //-----------------------------------
    TiXmlElement * capabilityEl = new TiXmlElement ( "Capability" );
    TiXmlElement * requestEl = new TiXmlElement ( "Request" );
    TiXmlElement * getCapabilitiestEl = new TiXmlElement ( "GetCapabilities" );

    getCapabilitiestEl->LinkEndChild ( buildTextNode ( "Format","text/xml" ) );
    //DCPType
    TiXmlElement * DCPTypeEl = new TiXmlElement ( "DCPType" );
    TiXmlElement * HTTPEl = new TiXmlElement ( "HTTP" );
    TiXmlElement * GetEl = new TiXmlElement ( "Get" );

    //OnlineResource
    onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
    GetEl->LinkEndChild ( onlineResourceEl );
    HTTPEl->LinkEndChild ( GetEl );

    if ( servicesConf.isPostEnabled() ) {
        TiXmlElement * PostEl = new TiXmlElement ( "Post" );
        onlineResourceEl = new TiXmlElement ( "OnlineResource" );
        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
        onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
        PostEl->LinkEndChild ( onlineResourceEl );
        HTTPEl->LinkEndChild ( PostEl );
    }

    DCPTypeEl->LinkEndChild ( HTTPEl );
    getCapabilitiestEl->LinkEndChild ( DCPTypeEl );
    requestEl->LinkEndChild ( getCapabilitiestEl );

    TiXmlElement * getMapEl = new TiXmlElement ( "GetMap" );
    for ( unsigned int i=0; i<servicesConf.getFormatList()->size(); i++ ) {
        getMapEl->LinkEndChild ( buildTextNode ( "Format",servicesConf.getFormatList()->at ( i ) ) );
    }
    DCPTypeEl = new TiXmlElement ( "DCPType" );
    HTTPEl = new TiXmlElement ( "HTTP" );
    GetEl = new TiXmlElement ( "Get" );
    onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
    GetEl->LinkEndChild ( onlineResourceEl );
    HTTPEl->LinkEndChild ( GetEl );

    if ( servicesConf.isPostEnabled() ) {
        TiXmlElement * PostEl = new TiXmlElement ( "Post" );
        onlineResourceEl = new TiXmlElement ( "OnlineResource" );
        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
        onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
        PostEl->LinkEndChild ( onlineResourceEl );
        HTTPEl->LinkEndChild ( PostEl );
    }

    DCPTypeEl->LinkEndChild ( HTTPEl );
    getMapEl->LinkEndChild ( DCPTypeEl );

    requestEl->LinkEndChild ( getMapEl );

    capabilityEl->LinkEndChild ( requestEl );

    //Exception
    TiXmlElement * exceptionEl = new TiXmlElement ( "Exception" );
    exceptionEl->LinkEndChild ( buildTextNode ( "Format","XML" ) );
    capabilityEl->LinkEndChild ( exceptionEl );

    // Inspire (extended Capability)
    if ( servicesConf.isInspire() ) {
        // TODO : en dur. A mettre dans la configuration du service (prevoir differents profils d'application possibles)
        TiXmlElement * extendedCapabilititesEl = new TiXmlElement ( "inspire_vs:ExtendedCapabilities" );

        // MetadataURL
        TiXmlElement * metadataUrlEl = new TiXmlElement ( "inspire_common:MetadataUrl" );
        metadataUrlEl->LinkEndChild ( buildTextNode ( "inspire_common:URL", servicesConf.getWMSMetadataURL()->getHRef() ) );
        metadataUrlEl->LinkEndChild ( buildTextNode ( "inspire_common:MediaType", servicesConf.getWMSMetadataURL()->getType() ) );
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
        responseLanguageEl->LinkEndChild ( buildTextNode ( "inspire_common:Language","fre" ) );
        extendedCapabilititesEl->LinkEndChild ( responseLanguageEl );

        capabilityEl->LinkEndChild ( extendedCapabilititesEl );
    }
    // Layer
    if ( layerList.empty() ) {
        LOGGER_ERROR ( _ ( "Liste de layers vide" ) );
    } else {
        // Parent layer
        TiXmlElement * parentLayerEl = new TiXmlElement ( "Layer" );
        // Title
        parentLayerEl->LinkEndChild ( buildTextNode ( "Title", "cache IGN" ) );
        // Abstract
        parentLayerEl->LinkEndChild ( buildTextNode ( "Abstract", "Cache IGN" ) );
        // Global CRS
        for ( unsigned int i=0; i < servicesConf.getGlobalCRSList()->size(); i++ ) {
            parentLayerEl->LinkEndChild ( buildTextNode ( "CRS", servicesConf.getGlobalCRSList()->at ( i ).getRequestCode() ) );
        }
        // Child layers
        std::map<std::string, Layer*>::iterator it;
        for ( it=layerList.begin(); it!=layerList.end(); it++ ) {
            TiXmlElement * childLayerEl = new TiXmlElement ( "Layer" );
            Layer* childLayer = it->second;
            // Name
            childLayerEl->LinkEndChild ( buildTextNode ( "Name", childLayer->getId() ) );
            // Title
            childLayerEl->LinkEndChild ( buildTextNode ( "Title", childLayer->getTitle() ) );
            // Abstract
            childLayerEl->LinkEndChild ( buildTextNode ( "Abstract", childLayer->getAbstract() ) );
            // KeywordList
            if ( childLayer->getKeyWords()->size() != 0 ) {
                TiXmlElement * kwlEl = new TiXmlElement ( "KeywordList" );

                TiXmlElement * kwEl;
                for ( unsigned int i=0; i < childLayer->getKeyWords()->size(); i++ ) {
                    kwEl = new TiXmlElement ( "Keyword" );
                    kwEl->LinkEndChild ( new TiXmlText ( childLayer->getKeyWords()->at ( i ).getContent() ) );
                    const std::map<std::string,std::string>* attributes = childLayer->getKeyWords()->at ( i ).getAttributes();
                    for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                        kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
                    }

                    kwlEl->LinkEndChild ( kwEl );
                }
                childLayerEl->LinkEndChild ( kwlEl );
            }
            // CRS
            for ( unsigned int i=0; i < childLayer->getWMSCRSList().size(); i++ ) {
                childLayerEl->LinkEndChild ( buildTextNode ( "CRS", childLayer->getWMSCRSList() [i].getRequestCode() ) );
            }
            // GeographicBoundingBox
            TiXmlElement * gbbEl = new TiXmlElement ( "EX_GeographicBoundingBox" );

            os.str ( "" );
            os<<childLayer->getGeographicBoundingBox().minx;
            gbbEl->LinkEndChild ( buildTextNode ( "westBoundLongitude", os.str() ) );
            os.str ( "" );
            os<<childLayer->getGeographicBoundingBox().maxx;
            gbbEl->LinkEndChild ( buildTextNode ( "eastBoundLongitude", os.str() ) );
            os.str ( "" );
            os<<childLayer->getGeographicBoundingBox().miny;
            gbbEl->LinkEndChild ( buildTextNode ( "southBoundLatitude", os.str() ) );
            os.str ( "" );
            os<<childLayer->getGeographicBoundingBox().maxy;
            gbbEl->LinkEndChild ( buildTextNode ( "northBoundLatitude", os.str() ) );
            os.str ( "" );
            childLayerEl->LinkEndChild ( gbbEl );


            // BoundingBox
            if ( servicesConf.isInspire() ) {
                for ( unsigned int i=0; i < childLayer->getWMSCRSList().size(); i++ ) {
                    BoundingBox<double> bbox ( 0,0,0,0 );
                    if ( childLayer->getWMSCRSList() [i].validateBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) ) {
                        bbox = childLayer->getWMSCRSList() [i].boundingBoxFromGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy );
                    } else {
                        bbox = childLayer->getWMSCRSList() [i].boundingBoxFromGeographic ( childLayer->getWMSCRSList() [i].cropBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) );
                    }
                    CRS crs = childLayer->getWMSCRSList() [i];
                    LOGGER_DEBUG ("check inverse for "<< crs.getProj4Code());
                    //Switch lon lat for EPSG longlat CRS
                    if ( ( crs.getAuthority() =="EPSG" || crs.getAuthority() =="epsg" ) && crs.isLongLat() ) {
                        double doubletmp;
                        doubletmp = bbox.xmin;
                        bbox.xmin = bbox.ymin;
                        bbox.ymin = doubletmp;
                        doubletmp = bbox.xmax;
                        bbox.xmax = bbox.ymax;
                        bbox.ymax = doubletmp;
                    }

                    TiXmlElement * bbEl = new TiXmlElement ( "BoundingBox" );
                    bbEl->SetAttribute ( "CRS",childLayer->getWMSCRSList() [i].getRequestCode() );
                    int floatprecision = GetDecimalPlaces ( bbox.xmin );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.xmax ) );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymin ) );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymax ) );
                    floatprecision = std::min ( floatprecision,9 ); //FIXME gestion du nombre maximal de décimal.

                    os.str ( "" );
                    os<< std::fixed << std::setprecision ( floatprecision );
                    os<<bbox.xmin;
                    bbEl->SetAttribute ( "minx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymin;
                    bbEl->SetAttribute ( "miny",os.str() );
                    os.str ( "" );
                    os<<bbox.xmax;
                    bbEl->SetAttribute ( "maxx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymax;
                    bbEl->SetAttribute ( "maxy",os.str() );
                    os.str ( "" );
                    childLayerEl->LinkEndChild ( bbEl );
                }
                for ( unsigned int i=0; i < servicesConf.getGlobalCRSList()->size(); i++ ) {
                    BoundingBox<double> bbox ( 0,0,0,0 );
                    if ( servicesConf.getGlobalCRSList()->at ( i ).validateBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) ) {
                        bbox = servicesConf.getGlobalCRSList()->at ( i ).boundingBoxFromGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy );
                    } else {
                        bbox = servicesConf.getGlobalCRSList()->at ( i ).boundingBoxFromGeographic ( servicesConf.getGlobalCRSList()->at ( i ).cropBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) );
                    }
                    CRS crs = servicesConf.getGlobalCRSList()->at ( i );
                    //Switch lon lat for EPSG longlat CRS
                    LOGGER_DEBUG ("check inverse for "<< crs.getProj4Code());
                    if ( ( crs.getAuthority() =="EPSG" || crs.getAuthority() =="epsg" ) && crs.isLongLat() ) {
                        double doubletmp;
                        doubletmp = bbox.xmin;
                        bbox.xmin = bbox.ymin;
                        bbox.ymin = doubletmp;
                        doubletmp = bbox.xmax;
                        bbox.xmax = bbox.ymax;
                        bbox.ymax = doubletmp;
                    }

                    TiXmlElement * bbEl = new TiXmlElement ( "BoundingBox" );
                    bbEl->SetAttribute ( "CRS",servicesConf.getGlobalCRSList()->at ( i ).getRequestCode() );
                    int floatprecision = GetDecimalPlaces ( bbox.xmin );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.xmax ) );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymin ) );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymax ) );
                    floatprecision = std::min ( floatprecision,9 ); //FIXME gestion du nombre maximal de décimal.
                    os.str ( "" );
                    os<< std::fixed << std::setprecision ( floatprecision );
                    os<<bbox.xmin;
                    bbEl->SetAttribute ( "minx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymin;
                    bbEl->SetAttribute ( "miny",os.str() );
                    os.str ( "" );
                    os<<bbox.xmax;
                    bbEl->SetAttribute ( "maxx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymax;
                    bbEl->SetAttribute ( "maxy",os.str() );
                    os.str ( "" );
                    childLayerEl->LinkEndChild ( bbEl );
                }
            } else {
                TiXmlElement * bbEl = new TiXmlElement ( "BoundingBox" );
                bbEl->SetAttribute ( "CRS",childLayer->getBoundingBox().srs );
                bbEl->SetAttribute ( "minx",childLayer->getBoundingBox().minx );
                bbEl->SetAttribute ( "miny",childLayer->getBoundingBox().miny );
                bbEl->SetAttribute ( "maxx",childLayer->getBoundingBox().maxx );
                bbEl->SetAttribute ( "maxy",childLayer->getBoundingBox().maxy );
                childLayerEl->LinkEndChild ( bbEl );
            }
            //MetadataURL
            if ( childLayer->getMetadataURLs().size() != 0 ) {
                for ( unsigned int i=0; i < childLayer->getMetadataURLs().size(); ++i ) {
                    TiXmlElement * mtdURLEl = new TiXmlElement ( "MetadataURL" );
                    MetadataURL mtdUrl = childLayer->getMetadataURLs().at ( i );
                    mtdURLEl->SetAttribute ( "type", mtdUrl.getType() );
                    mtdURLEl->LinkEndChild ( buildTextNode ( "Format",mtdUrl.getFormat() ) );

                    TiXmlElement* onlineResourceEl = new TiXmlElement ( "OnlineResource" );
                    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
                    onlineResourceEl->SetAttribute ( "xlink:href", mtdUrl.getHRef() );
                    mtdURLEl->LinkEndChild ( onlineResourceEl );
                    childLayerEl->LinkEndChild ( mtdURLEl );
                }
            }

            // Style
            LOGGER_DEBUG ( _ ( "Nombre de styles : " ) <<childLayer->getStyles().size() );
            if ( childLayer->getStyles().size() != 0 ) {
                for ( unsigned int i=0; i < childLayer->getStyles().size(); i++ ) {
                    TiXmlElement * styleEl= new TiXmlElement ( "Style" );
                    Style* style = childLayer->getStyles() [i];
                    styleEl->LinkEndChild ( buildTextNode ( "Name", style->getId().c_str() ) );
                    int j;
                    for ( j=0 ; j < style->getTitles().size(); ++j ) {
                        styleEl->LinkEndChild ( buildTextNode ( "Title", style->getTitles() [j].c_str() ) );
                    }
                    for ( j=0 ; j < style->getAbstracts().size(); ++j ) {
                        styleEl->LinkEndChild ( buildTextNode ( "Abstract", style->getAbstracts() [j].c_str() ) );
                    }
                    for ( j=0 ; j < style->getLegendURLs().size(); ++j ) {
                        LOGGER_DEBUG ( _ ( "LegendURL" ) << style->getId() );
                        LegendURL legendURL = style->getLegendURLs() [j];
                        TiXmlElement* legendURLEl = new TiXmlElement ( "LegendURL" );

                        TiXmlElement* onlineResourceEl = new TiXmlElement ( "OnlineResource" );
                        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
                        onlineResourceEl->SetAttribute ( "xlink:href", legendURL.getHRef() );
                        legendURLEl->LinkEndChild ( buildTextNode ( "Format", legendURL.getFormat() ) );
                        legendURLEl->LinkEndChild ( onlineResourceEl );

                        if ( legendURL.getWidth() !=0 )
                            legendURLEl->SetAttribute ( "width", legendURL.getWidth() );
                        if ( legendURL.getHeight() !=0 )
                            legendURLEl->SetAttribute ( "height", legendURL.getHeight() );
                        styleEl->LinkEndChild ( legendURLEl );
                        LOGGER_DEBUG ( _ ( "LegendURL OK" ) << style->getId() );
                    }

                    LOGGER_DEBUG ( _ ( "Style fini : " ) << style->getId() );
                    childLayerEl->LinkEndChild ( styleEl );
                }
            }

            // Scale denominators
            os.str ( "" );
            os<<childLayer->getMinRes() *1000/0.28;
            childLayerEl->LinkEndChild ( buildTextNode ( "MinScaleDenominator", os.str() ) );
            os.str ( "" );
            os<<childLayer->getMaxRes() *1000/0.28;
            childLayerEl->LinkEndChild ( buildTextNode ( "MaxScaleDenominator", os.str() ) );

            // TODO : gerer le cas des CRS avec des unites en degres

            /* TODO:
             *
             layer->getAuthority();
             layer->getOpaque();

            */
            LOGGER_DEBUG ( _ ( "Layer Fini" ) );
            parentLayerEl->LinkEndChild ( childLayerEl );

        }// for layer
        LOGGER_DEBUG ( _ ( "Layers Fini" ) );
        capabilityEl->LinkEndChild ( parentLayerEl );
    }

    capabilitiesEl->LinkEndChild ( capabilityEl );
    doc.LinkEndChild ( capabilitiesEl );

    // std::cout << doc; // ecriture non formatée dans le flux
    // doc.Print();      // affichage formaté sur stdout
    std::string wmsCapaTemplate;
    wmsCapaTemplate << doc;  // ecriture non formatée dans un std::string
    doc.Clear();

    // Découpage en fragments constants.
    size_t beginPos;
    size_t endPos;
    endPos=wmsCapaTemplate.find ( hostNameTag );
    wms130CapaFrag.push_back ( wmsCapaTemplate.substr ( 0,endPos ) );

    beginPos= endPos + hostNameTag.length();
    endPos  = wmsCapaTemplate.find ( pathTag, beginPos );
    while ( endPos != std::string::npos ) {
        wms130CapaFrag.push_back ( wmsCapaTemplate.substr ( beginPos,endPos-beginPos ) );
        beginPos = endPos + pathTag.length();
        endPos=wmsCapaTemplate.find ( pathTag,beginPos );
    }
    wms130CapaFrag.push_back ( wmsCapaTemplate.substr ( beginPos ) );
    wmsCapaFrag.insert( std::pair<std::string,std::vector<std::string> > ("1.3.0",wms130CapaFrag) );
    LOGGER_DEBUG ( _ ( "WMS 1.3.0 fini" ) );
}

//---- WMS 1.1.1
void Rok4Server::buildWMS111Capabilities() {
    std::vector <std::string> wms111CapaFrag;
    std::string hostNameTag="]HOSTNAME[";   ///Tag a remplacer par le nom du serveur
    std::string pathTag="]HOSTNAME/PATH[";  ///Tag à remplacer par le chemin complet avant le ?.
    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration ( "1.0", "UTF-8", "" );
    doc.LinkEndChild ( decl );
    std::ostringstream os;

    TiXmlElement * capabilitiesEl = new TiXmlElement ( "WMS_Capabilities" );
    capabilitiesEl->SetAttribute ( "version","1.1.1" );
    capabilitiesEl->SetAttribute ( "xmlns","http://www.opengis.net/wms" );
    capabilitiesEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    capabilitiesEl->SetAttribute ( "xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance" );
    capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.1.1/capabilities_1_1_1.xml" );

    // Pour Inspire. Cf. remarque plus bas.
    if ( servicesConf.isInspire() ) {
        capabilitiesEl->SetAttribute ( "xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs/1.0" );
        capabilitiesEl->SetAttribute ( "xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
        capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.1.1/capabilities_1_1_1.xml  http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd http://inspire.ec.europa.eu/schemas/common/1.0 http://inspire.ec.europa.eu/schemas/common/1.0/common.xsd" );
    }



    // Traitement de la partie service
    //----------------------------------
    TiXmlElement * serviceEl = new TiXmlElement ( "Service" );
    serviceEl->LinkEndChild ( buildTextNode ( "Name",servicesConf.getName() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "Title",servicesConf.getTitle() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "Abstract",servicesConf.getAbstract() ) );
    //KeywordList
    if ( servicesConf.getKeyWords()->size() != 0 ) {
        TiXmlElement * kwlEl = new TiXmlElement ( "KeywordList" );
        TiXmlElement * kwEl;
        for ( unsigned int i=0; i < servicesConf.getKeyWords()->size(); i++ ) {
            kwEl = new TiXmlElement ( "Keyword" );
            kwEl->LinkEndChild ( new TiXmlText ( servicesConf.getKeyWords()->at ( i ).getContent() ) );
            const std::map<std::string,std::string>* attributes = servicesConf.getKeyWords()->at ( i ).getAttributes();
            for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
            }
            kwlEl->LinkEndChild ( kwEl );
        }
        //kwlEl->LinkEndChild ( buildTextNode ( "Keyword", ROK4_INFO ) );
        serviceEl->LinkEndChild ( kwlEl );
    }
    //OnlineResource
    TiXmlElement * onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href",hostNameTag );
    serviceEl->LinkEndChild ( onlineResourceEl );
    // ContactInformation
    TiXmlElement * contactInformationEl = new TiXmlElement ( "ContactInformation" );

    TiXmlElement * contactPersonPrimaryEl = new TiXmlElement ( "ContactPersonPrimary" );
    contactPersonPrimaryEl->LinkEndChild ( buildTextNode ( "ContactPerson",servicesConf.getIndividualName() ) );
    contactPersonPrimaryEl->LinkEndChild ( buildTextNode ( "ContactOrganization",servicesConf.getServiceProvider() ) );

    contactInformationEl->LinkEndChild ( contactPersonPrimaryEl );

    contactInformationEl->LinkEndChild ( buildTextNode ( "ContactPosition",servicesConf.getIndividualPosition() ) );

    TiXmlElement * contactAddressEl = new TiXmlElement ( "ContactAddress" );
    contactAddressEl->LinkEndChild ( buildTextNode ( "AddressType",servicesConf.getAddressType() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "Address",servicesConf.getDeliveryPoint() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "City",servicesConf.getCity() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "StateOrProvince",servicesConf.getAdministrativeArea() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "PostCode",servicesConf.getPostCode() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "Country",servicesConf.getCountry() ) );

    contactInformationEl->LinkEndChild ( contactAddressEl );

    contactInformationEl->LinkEndChild ( buildTextNode ( "ContactVoiceTelephone",servicesConf.getVoice() ) );

    contactInformationEl->LinkEndChild ( buildTextNode ( "ContactFacsimileTelephone",servicesConf.getFacsimile() ) );

    contactInformationEl->LinkEndChild ( buildTextNode ( "ContactElectronicMailAddress",servicesConf.getElectronicMailAddress() ) );

    serviceEl->LinkEndChild ( contactInformationEl );

    serviceEl->LinkEndChild ( buildTextNode ( "Fees",servicesConf.getFee() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "AccessConstraints",servicesConf.getAccessConstraint() ) );

    os << servicesConf.getLayerLimit();
    serviceEl->LinkEndChild ( buildTextNode ( "LayerLimit",os.str() ) );
    os.str ( "" );
    serviceEl->LinkEndChild ( buildTextNode ( "MaxWidth",numToStr ( servicesConf.getMaxWidth() ) ) );
    serviceEl->LinkEndChild ( buildTextNode ( "MaxHeight",numToStr ( servicesConf.getMaxHeight() ) ) );

    capabilitiesEl->LinkEndChild ( serviceEl );



    // Traitement de la partie Capability
    //-----------------------------------
    TiXmlElement * capabilityEl = new TiXmlElement ( "Capability" );
    TiXmlElement * requestEl = new TiXmlElement ( "Request" );
    TiXmlElement * getCapabilitiestEl = new TiXmlElement ( "GetCapabilities" );

    getCapabilitiestEl->LinkEndChild ( buildTextNode ( "Format","text/xml" ) );
    //DCPType
    TiXmlElement * DCPTypeEl = new TiXmlElement ( "DCPType" );
    TiXmlElement * HTTPEl = new TiXmlElement ( "HTTP" );
    TiXmlElement * GetEl = new TiXmlElement ( "Get" );

    //OnlineResource
    onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
    GetEl->LinkEndChild ( onlineResourceEl );
    HTTPEl->LinkEndChild ( GetEl );

    if ( servicesConf.isPostEnabled() ) {
        TiXmlElement * PostEl = new TiXmlElement ( "Post" );
        onlineResourceEl = new TiXmlElement ( "OnlineResource" );
        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
        onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
        PostEl->LinkEndChild ( onlineResourceEl );
        HTTPEl->LinkEndChild ( PostEl );
    }

    DCPTypeEl->LinkEndChild ( HTTPEl );
    getCapabilitiestEl->LinkEndChild ( DCPTypeEl );
    requestEl->LinkEndChild ( getCapabilitiestEl );

    TiXmlElement * getMapEl = new TiXmlElement ( "GetMap" );
    for ( unsigned int i=0; i<servicesConf.getFormatList()->size(); i++ ) {
        getMapEl->LinkEndChild ( buildTextNode ( "Format",servicesConf.getFormatList()->at ( i ) ) );
    }
    DCPTypeEl = new TiXmlElement ( "DCPType" );
    HTTPEl = new TiXmlElement ( "HTTP" );
    GetEl = new TiXmlElement ( "Get" );
    onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
    GetEl->LinkEndChild ( onlineResourceEl );
    HTTPEl->LinkEndChild ( GetEl );

    if ( servicesConf.isPostEnabled() ) {
        TiXmlElement * PostEl = new TiXmlElement ( "Post" );
        onlineResourceEl = new TiXmlElement ( "OnlineResource" );
        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
        onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
        PostEl->LinkEndChild ( onlineResourceEl );
        HTTPEl->LinkEndChild ( PostEl );
    }

    DCPTypeEl->LinkEndChild ( HTTPEl );
    getMapEl->LinkEndChild ( DCPTypeEl );

    requestEl->LinkEndChild ( getMapEl );

    capabilityEl->LinkEndChild ( requestEl );

    //Exception
    TiXmlElement * exceptionEl = new TiXmlElement ( "Exception" );
    exceptionEl->LinkEndChild ( buildTextNode ( "Format","XML" ) );
    capabilityEl->LinkEndChild ( exceptionEl );

    // Inspire (extended Capability)
    if ( servicesConf.isInspire() ) {
        // TODO : en dur. A mettre dans la configuration du service (prevoir differents profils d'application possibles)
        TiXmlElement * extendedCapabilititesEl = new TiXmlElement ( "inspire_vs:ExtendedCapabilities" );

        // MetadataURL
        TiXmlElement * metadataUrlEl = new TiXmlElement ( "inspire_common:MetadataUrl" );
        metadataUrlEl->LinkEndChild ( buildTextNode ( "inspire_common:URL", servicesConf.getWMSMetadataURL()->getHRef() ) );
        metadataUrlEl->LinkEndChild ( buildTextNode ( "inspire_common:MediaType", servicesConf.getWMSMetadataURL()->getType() ) );
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
        responseLanguageEl->LinkEndChild ( buildTextNode ( "inspire_common:Language","fre" ) );
        extendedCapabilititesEl->LinkEndChild ( responseLanguageEl );

        capabilityEl->LinkEndChild ( extendedCapabilititesEl );
    }
    // Layer
    if ( layerList.empty() ) {
        LOGGER_ERROR ( _ ( "Liste de layers vide" ) );
    } else {
        // Parent layer
        TiXmlElement * parentLayerEl = new TiXmlElement ( "Layer" );
        // Title
        parentLayerEl->LinkEndChild ( buildTextNode ( "Title", "cache IGN" ) );
        // Abstract
        parentLayerEl->LinkEndChild ( buildTextNode ( "Abstract", "Cache IGN" ) );
        // Global CRS
        for ( unsigned int i=0; i < servicesConf.getGlobalCRSList()->size(); i++ ) {
            parentLayerEl->LinkEndChild ( buildTextNode ( "SRS", servicesConf.getGlobalCRSList()->at ( i ).getRequestCode() ) );
        }
        // Child layers
        std::map<std::string, Layer*>::iterator it;
        for ( it=layerList.begin(); it!=layerList.end(); it++ ) {
            TiXmlElement * childLayerEl = new TiXmlElement ( "Layer" );
            Layer* childLayer = it->second;
            // Name
            childLayerEl->LinkEndChild ( buildTextNode ( "Name", childLayer->getId() ) );
            // Title
            childLayerEl->LinkEndChild ( buildTextNode ( "Title", childLayer->getTitle() ) );
            // Abstract
            childLayerEl->LinkEndChild ( buildTextNode ( "Abstract", childLayer->getAbstract() ) );
            // KeywordList
            if ( childLayer->getKeyWords()->size() != 0 ) {
                TiXmlElement * kwlEl = new TiXmlElement ( "KeywordList" );

                TiXmlElement * kwEl;
                for ( unsigned int i=0; i < childLayer->getKeyWords()->size(); i++ ) {
                    kwEl = new TiXmlElement ( "Keyword" );
                    kwEl->LinkEndChild ( new TiXmlText ( childLayer->getKeyWords()->at ( i ).getContent() ) );
                    const std::map<std::string,std::string>* attributes = childLayer->getKeyWords()->at ( i ).getAttributes();
                    for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                        kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
                    }

                    kwlEl->LinkEndChild ( kwEl );
                }
                childLayerEl->LinkEndChild ( kwlEl );
            }
            // CRS
            for ( unsigned int i=0; i < childLayer->getWMSCRSList().size(); i++ ) {
                childLayerEl->LinkEndChild ( buildTextNode ( "SRS", childLayer->getWMSCRSList() [i].getRequestCode() ) );
            }
            // GeographicBoundingBox
            TiXmlElement * gbbEl = new TiXmlElement ( "LatLonBoundingBox" );

            os.str ( "" );
            os<<childLayer->getGeographicBoundingBox().minx;
            gbbEl->SetAttribute ( "minx",os.str() );
            os.str ( "" );
            os<<childLayer->getGeographicBoundingBox().miny;
            gbbEl->SetAttribute ( "miny",os.str() );
            os.str ( "" );
            os<<childLayer->getGeographicBoundingBox().maxx;
            gbbEl->SetAttribute ( "maxx",os.str() );
            os.str ( "" );
            os<<childLayer->getGeographicBoundingBox().maxy;
            gbbEl->SetAttribute ( "maxy",os.str() );
            os.str ( "" );
            childLayerEl->LinkEndChild ( gbbEl );


            // BoundingBox
            if ( servicesConf.isInspire() ) {
                for ( unsigned int i=0; i < childLayer->getWMSCRSList().size(); i++ ) {
                    BoundingBox<double> bbox ( 0,0,0,0 );
                    if ( childLayer->getWMSCRSList() [i].validateBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) ) {
                        bbox = childLayer->getWMSCRSList() [i].boundingBoxFromGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy );
                    } else {
                        bbox = childLayer->getWMSCRSList() [i].boundingBoxFromGeographic ( childLayer->getWMSCRSList() [i].cropBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) );
                    }
                    CRS crs = childLayer->getWMSCRSList() [i];
                    LOGGER_DEBUG ("check inverse for "<< crs.getProj4Code());
                    //Switch lon lat for EPSG longlat CRS
                    if ( ( crs.getAuthority() =="EPSG" || crs.getAuthority() =="epsg" ) && crs.isLongLat() ) {
                        double doubletmp;
                        doubletmp = bbox.xmin;
                        bbox.xmin = bbox.ymin;
                        bbox.ymin = doubletmp;
                        doubletmp = bbox.xmax;
                        bbox.xmax = bbox.ymax;
                        bbox.ymax = doubletmp;
                    }

                    TiXmlElement * bbEl = new TiXmlElement ( "BoundingBox" );
                    bbEl->SetAttribute ( "SRS",childLayer->getWMSCRSList() [i].getRequestCode() );
                    int floatprecision = GetDecimalPlaces ( bbox.xmin );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.xmax ) );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymin ) );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymax ) );
                    floatprecision = std::min ( floatprecision,9 ); //FIXME gestion du nombre maximal de décimal.

                    os.str ( "" );
                    os<< std::fixed << std::setprecision ( floatprecision );
                    os<<bbox.xmin;
                    bbEl->SetAttribute ( "minx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymin;
                    bbEl->SetAttribute ( "miny",os.str() );
                    os.str ( "" );
                    os<<bbox.xmax;
                    bbEl->SetAttribute ( "maxx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymax;
                    bbEl->SetAttribute ( "maxy",os.str() );
                    os.str ( "" );
                    childLayerEl->LinkEndChild ( bbEl );
                }
                for ( unsigned int i=0; i < servicesConf.getGlobalCRSList()->size(); i++ ) {
                    BoundingBox<double> bbox ( 0,0,0,0 );
                    if ( servicesConf.getGlobalCRSList()->at ( i ).validateBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) ) {
                        bbox = servicesConf.getGlobalCRSList()->at ( i ).boundingBoxFromGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy );
                    } else {
                        bbox = servicesConf.getGlobalCRSList()->at ( i ).boundingBoxFromGeographic ( servicesConf.getGlobalCRSList()->at ( i ).cropBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) );
                    }
                    CRS crs = servicesConf.getGlobalCRSList()->at ( i );
                    //Switch lon lat for EPSG longlat CRS
                    LOGGER_DEBUG ("check inverse for "<< crs.getProj4Code());
                    if ( ( crs.getAuthority() =="EPSG" || crs.getAuthority() =="epsg" ) && crs.isLongLat() ) {
                        double doubletmp;
                        doubletmp = bbox.xmin;
                        bbox.xmin = bbox.ymin;
                        bbox.ymin = doubletmp;
                        doubletmp = bbox.xmax;
                        bbox.xmax = bbox.ymax;
                        bbox.ymax = doubletmp;
                    }

                    TiXmlElement * bbEl = new TiXmlElement ( "BoundingBox" );
                    bbEl->SetAttribute ( "SRS",servicesConf.getGlobalCRSList()->at ( i ).getRequestCode() );
                    int floatprecision = GetDecimalPlaces ( bbox.xmin );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.xmax ) );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymin ) );
                    floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymax ) );
                    floatprecision = std::min ( floatprecision,9 ); //FIXME gestion du nombre maximal de décimal.
                    os.str ( "" );
                    os<< std::fixed << std::setprecision ( floatprecision );
                    os<<bbox.xmin;
                    bbEl->SetAttribute ( "minx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymin;
                    bbEl->SetAttribute ( "miny",os.str() );
                    os.str ( "" );
                    os<<bbox.xmax;
                    bbEl->SetAttribute ( "maxx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymax;
                    bbEl->SetAttribute ( "maxy",os.str() );
                    os.str ( "" );
                    childLayerEl->LinkEndChild ( bbEl );
                }
            } else {
                TiXmlElement * bbEl = new TiXmlElement ( "BoundingBox" );
                bbEl->SetAttribute ( "SRS",childLayer->getBoundingBox().srs );
                bbEl->SetAttribute ( "minx",childLayer->getBoundingBox().minx );
                bbEl->SetAttribute ( "miny",childLayer->getBoundingBox().miny );
                bbEl->SetAttribute ( "maxx",childLayer->getBoundingBox().maxx );
                bbEl->SetAttribute ( "maxy",childLayer->getBoundingBox().maxy );
                childLayerEl->LinkEndChild ( bbEl );
            }
            //MetadataURL
            if ( childLayer->getMetadataURLs().size() != 0 ) {
                for ( unsigned int i=0; i < childLayer->getMetadataURLs().size(); ++i ) {
                    TiXmlElement * mtdURLEl = new TiXmlElement ( "MetadataURL" );
                    MetadataURL mtdUrl = childLayer->getMetadataURLs().at ( i );
                    mtdURLEl->SetAttribute ( "type", mtdUrl.getType() );
                    mtdURLEl->LinkEndChild ( buildTextNode ( "Format",mtdUrl.getFormat() ) );

                    TiXmlElement* onlineResourceEl = new TiXmlElement ( "OnlineResource" );
                    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
                    onlineResourceEl->SetAttribute ( "xlink:href", mtdUrl.getHRef() );
                    mtdURLEl->LinkEndChild ( onlineResourceEl );
                    childLayerEl->LinkEndChild ( mtdURLEl );
                }
            }

            // Style
            LOGGER_DEBUG ( _ ( "Nombre de styles : " ) <<childLayer->getStyles().size() );
            if ( childLayer->getStyles().size() != 0 ) {
                for ( unsigned int i=0; i < childLayer->getStyles().size(); i++ ) {
                    TiXmlElement * styleEl= new TiXmlElement ( "Style" );
                    Style* style = childLayer->getStyles() [i];
                    styleEl->LinkEndChild ( buildTextNode ( "Name", style->getId().c_str() ) );
                    int j;
                    for ( j=0 ; j < style->getTitles().size(); ++j ) {
                        styleEl->LinkEndChild ( buildTextNode ( "Title", style->getTitles() [j].c_str() ) );
                    }
                    for ( j=0 ; j < style->getAbstracts().size(); ++j ) {
                        styleEl->LinkEndChild ( buildTextNode ( "Abstract", style->getAbstracts() [j].c_str() ) );
                    }
                    for ( j=0 ; j < style->getLegendURLs().size(); ++j ) {
                        LOGGER_DEBUG ( _ ( "LegendURL" ) << style->getId() );
                        LegendURL legendURL = style->getLegendURLs() [j];
                        TiXmlElement* legendURLEl = new TiXmlElement ( "LegendURL" );

                        TiXmlElement* onlineResourceEl = new TiXmlElement ( "OnlineResource" );
                        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
                        onlineResourceEl->SetAttribute ( "xlink:href", legendURL.getHRef() );
                        legendURLEl->LinkEndChild ( buildTextNode ( "Format", legendURL.getFormat() ) );
                        legendURLEl->LinkEndChild ( onlineResourceEl );

                        if ( legendURL.getWidth() !=0 )
                            legendURLEl->SetAttribute ( "width", legendURL.getWidth() );
                        if ( legendURL.getHeight() !=0 )
                            legendURLEl->SetAttribute ( "height", legendURL.getHeight() );
                        styleEl->LinkEndChild ( legendURLEl );
                        LOGGER_DEBUG ( _ ( "LegendURL OK" ) << style->getId() );
                    }

                    LOGGER_DEBUG ( _ ( "Style fini : " ) << style->getId() );
                    childLayerEl->LinkEndChild ( styleEl );
                }
            }

            // Scale denominators
            TiXmlElement * siEl = new TiXmlElement ( "ScaleInt" );

            os.str ( "" );
            os<<childLayer->getMaxRes() *1000/0.28;
            siEl->SetAttribute ( "max",os.str() );
            os.str ( "" );
            os<<childLayer->getMinRes() *1000/0.28;
            siEl->SetAttribute ( "min",os.str() );
            os.str ( "" );
            childLayerEl->LinkEndChild ( siEl );

            // TODO : gerer le cas des CRS avec des unites en degres

            /* TODO:
             *
             layer->getAuthority();
             layer->getOpaque();

            */
            LOGGER_DEBUG ( _ ( "Layer Fini" ) );
            parentLayerEl->LinkEndChild ( childLayerEl );

        }// for layer
        LOGGER_DEBUG ( _ ( "Layers Fini" ) );
        capabilityEl->LinkEndChild ( parentLayerEl );
    }

    capabilitiesEl->LinkEndChild ( capabilityEl );
    doc.LinkEndChild ( capabilitiesEl );

    // std::cout << doc; // ecriture non formatée dans le flux
    // doc.Print();      // affichage formaté sur stdout
    std::string wmsCapaTemplate;
    wmsCapaTemplate << doc;  // ecriture non formatée dans un std::string
    doc.Clear();

    // Découpage en fragments constants.
    size_t beginPos;
    size_t endPos;
    endPos=wmsCapaTemplate.find ( hostNameTag );
    wms111CapaFrag.push_back ( wmsCapaTemplate.substr ( 0,endPos ) );

    beginPos= endPos + hostNameTag.length();
    endPos  = wmsCapaTemplate.find ( pathTag, beginPos );
    while ( endPos != std::string::npos ) {
        wms111CapaFrag.push_back ( wmsCapaTemplate.substr ( beginPos,endPos-beginPos ) );
        beginPos = endPos + pathTag.length();
        endPos=wmsCapaTemplate.find ( pathTag,beginPos );
    }
    wms111CapaFrag.push_back ( wmsCapaTemplate.substr ( beginPos ) );
    wmsCapaFrag.insert( std::pair<std::string,std::vector<std::string> > ("1.1.1",wms111CapaFrag) );
    LOGGER_DEBUG ( _ ( "WMS 1.1.1 fini" ) );
}
//----

// Prepare WMTS GetCapabilities fragments
//   Done only 1 time (during server initialization)
void Rok4Server::buildWMTSCapabilities() {
    // std::string hostNameTag="]HOSTNAME[";   ///Tag a remplacer par le nom du serveur
    std::string pathTag="]HOSTNAME/PATH[";  ///Tag à remplacer par le chemin complet avant le ?.

    std::map<std::string,TileMatrixSet> usedTMSList;

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
    if ( servicesConf.isInspire() ) {
        capabilitiesEl->SetAttribute ( "xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
        capabilitiesEl->SetAttribute ( "xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0" );
        capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0/inspire_vs_ows_11.xsd" );
    }


    //----------------------------------------------------------------------
    // ServiceIdentification
    //----------------------------------------------------------------------
    TiXmlElement * serviceEl = new TiXmlElement ( "ows:ServiceIdentification" );

    serviceEl->LinkEndChild ( buildTextNode ( "ows:Title", servicesConf.getTitle() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "ows:Abstract", servicesConf.getAbstract() ) );
    //KeywordList
    if ( servicesConf.getKeyWords()->size() != 0 ) {
        TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
        TiXmlElement * kwEl;
        for ( unsigned int i=0; i < servicesConf.getKeyWords()->size(); i++ ) {
            kwEl = new TiXmlElement ( "ows:Keyword" );
            kwEl->LinkEndChild ( new TiXmlText ( servicesConf.getKeyWords()->at ( i ).getContent() ) );
            const std::map<std::string,std::string>* attributes = servicesConf.getKeyWords()->at ( i ).getAttributes();
            for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
            }

            kwlEl->LinkEndChild ( kwEl );
        }
        //kwlEl->LinkEndChild ( buildTextNode ( "ows:Keyword", ROK4_INFO ) );
        serviceEl->LinkEndChild ( kwlEl );
    }
    serviceEl->LinkEndChild ( buildTextNode ( "ows:ServiceType", servicesConf.getServiceType() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "ows:ServiceTypeVersion", servicesConf.getServiceTypeVersion() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "ows:Fees", servicesConf.getFee() ) );
    serviceEl->LinkEndChild ( buildTextNode ( "ows:AccessConstraints", servicesConf.getAccessConstraint() ) );


    capabilitiesEl->LinkEndChild ( serviceEl );

    //----------------------------------------------------------------------
    // serviceProvider (facultatif)
    //----------------------------------------------------------------------
    TiXmlElement * serviceProviderEl = new TiXmlElement ( "ows:ServiceProvider" );

    serviceProviderEl->LinkEndChild ( buildTextNode ( "ows:ProviderName",servicesConf.getServiceProvider() ) );
    TiXmlElement * providerSiteEl = new TiXmlElement ( "ows:ProviderSite" );
    providerSiteEl->SetAttribute ( "xlink:href",servicesConf.getProviderSite() );
    serviceProviderEl->LinkEndChild ( providerSiteEl );

    TiXmlElement * serviceContactEl = new TiXmlElement ( "ows:ServiceContact" );

    serviceContactEl->LinkEndChild ( buildTextNode ( "ows:IndividualName",servicesConf.getIndividualName() ) );
    serviceContactEl->LinkEndChild ( buildTextNode ( "ows:PositionName",servicesConf.getIndividualPosition() ) );

    TiXmlElement * contactInfoEl = new TiXmlElement ( "ows:ContactInfo" );
    TiXmlElement * contactInfoPhoneEl = new TiXmlElement ( "ows:Phone" );

    contactInfoPhoneEl->LinkEndChild ( buildTextNode ( "ows:Voice",servicesConf.getVoice() ) );
    contactInfoPhoneEl->LinkEndChild ( buildTextNode ( "ows:Facsimile",servicesConf.getFacsimile() ) );

    contactInfoEl->LinkEndChild ( contactInfoPhoneEl );

    TiXmlElement * contactAddressEl = new TiXmlElement ( "ows:Address" );
    //contactAddressEl->LinkEndChild(buildTextNode("AddressType","type"));
    contactAddressEl->LinkEndChild ( buildTextNode ( "ows:DeliveryPoint",servicesConf.getDeliveryPoint() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "ows:City",servicesConf.getCity() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "ows:AdministrativeArea",servicesConf.getAdministrativeArea() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "ows:PostalCode",servicesConf.getPostCode() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "ows:Country",servicesConf.getCountry() ) );
    contactAddressEl->LinkEndChild ( buildTextNode ( "ows:ElectronicMailAddress",servicesConf.getElectronicMailAddress() ) );
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
    allowedValuesEl->LinkEndChild ( buildTextNode ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    if ( servicesConf.isPostEnabled() ) {
        TiXmlElement * postEl = new TiXmlElement ( "ows:Post" );
        postEl->SetAttribute ( "xlink:href","]HOSTNAME/PATH[" );
        constraintEl = new TiXmlElement ( "ows:Constraint" );
        constraintEl->SetAttribute ( "name","PostEncoding" );
        allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
        allowedValuesEl->LinkEndChild ( buildTextNode ( "ows:Value", "XML" ) );
        //TODO Implement SOAP like request
        //allowedValuesEl->LinkEndChild(buildTextNode("ows:Value", "SOAP"));
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
    allowedValuesEl->LinkEndChild ( buildTextNode ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    if ( servicesConf.isPostEnabled() ) {
        TiXmlElement * postEl = new TiXmlElement ( "ows:Post" );
        postEl->SetAttribute ( "xlink:href","]HOSTNAME/PATH[" );
        constraintEl = new TiXmlElement ( "ows:Constraint" );
        constraintEl->SetAttribute ( "name","PostEncoding" );
        allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
        allowedValuesEl->LinkEndChild ( buildTextNode ( "ows:Value", "XML" ) );
        //TODO Implement SOAP like request
        //allowedValuesEl->LinkEndChild(buildTextNode("ows:Value", "SOAP"));
        constraintEl->LinkEndChild ( allowedValuesEl );
        postEl->LinkEndChild ( constraintEl );
        httpEl->LinkEndChild ( postEl );
    }
    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );


    // Inspire (extended Capability)
    // TODO : en dur. A mettre dans la configuration du service (prevoir differents profils d'application possibles)
    if ( servicesConf.isInspire() ) {
        TiXmlElement * extendedCapabilititesEl = new TiXmlElement ( "inspire_vs:ExtendedCapabilities" );

        // MetadataURL
        TiXmlElement * metadataUrlEl = new TiXmlElement ( "inspire_common:MetadataUrl" );
        metadataUrlEl->LinkEndChild ( buildTextNode ( "inspire_common:URL", servicesConf.getWMTSMetadataURL()->getHRef() ) );
        metadataUrlEl->LinkEndChild ( buildTextNode ( "inspire_common:MediaType", servicesConf.getWMTSMetadataURL()->getType() ) );
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
        responseLanguageEl->LinkEndChild ( buildTextNode ( "inspire_common:Language","fre" ) );
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
    std::map<std::string, Layer*>::iterator itLay ( layerList.begin() ), itLayEnd ( layerList.end() );
    for ( ; itLay!=itLayEnd; ++itLay ) {
        TiXmlElement * layerEl=new TiXmlElement ( "Layer" );
        Layer* layer = itLay->second;

        layerEl->LinkEndChild ( buildTextNode ( "ows:Title", layer->getTitle() ) );
        layerEl->LinkEndChild ( buildTextNode ( "ows:Abstract", layer->getAbstract() ) );
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
        wgsBBEl->LinkEndChild ( buildTextNode ( "ows:LowerCorner", os.str() ) );
        os.str ( "" );
        os<<layer->getGeographicBoundingBox().maxx;
        os<<" ";
        os<<layer->getGeographicBoundingBox().maxy;
        wgsBBEl->LinkEndChild ( buildTextNode ( "ows:UpperCorner", os.str() ) );
        os.str ( "" );
        layerEl->LinkEndChild ( wgsBBEl );


        layerEl->LinkEndChild ( buildTextNode ( "ows:Identifier", layer->getId() ) );

        //Style
        if ( layer->getStyles().size() != 0 ) {
            for ( unsigned int i=0; i < layer->getStyles().size(); i++ ) {
                TiXmlElement * styleEl= new TiXmlElement ( "Style" );
                if ( i==0 ) styleEl->SetAttribute ( "isDefault","true" );
                Style* style = layer->getStyles() [i];
                int j;
                for ( j=0 ; j < style->getTitles().size(); ++j ) {
                    LOGGER_DEBUG ( _ ( "Title : " ) << style->getTitles() [j].c_str() );
                    styleEl->LinkEndChild ( buildTextNode ( "ows:Title", style->getTitles() [j].c_str() ) );
                }
                for ( j=0 ; j < style->getAbstracts().size(); ++j ) {
                    LOGGER_DEBUG ( _ ( "Abstract : " ) << style->getAbstracts() [j].c_str() );
                    styleEl->LinkEndChild ( buildTextNode ( "ows:Abstract", style->getAbstracts() [j].c_str() ) );
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
                    //kwlEl->LinkEndChild ( buildTextNode ( "ows:Keyword", ROK4_INFO ) );
                    styleEl->LinkEndChild ( kwlEl );
                }

                styleEl->LinkEndChild ( buildTextNode ( "ows:Identifier", style->getId() ) );
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
        layerEl->LinkEndChild ( buildTextNode ( "Format",Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) ) );

        /* on suppose qu'on a qu'un TMS par layer parce que si on admet avoir un TMS par pyramide
         *  il faudra contrôler la cohérence entre le format, la projection et le TMS... */
        TiXmlElement * tmsLinkEl = new TiXmlElement ( "TileMatrixSetLink" );
        tmsLinkEl->LinkEndChild ( buildTextNode ( "TileMatrixSet",layer->getDataPyramid()->getTms().getId() ) );
        usedTMSList.insert ( std::pair<std::string,TileMatrixSet> ( layer->getDataPyramid()->getTms().getId() , layer->getDataPyramid()->getTms() ) );
        //tileMatrixSetLimits
        TiXmlElement * tmsLimitsEl = new TiXmlElement ( "TileMatrixSetLimits" );

        std::map<std::string, Level*> layerLevelList = layer->getDataPyramid()->getLevels();

        std::map<std::string, Level*>::iterator itLevelList ( layerLevelList.begin() );
        std::map<std::string, Level*>::iterator itLevelListEnd ( layerLevelList.end() );
        for ( ; itLevelList!=itLevelListEnd; ++itLevelList ) {
            Level * level = itLevelList->second;
            TiXmlElement * tmLimitsEl = new TiXmlElement ( "TileMatrixLimits" );
            tmLimitsEl->LinkEndChild ( buildTextNode ( "TileMatrix",level->getTm().getId() ) );

            tmLimitsEl->LinkEndChild ( buildTextNode ( "MinTileRow",numToStr ( ( level->getMinTileRow() <0?0:level->getMinTileRow() ) ) ) );
            tmLimitsEl->LinkEndChild ( buildTextNode ( "MaxTileRow",numToStr ( ( level->getMaxTileRow() <0?level->getTm().getMatrixW() :level->getMaxTileRow() ) ) ) );
            tmLimitsEl->LinkEndChild ( buildTextNode ( "MinTileCol",numToStr ( ( level->getMinTileCol() <0?0:level->getMinTileCol() ) ) ) );
            tmLimitsEl->LinkEndChild ( buildTextNode ( "MaxTileCol",numToStr ( ( level->getMaxTileCol() <0?level->getTm().getMatrixH() :level->getMaxTileCol() ) ) ) );
            tmsLimitsEl->LinkEndChild ( tmLimitsEl );
        }
        tmsLinkEl->LinkEndChild ( tmsLimitsEl );

        layerEl->LinkEndChild ( tmsLinkEl );

        contentsEl->LinkEndChild ( layerEl );
    }

    // TileMatrixSet
    //--------------------------------------------------------
    std::map<std::string,TileMatrixSet>::iterator itTms ( usedTMSList.begin() ), itTmsEnd ( usedTMSList.end() );
    for ( ; itTms!=itTmsEnd; ++itTms ) {

        TileMatrixSet tms = itTms->second;

        TiXmlElement * tmsEl=new TiXmlElement ( "TileMatrixSet" );
        tmsEl->LinkEndChild ( buildTextNode ( "ows:Identifier",tms.getId() ) );
        if ( ! ( tms.getTitle().empty() ) ) {
            tmsEl->LinkEndChild ( buildTextNode ( "ows:Title", tms.getTitle().c_str() ) );
        }

        if ( ! ( tms.getAbstract().empty() ) ) {
            tmsEl->LinkEndChild ( buildTextNode ( "ows:Abstract", tms.getAbstract().c_str() ) );
        }

        if ( tms.getKeyWords()->size() != 0 ) {
            TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
            TiXmlElement * kwEl;
            for ( unsigned int i=0; i < tms.getKeyWords()->size(); i++ ) {
                kwEl = new TiXmlElement ( "ows:Keyword" );
                kwEl->LinkEndChild ( new TiXmlText ( tms.getKeyWords()->at ( i ).getContent() ) );
                const std::map<std::string,std::string>* attributes = tms.getKeyWords()->at ( i ).getAttributes();
                for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                    kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
                }

                kwlEl->LinkEndChild ( kwEl );
            }
            //kwlEl->LinkEndChild ( buildTextNode ( "ows:Keyword", ROK4_INFO ) );
            tmsEl->LinkEndChild ( kwlEl );
        }


        tmsEl->LinkEndChild ( buildTextNode ( "ows:SupportedCRS",tms.getCrs().getRequestCode() ) );
        std::map<std::string, TileMatrix>* tmList = tms.getTmList();

        // TileMatrix
        std::map<std::string, TileMatrix>::iterator itTm ( tmList->begin() ), itTmEnd ( tmList->end() );
        for ( ; itTm!=itTmEnd; ++itTm ) {
            TileMatrix tm =itTm->second;
            TiXmlElement * tmEl=new TiXmlElement ( "TileMatrix" );
            tmEl->LinkEndChild ( buildTextNode ( "ows:Identifier",tm.getId() ) );
            tmEl->LinkEndChild ( buildTextNode ( "ScaleDenominator",doubleToStr ( ( long double ) ( tm.getRes() *tms.getCrs().getMetersPerUnit() ) /0.00028 ) ) );
            tmEl->LinkEndChild ( buildTextNode ( "TopLeftCorner",numToStr ( tm.getX0() ) + " " + numToStr ( tm.getY0() ) ) );
            tmEl->LinkEndChild ( buildTextNode ( "TileWidth",numToStr ( tm.getTileW() ) ) );
            tmEl->LinkEndChild ( buildTextNode ( "TileHeight",numToStr ( tm.getTileH() ) ) );
            tmEl->LinkEndChild ( buildTextNode ( "MatrixWidth",numToStr ( tm.getMatrixW() ) ) );
            tmEl->LinkEndChild ( buildTextNode ( "MatrixHeight",numToStr ( tm.getMatrixH() ) ) );
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


// Compute the number of decimals
//  3.14 -> 2
//  1.0001 -> 4
// Maximum is 10
int Rok4Server::GetDecimalPlaces ( double dbVal ) {
    dbVal = fmod(dbVal, 1);
    static const int MAX_DP = 10;
    double THRES = pow ( 0.1, MAX_DP );
    if ( dbVal == 0.0 )
        return 0;
    int nDecimal = 0;
    while ( dbVal - floor ( dbVal ) > THRES && nDecimal < MAX_DP && ceil(dbVal)-dbVal > THRES) {
        dbVal *= 10.0;
        THRES *= 10.0;
        nDecimal++;
    }
    return nDecimal;
}
