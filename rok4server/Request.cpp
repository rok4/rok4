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

/**
 * \file Request.cpp
 * \~french
 * \brief Implémentation de la classe Request, analysant les requêtes HTTP
 * \~english
 * \brief Implement the Request Class analysing HTTP requests
 */

#include "Request.h"
#include "Message.h"
#include "CRS.h"
#include "Pyramid.h"
#include <cstdlib>
#include <climits>
#include <vector>
#include <cstdio>
#include "tinyxml.h"
#include "config.h"
#include <algorithm>
#include "intl.h"


namespace RequestType {

    const char* const requesttype_name[] = {
        "UNKNOWN",
        "MISSING",
        "GetServices",
        "GetCapabilities",
        "GetLayer",
        "GetLayerMetadata",
        "GetMap",
        "GetTile",
        "GetFeatureInfo",
        "GetVersion"
    };

    std::string toString ( eRequestType rt ) {
        return std::string ( requesttype_name[rt] );
    }
}

namespace ServiceType {

    const char* const servicetype_name[] = {
        "MISSING",
        "UNKNOWN",
        "WMTS",
        "WMS",
        "TMS"
    };

    std::string toString ( eServiceType st ) {
        return std::string ( servicetype_name[st] );
    }
}




/**
 * \~french
 * \brief Convertit un caractère héxadécimal (0-9, A-Z, a-z) en décimal
 * \param[in] hex caractère
 * \return 0xFF sur une entrée invalide
 * \~english
 * \brief Converts hex char (0-9, A-Z, a-z) to decimal.
 * \param[in] hex character
 * \return 0xFF on invalid input.
 */
char hex2int ( unsigned char hex ) {
    hex = hex - '0';
    // Si hex <= 9 on a le résultat
    //   Sinon
    if ( hex > 9 ) {
        hex = ( hex + '0' - 1 ) | 0x20; // Pour le passage des majuscules aux minuscules dans la table ASCII
        hex = hex - 'a' + 11;
    }
    if ( hex > 15 ) // En cas d'erreur
        hex = 0xFF;

    return hex;
}

void Request::url_decode ( char *src ) {
    unsigned char high, low;
    char* dst = src;

    while ( ( *src ) != '\0' ) {
        if ( *src == '+' ) {
            *dst = ' ';
        } else if ( *src == '%' ) {
            *dst = '%';

            high = hex2int ( * ( src + 1 ) );
            if ( high != 0xFF ) {
                low = hex2int ( * ( src + 2 ) );
                if ( low != 0xFF ) {
                    high = ( high << 4 ) | low;

                    /* map control-characters out */
                    if ( high < 32 || high == 127 ) high = '_';

                    *dst = high;
                    src += 2;
                }
            }
        } else {
            *dst = *src;
        }

        dst++;
        src++;
    }

    *dst = '\0';
}

/**
 * \~french
 * \brief Supprime l'espace de nom (la partie avant :) de la balise XML
 * \param[in,out] elementName le nom de la balise
 * \~english
 * \brief Remove the namespace (before :) in the XML element
 * \param[in,out] elementName the element name
 */
void removeNameSpace ( std::string& elementName ) {
    size_t pos = elementName.find ( ":" );
    if ( elementName.size() <= pos ) {
        return;
    }
    // Garde le ":" -> "left:right" devient ":right"
    elementName.erase ( elementName.begin(),elementName.begin() +pos );
}

/**
 * \~french
 * \brief Analyse des paramètres d'une requête GetCapabilities en POST XML
 * \param[in] hGetCap élement XML de la requête
 * \param[in,out] parameters liste associative des paramètres
 * \~english
 * \brief Parse a GetCapabilities request in POST XML
 * \param[in] hGetCap request XML element
 * \param[in,out] parameters associative parameters list
 */
void parseGetCapabilitiesPost ( TiXmlHandle& hGetCap, std::map< std::string, std::string >& parameters ) {
    BOOST_LOG_TRIVIAL(debug) <<  _ ( "Parse GetCapabilities Request" ) ;
    std::string version;
    std::string service;

    parameters.insert ( std::pair<std::string, std::string> ( "request", "getcapabilities" ) );

    TiXmlElement* pElem = hGetCap.ToElement();

    if ( pElem->QueryStringAttribute ( "service",&service ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No service attribute" ) ;
        return;
    }

    std::transform ( service.begin(), service.end(), service.begin(), tolower );

    parameters.insert ( std::pair<std::string, std::string> ( "service", service ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No version attribute" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );

}
/**
 * \~french
 * \brief Analyse des paramètres d'une requête GetTile en POST XML
 * \param[in] hGetTile élement XML de la requête
 * \param[in,out] parameters liste associative des paramètres
 * \~english
 * \brief Parse a GetTile request in POST XML
 * \param[in] hGetTile request XML element
 * \param[in,out] parameters associative parameters list
 */
void parseGetTilePost ( TiXmlHandle& hGetTile, std::map< std::string, std::string >& parameters ) {
    BOOST_LOG_TRIVIAL(debug) <<  _ ( "Parse GetTile Request" ) ;
    TiXmlElement* pElem = hGetTile.ToElement();
    std::string version;
    std::string service;

    parameters.insert ( std::pair<std::string, std::string> ( "request", "gettile" ) );

    if ( pElem->QueryStringAttribute ( "service",&service ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No service attribute" ) ;
        return;
    }
    std::transform ( service.begin(), service.end(), service.begin(), tolower );
    parameters.insert ( std::pair<std::string, std::string> ( "service", service ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No version attribute" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );
    //Layer
    pElem = hGetTile.FirstChildElement().ToElement();

    if ( !pElem && pElem->ValueStr().find ( "Layer" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No Layer" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "layer", DocumentXML::getTextStrFromElem(pElem) ) );

    //Style
    pElem = pElem->NextSiblingElement();

    if ( !pElem && pElem->ValueStr().find ( "Style" ) == std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No Style" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "style", DocumentXML::getTextStrFromElem(pElem) ) );

    //Format
    pElem = pElem->NextSiblingElement();

    if ( !pElem && pElem->ValueStr().find ( "Format" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No Format" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "format", DocumentXML::getTextStrFromElem(pElem) ) );

    //DimensionNameValue name="NAME" OPTIONAL
    pElem = pElem->NextSiblingElement();
    while ( pElem && pElem->ValueStr().find ( "DimensionNameValue" ) !=std::string::npos && pElem->GetText() ) {
        std::string dimensionName;
        if ( pElem->QueryStringAttribute ( "name",&dimensionName ) != TIXML_SUCCESS ) {
            BOOST_LOG_TRIVIAL(debug) <<  _ ( "No Name attribute" ) ;
            continue;
        } else {
            parameters.insert ( std::pair<std::string, std::string> ( dimensionName, DocumentXML::getTextStrFromElem(pElem) ) );
        }
        pElem = pElem->NextSiblingElement();
    }

    //TileMatrixSet
    if ( !pElem && pElem->ValueStr().find ( "TileMatrixSet" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No TileMatrixSet" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilematrixset", DocumentXML::getTextStrFromElem(pElem) ) );

    //TileMatrix
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileMatrix" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No TileMatrix" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilematrix", DocumentXML::getTextStrFromElem(pElem) ) );

    //TileRow
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileRow" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No TileRow" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilerow", DocumentXML::getTextStrFromElem(pElem) ) );
    //TileCol
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileCol" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No TileCol" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilecol", DocumentXML::getTextStrFromElem(pElem) ) );

    pElem = pElem->NextSiblingElement();
    // "VendorOption"
    while ( pElem ) {
        if ( pElem->ValueStr().find ( "VendorOption" ) !=std::string::npos && pElem->Attribute ( "name" ) ) {
            BOOST_LOG_TRIVIAL(debug) <<  _ ( "VendorOption" ) ;
            std::string vendorOpt = pElem->Attribute ( "name" );
            std::transform ( vendorOpt.begin(), vendorOpt.end(), vendorOpt.begin(), ::tolower );
            parameters.insert ( std::pair<std::string, std::string> ( vendorOpt, ( pElem->GetText() ?DocumentXML::getTextStrFromElem(pElem) :"true" ) ) );
        }
        pElem =  pElem->NextSiblingElement();
    }

    pElem = pElem->NextSiblingElement();
    // "VendorOption"
    while ( pElem ) {
        if ( pElem->ValueStr().find ( "VendorOption" ) !=std::string::npos && pElem->Attribute ( "name" ) ) {
            BOOST_LOG_TRIVIAL(debug) <<  "VendorOption" ;
            std::string vendorOpt = pElem->Attribute ( "name" );
            std::transform ( vendorOpt.begin(), vendorOpt.end(), vendorOpt.begin(), ::tolower );
            parameters.insert ( std::pair<std::string, std::string> ( vendorOpt, ( pElem->GetText() ?pElem->GetText() :"true" ) ) );
        }
        pElem =  pElem->NextSiblingElement();
    }

}
/**
 * \~french
 * \brief Analyse des paramètres d'une requête GetMap en POST XML
 * \param[in] hGetMap élement XML de la requête
 * \param[in,out] parameters liste associative des paramètres
 * \~english
 * \brief Parse a GetMap request in POST XML
 * \param[in] hGetMap request XML element
 * \param[in,out] parameters associative parameters list
 */
void parseGetMapPost ( TiXmlHandle& hGetMap, std::map< std::string, std::string >& parameters ) {
    BOOST_LOG_TRIVIAL(debug) <<  _ ( "Parse GetMap Request" ) ;
    TiXmlElement* pElem = hGetMap.ToElement();
    std::string version;
    std::string sldVersion;
    std::string layers;
    std::string styles;
    std::string bbox;
    std::stringstream format_options;

    parameters.insert ( std::pair<std::string, std::string> ( "service", "wms" ) );
    parameters.insert ( std::pair<std::string, std::string> ( "request", "getmap" ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No version attribute" ) ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );

    //StyledLayerDescriptor Layers and Style

    pElem=hGetMap.FirstChildElement().ToElement();
    if ( !pElem && pElem->ValueStr().find ( "StyledLayerDescriptor" ) ==std::string::npos ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No StyledLayerDescriptor" ) ;
        return;
    }
    pElem=hGetMap.FirstChildElement().FirstChildElement().ToElement();
    if ( !pElem ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "Empty StyledLayerDescriptor" ) ;
        return;
    }

    //NamedLayer
    while ( pElem ) {

        //NamedLayer test
        if ( pElem->ValueStr().find ( "NamedLayer" ) ==std::string::npos ) {

            pElem= pElem->NextSiblingElement();
            continue;
        }

        TiXmlElement* pElemNamedLayer = pElem->FirstChildElement();
        if ( !pElemNamedLayer && pElemNamedLayer->ValueStr().find ( "Name" ) ==std::string::npos && !pElemNamedLayer->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<  _ ( "NamedLayer without Name" ) ;
            return;
        }
        if ( layers.size() >0 ) {
            layers.append ( "," );
            styles.append ( "," );
        }
        layers.append ( pElemNamedLayer->GetText() );


        while ( pElemNamedLayer->ValueStr().find ( "NamedStyle" ) ==std::string::npos ) {
            pElemNamedLayer = pElemNamedLayer->NextSiblingElement();
            if ( !pElemNamedLayer ) {
                BOOST_LOG_TRIVIAL(debug) <<  _ ( "NamedLayer without NamedStyle, use default style" ) ;
                break;
            }
        }
        pElemNamedLayer = pElemNamedLayer->FirstChildElement();
        if ( !pElemNamedLayer && pElemNamedLayer->ValueStr().find ( "Name" ) ==std::string::npos && !pElemNamedLayer->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<  _ ( "NamedStyle without Name" ) ;
            return;
        } else {
            styles.append ( pElemNamedLayer->GetText() );
        }
        pElem= pElem->NextSiblingElement ( pElem->ValueStr() );

    }
    if ( layers.size() <=0 ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "StyledLayerDescriptor without NamedLayer" ) ;
        return;
    }

    parameters.insert ( std::pair<std::string, std::string> ( "layers", layers ) );
    parameters.insert ( std::pair<std::string, std::string> ( "styles", styles ) );

    //CRS
    pElem=hGetMap.FirstChildElement().ToElement()->NextSiblingElement();
    ;
    if ( !pElem && pElem->ValueStr().find ( "CRS" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No CRS" ) ;
        return;
    }

    parameters.insert ( std::pair<std::string, std::string> ( "crs", DocumentXML::getTextStrFromElem(pElem) ) );

    //BoundingBox

    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "BoundingBox" ) ==std::string::npos ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "No BoundingBox" ) ;
        return;
    }

    //FIXME crs attribute might be different from CRS in the request
    //eg output image in EPSG:3857 BBox defined in EPSG:4326
    TiXmlElement * pElemBBox = pElem->FirstChildElement();
    if ( !pElemBBox && pElemBBox->ValueStr().find ( "LowerCorner" ) ==std::string::npos && !pElemBBox->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "BoundingBox Invalid" ) ;
        return;
    }

    bbox.append ( pElemBBox->GetText() );
    bbox.replace ( bbox.find ( " " ),1,"," );
    bbox.append ( "," );

    pElemBBox = pElemBBox->NextSiblingElement();
    if ( !pElemBBox && pElemBBox->ValueStr().find ( "UpperCorner" ) ==std::string::npos && !pElemBBox->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<  _ ( "BoundingBox Invalid" ) ;
        return;
    }
    bbox.append ( pElemBBox->GetText() );
    bbox.replace ( bbox.find ( " " ),1,"," );

    parameters.insert ( std::pair<std::string, std::string> ( "bbox", bbox ) );

    //Output
    {
        pElem = pElem->NextSiblingElement();
        if ( !pElem && pElem->ValueStr().find ( "Output" ) ==std::string::npos ) {
            BOOST_LOG_TRIVIAL(debug) <<  _ ( "No Output" ) ;
            return;
        }
        TiXmlElement * pElemOut =  pElem->FirstChildElement();
        if ( !pElemOut && pElemOut->ValueStr().find ( "Size" ) ==std::string::npos ) {
            BOOST_LOG_TRIVIAL(debug) <<  _ ( "Output Invalid no Size" ) ;
            return;
        }
        TiXmlElement * pElemOutTmp =  pElemOut->FirstChildElement();
        if ( !pElemOutTmp && pElemOutTmp->ValueStr().find ( "Width" ) ==std::string::npos && !pElemOutTmp->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<  _ ( "Output Invalid, Width incorrect" ) ;
            return;
        }
        parameters.insert ( std::pair<std::string, std::string> ( "width", pElemOutTmp->GetText() ) );

        pElemOutTmp =  pElemOutTmp->NextSiblingElement();
        if ( !pElemOutTmp && pElemOutTmp->ValueStr().find ( "Height" ) ==std::string::npos && !pElemOutTmp->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<  "Output Invalid, Height incorrect" ;
            return;
        }
        parameters.insert ( std::pair<std::string, std::string> ( "height", pElemOutTmp->GetText() ) );

        pElemOut =  pElemOut->NextSiblingElement();
        if ( !pElemOut && pElemOut->ValueStr().find ( "Format" ) ==std::string::npos && !pElemOut->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<  _ ( "Output Invalid no Format" ) ;
            return;
        }

        parameters.insert ( std::pair<std::string, std::string> ( "format", pElemOut->GetText() ) );

        pElemOut =  pElemOut->NextSiblingElement();

        if ( pElemOut ) {

            if ( pElemOut->ValueStr().find ( "Transparent" ) !=std::string::npos && pElemOut->GetText() ) {

                parameters.insert ( std::pair<std::string, std::string> ( "transparent", pElemOut->GetText() ) );
                pElemOut =  pElemOut->NextSiblingElement();
            }

        }
        if ( pElemOut ) {
            if ( pElemOut->ValueStr().find ( "BGcolor" ) !=std::string::npos && pElemOut->GetText() ) {

                parameters.insert ( std::pair<std::string, std::string> ( "bgcolor", pElemOut->GetText() ) );
                pElemOut =  pElemOut->NextSiblingElement();
            }
        }
        // "VendorOption"
        while ( pElemOut ) {
            if ( pElemOut->ValueStr().find ( "VendorOption" ) !=std::string::npos && pElemOut->Attribute ( "name" ) && pElemOut->GetText() ) {
                if ( format_options.str().size() >0 ) {
                    format_options << ";";
                }
                format_options <<  pElemOut->Attribute ( "name" ) << ":"<< pElemOut->GetText();

            }
            pElemOut =  pElemOut->NextSiblingElement();
        }

    }

    // Handle output format options
    if ( format_options.str().size() > 0 ) {
        parameters.insert ( std::pair<std::string, std::string> ( "format_options", format_options.str() ) );
    }

    //OPTIONAL

    //Exceptions
    pElem = pElem->NextSiblingElement();
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Exceptions" ) !=std::string::npos && pElem->GetText() ) {
            parameters.insert ( std::pair<std::string, std::string> ( "exceptions", DocumentXML::getTextStrFromElem(pElem) ) );
            pElem = pElem->NextSiblingElement();
        }
    }

    //Time
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Time" ) !=std::string::npos && pElem->GetText() ) {
            parameters.insert ( std::pair<std::string, std::string> ( "time", DocumentXML::getTextStrFromElem(pElem) ) );
            pElem = pElem->NextSiblingElement();
        }
    }

    //Elevation
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Elevation" ) !=std::string::npos ) {
            pElem = pElem->FirstChildElement();
            if ( !pElem ) {
                BOOST_LOG_TRIVIAL(debug) <<  _ ( "Elevation incorrect" ) ;
                return;
            }
            std::string elevation;
            if ( pElem->ValueStr().find ( "Interval" ) !=std::string::npos ) {
                pElem = pElem->FirstChildElement();
                if ( !pElem && pElem->ValueStr().find ( "Min" ) ==std::string::npos && !pElem->GetText() ) {
                    return;
                }
                elevation.append ( DocumentXML::getTextStrFromElem(pElem) );
                elevation.append ( "/" );
                pElem = pElem->NextSiblingElement();
                if ( !pElem && pElem->ValueStr().find ( "Max" ) ==std::string::npos && !pElem->GetText() ) {
                    return;
                }
                elevation.append ( DocumentXML::getTextStrFromElem(pElem) );

            } else if ( pElem->ValueStr().find ( "Value" ) !=std::string::npos ) {
                while ( pElem && pElem->GetText() ) {
                    if ( elevation.size() >0 ) {
                        elevation.append ( "," );
                    }
                    elevation.append ( DocumentXML::getTextStrFromElem(pElem) );
                    pElem = pElem->NextSiblingElement ( pElem->ValueStr() );
                }
            }
            parameters.insert ( std::pair<std::string, std::string> ( "elevation", elevation ) );
        }
    }

}

/**
 * \~french
 * \brief Analyse des paramètres d'une requête POST XML
 * \param[in] content contenu de la requête POST
 * \param[in,out] parameters liste associative des paramètres
 * \todo HTTP POST de type KVP
 * \todo HTTP POST de type XML/Soap
 * \~english
 * \brief Parse a POST XML request
 * \param[in] content POST request content
 * \param[in,out] parameters associative parameters list
 * \todo HTTP POST, KVP style
 * \todo HTTP POST, XML/Soap style
 */
void parsePostContent ( std::string content, std::map< std::string, std::string >& parameters ) {
    TiXmlDocument doc ( "request" );
    content.append ( "\n" );
    if ( !doc.Parse ( content.c_str() ) ) {
        BOOST_LOG_TRIVIAL(info) <<  _ ( "POST request with invalid content" ) ;
        return;
    }
    TiXmlElement *rootEl = doc.FirstChildElement();
    if ( !rootEl ) {
        BOOST_LOG_TRIVIAL(info) <<  _ ( "Cannot retrieve XML root element" ) ;
        return;
    }
    //TODO Unfold Soap envelope
    std::string value = rootEl->ValueStr();
    //  removeNameSpace(value);
    TiXmlHandle hRoot ( rootEl );

    if ( value.find ( "GetCapabilities" ) !=std::string::npos ) {
        parseGetCapabilitiesPost ( hRoot,parameters );
    } else if ( value.find ( "GetMap" ) !=std::string::npos ) {
        parseGetMapPost ( hRoot,parameters );
    } else if ( value.find ( "GetTile" ) !=std::string::npos ) {
        parseGetTilePost ( hRoot,parameters );
    }//TODO Support other request DescribeLayer GetFeatureInfo ...

}

void Request::determineServiceAndRequest() {

    // SERVICE
    std::map<std::string, std::string>::iterator it = params.find ( "service" );
    if ( it == params.end() ) {
        // Dans le cas du TMS, on n'a pas de paramètres.
        // On va donc admettre qu'en l'absence total de paramètres, on est implicitement en TMS
        if (params.size() == 0) {
            service = ServiceType::TMS;
        }
    } else {
        std::string str_service = it->second;
        std::transform(str_service.begin(), str_service.end(), str_service.begin(), ::tolower);
        if (str_service == "wms") {
            service = ServiceType::WMS;
        }
        else if (str_service == "wmts") {
            service = ServiceType::WMTS;
        }
        else if (str_service != "") {
            service = ServiceType::SERVICE_UNKNOWN;
        }
    }

    // Il y a un cas où le service n'a pas été identifié mais doit pourtant être reconnu
    // En WMS 1.1.1, on peut ne pas mettre le service et la requête map ou getamp suffit
    // On identifiera donc le service dans la partie sur la requête

    // REQUETE
    // On teste également la cohérence de la requête avec le service
    it = params.find ( "request" );
    if ( it == params.end() ) {
        // Dans le cas du TMS, on n'a pas de paramètres.
        // On va donc admettre qu'en l'absence total de paramètres, on est implicitement en TMS
        // C'est la profondeur du path à partir de la version 1.0.0 qui va permettre d'identifier
        // le tpye de requête
        if (params.size() == 0 && service == ServiceType::TMS) {
            std::stringstream ss(path);
            std::string token;
            char delim = '/';
            int tmsVersionPos = -1;
            std::vector<std::string> pathParts;
            while (std::getline(ss, token, delim)) {
                if (token == "1.0.0") {
                    tmsVersionPos = pathParts.size();
                }
                pathParts.push_back(token);
            }

            if (tmsVersionPos == -1) {
                // La version n'a pas été rencontrée, on n'est pas dans le cas d'une consultation TMS
                // On va alors considérer que l'on veut la liste des services disponibles sur ce serveur
                // On reste en TMS comme service
                request = RequestType::GETSERVICES;
            }

            else if (tmsVersionPos == pathParts.size() - 1) {
                // la version est en dernière position, on veut le "GetCapabilities" TMS
                request = RequestType::GETCAPABILITIES;
            }

            else if (tmsVersionPos == pathParts.size() - 2) {
                // la version est en avant dernière position, on veut le détail de la couche
                request = RequestType::GETLAYER;
            }

            else if (tmsVersionPos == pathParts.size() - 3 && pathParts.back() == "metadata.json") {
                // la version est en avant dernière position, on veut le détail de la couche
                request = RequestType::GETLAYERMETADATA;
            }

            else if (tmsVersionPos == pathParts.size() - 5) {
                // on requête une tuile
                request = RequestType::GETTILE;
            } else {
                // La profondeur de requête ne permet pas de savoir l'action demandée -> ERREUR
                request = RequestType::REQUEST_UNKNOWN;
            }
        }
    } else {
        std::string str_request = it->second;
        std::transform(str_request.begin(), str_request.end(), str_request.begin(), ::tolower);
        if (str_request == "getmap" || str_request == "map") {
            if (service == ServiceType::SERVICE_MISSING || service == ServiceType::WMS) {
                // On confirme le service WMS (potentiellement non connu dans le cas 1.1.1)
                service = ServiceType::WMS;
                request = RequestType::GETMAP;
            } else {
                // On a une requête getmap avec un service qui ne le gère pas
                request = RequestType::REQUEST_UNKNOWN;
            }
        }
        else if (str_request == "gettile") {
            if (service != ServiceType::WMTS) {
                // On a une requête gettile avec un service inconnu, ou qui ne le gère pas,
                // ou pas comme ça (TMS)
                request = RequestType::REQUEST_UNKNOWN;
            } else {
                request = RequestType::GETTILE;
            }
        }
        else if (str_request == "getfeatureinfo") {
            if (service != ServiceType::WMS && service != ServiceType::WMTS) {
                // On a une requête getfeatureinfo avec un service inconnu ou qui ne le gère pas
                request = RequestType::REQUEST_UNKNOWN;
            } else {
                request = RequestType::GETFEATUREINFO;
            }
        }
        else if (str_request == "getcapabilities" || str_request == "capabilities") {
            if (service != ServiceType::WMTS && service != ServiceType::WMS) {
                // On a une requête getcapabilities avec un service inconnu, 
                // ou qui ne le gère pas, ou pas comme ça (TMS)
                request = RequestType::REQUEST_UNKNOWN;
            } else {
                request = RequestType::GETCAPABILITIES;
            }
        }
        else if (str_request == "getversion") {
            if (service != ServiceType::WMTS && service != ServiceType::WMS) {
                // On a une requête getcapabilities avec un service inconnu, 
                // ou qui ne le gère pas, ou pas comme ça (TMS)
                request = RequestType::REQUEST_UNKNOWN;
            } else {
                request = RequestType::GETVERSION;
            }
        }
        else if (str_request != "") {
            request = RequestType::REQUEST_UNKNOWN;
        }

    }
}

Request::Request ( char* strquery, char* hostName, char* path, char* https ) : 
    hostName ( hostName ),path ( path ), service(ServiceType::SERVICE_MISSING), request(RequestType::REQUEST_MISSING)
{
    BOOST_LOG_TRIVIAL(debug) <<  "QUERY="<<strquery ;
    if ( https && (strcmp ( https,"on" ) == 0 || strcmp ( https,"ON" ) ==0) ){
        scheme = "https://";
    } else {
        scheme = "http://";
    }

    url_decode ( strquery );

    for ( int pos = 0; strquery[pos]; ) {
        char* key = strquery + pos;
        for ( ; strquery[pos] && strquery[pos] != '=' && strquery[pos] != '&'; pos++ ); // on trouve le premier "=", "&" ou 0
        char* value = strquery + pos;
        for ( ; strquery[pos] && strquery[pos] != '&'; pos++ ); // on trouve le suivant "&" ou 0
        if ( *value == '=' ) *value++ = 0; // on met un 0 à la place du '=' entre key et value
        if ( strquery[pos] ) strquery[pos++] = 0; // on met un 0 à la fin du char* value

        toLowerCase ( key );
        params.insert ( std::pair<std::string, std::string> ( key, value ) );
    }

    determineServiceAndRequest();

}


Request::Request ( char* strquery, char* hostName, char* path, char* https, std::string postContent ) : 
    hostName ( hostName ),path ( path ), service(ServiceType::SERVICE_MISSING), request(RequestType::REQUEST_MISSING)
{
    BOOST_LOG_TRIVIAL(debug) <<  "QUERY="<<strquery ;
    if ( https && (strcmp ( https,"on" ) == 0 || strcmp ( https,"ON" ) ==0) ){
        scheme = "https://";
    } else {
        scheme = "http://";
    }

    if ( !postContent.empty() ) {
        parsePostContent ( postContent,params );
    }

    determineServiceAndRequest();

}


Request::~Request() {}

bool Request::hasParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = params.find ( paramName );
    if ( it == params.end() ) {
        return false;
    }
    return true;
}

std::string Request::getParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = params.find ( paramName );
    if ( it == params.end() ) {
        return "";
    }
    return it->second;
}
    