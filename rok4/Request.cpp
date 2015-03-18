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
#include "tinystr.h"
#include "config.h"
#include <algorithm>
#include "intl.h"

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

/**
 * \~french
 * \brief Découpe une chaîne de caractères selon un délimiteur
 * \param[in] s la chaîne à découper
 * \param[in] delim le délimiteur
 * \param[in,out] elems la liste contenant les parties de la chaîne
 * \return la liste contenant les parties de la chaîne
 * \~english
 * \brief Split a string using a specified delimitor
 * \param[in] s the string to split
 * \param[in] delim the delimitor
 * \param[in,out] elems the list with the splited string
 * \return the list with the splited string
 */
std::vector<std::string> &split ( const std::string &s, char delim, std::vector<std::string> &elems ) {
    std::stringstream ss ( s );
    std::string item;
    while ( std::getline ( ss, item, delim ) ) {
        elems.push_back ( item );
    }
    return elems;
}

/**
 * \~french
 * \brief Découpe une chaîne de caractères selon un délimiteur
 * \param[in] s la chaîne à découper
 * \param[in] delim le délimiteur
 * \return la liste contenant les parties de la chaîne
 * \~english
 * \brief Split a string using a specified delimitor
 * \param[in] s the string to split
 * \param[in] delim the delimitor
 * \return the list with the splited string
 */
std::vector<std::string> split ( const std::string &s, char delim ) {
    std::vector<std::string> elems;
    return split ( s, delim, elems );
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
 * \brief Transforme la chaîne de caractères en minuscule
 * \param[in,out] str la chaîne
 * \~english
 * \brief Translate the string to lower case
 * \param[in,out] str the string
 */
void toLowerCase ( char* str ) {
    if ( str ) for ( int i = 0; str[i]; i++ ) str[i] = tolower ( str[i] );
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
    LOGGER_DEBUG ( _ ( "Parse GetCapabilities Request" ) );
    std::string version;
    std::string service;

    parameters.insert ( std::pair<std::string, std::string> ( "request", "getcapabilities" ) );

    TiXmlElement* pElem = hGetCap.ToElement();

    if ( pElem->QueryStringAttribute ( "service",&service ) != TIXML_SUCCESS ) {
        LOGGER_DEBUG ( _ ( "No service attribute" ) );
        return;
    }

    std::transform ( service.begin(), service.end(), service.begin(), tolower );

    parameters.insert ( std::pair<std::string, std::string> ( "service", service ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        LOGGER_DEBUG ( _ ( "No version attribute" ) );
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
    LOGGER_DEBUG ( _ ( "Parse GetTile Request" ) );
    TiXmlElement* pElem = hGetTile.ToElement();
    std::string version;
    std::string service;

    parameters.insert ( std::pair<std::string, std::string> ( "request", "gettile" ) );

    if ( pElem->QueryStringAttribute ( "service",&service ) != TIXML_SUCCESS ) {
        LOGGER_DEBUG ( _ ( "No service attribute" ) );
        return;
    }
    std::transform ( service.begin(), service.end(), service.begin(), tolower );
    parameters.insert ( std::pair<std::string, std::string> ( "service", service ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        LOGGER_DEBUG ( _ ( "No version attribute" ) );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );
    //Layer
    pElem = hGetTile.FirstChildElement().ToElement();

    if ( !pElem && pElem->ValueStr().find ( "Layer" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( _ ( "No Layer" ) );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "layer", pElem->GetTextStr() ) );

    //Style
    pElem = pElem->NextSiblingElement();

    if ( !pElem && pElem->ValueStr().find ( "Style" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( _ ( "No Style" ) );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "style", pElem->GetTextStr() ) );

    //Format
    pElem = pElem->NextSiblingElement();

    if ( !pElem && pElem->ValueStr().find ( "Format" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( _ ( "No Format" ) );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "format", pElem->GetTextStr() ) );

    //DimensionNameValue name="NAME" OPTIONAL
    pElem = pElem->NextSiblingElement();
    while ( pElem && pElem->ValueStr().find ( "DimensionNameValue" ) !=std::string::npos && pElem->GetText() ) {
        std::string dimensionName;
        if ( pElem->QueryStringAttribute ( "name",&dimensionName ) != TIXML_SUCCESS ) {
            LOGGER_DEBUG ( _ ( "No Name attribute" ) );
            continue;
        } else {
            parameters.insert ( std::pair<std::string, std::string> ( dimensionName, pElem->GetTextStr() ) );
        }
        pElem = pElem->NextSiblingElement();
    }

    //TileMatrixSet
    if ( !pElem && pElem->ValueStr().find ( "TileMatrixSet" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( _ ( "No TileMatrixSet" ) );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilematrixset", pElem->GetTextStr() ) );

    //TileMatrix
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileMatrix" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( _ ( "No TileMatrix" ) );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilematrix", pElem->GetTextStr() ) );

    //TileRow
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileRow" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( _ ( "No TileRow" ) );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilerow", pElem->GetTextStr() ) );
    //TileCol
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileCol" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( _ ( "No TileCol" ) );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilecol", pElem->GetTextStr() ) );

    pElem = pElem->NextSiblingElement();
    // "VendorOption"
    while ( pElem ) {
        if ( pElem->ValueStr().find ( "VendorOption" ) !=std::string::npos && pElem->Attribute ( "name" ) ) {
            LOGGER_DEBUG ( _ ( "VendorOption" ) );
            std::string vendorOpt = pElem->Attribute ( "name" );
            std::transform ( vendorOpt.begin(), vendorOpt.end(), vendorOpt.begin(), ::tolower );
            parameters.insert ( std::pair<std::string, std::string> ( vendorOpt, ( pElem->GetText() ?pElem->GetTextStr() :"true" ) ) );
        }
        pElem =  pElem->NextSiblingElement();
    }

    pElem = pElem->NextSiblingElement();
    // "VendorOption"
    while ( pElem ) {
        if ( pElem->ValueStr().find ( "VendorOption" ) !=std::string::npos && pElem->Attribute ( "name" ) ) {
            LOGGER_DEBUG ( "VendorOption" );
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
    LOGGER_DEBUG ( _ ( "Parse GetMap Request" ) );
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
        LOGGER_DEBUG ( _ ( "No version attribute" ) );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );

    //StyledLayerDescriptor Layers and Style

    pElem=hGetMap.FirstChildElement().ToElement();
    if ( !pElem && pElem->ValueStr().find ( "StyledLayerDescriptor" ) ==std::string::npos ) {
        LOGGER_DEBUG ( _ ( "No StyledLayerDescriptor" ) );
        return;
    }
    pElem=hGetMap.FirstChildElement().FirstChildElement().ToElement();
    if ( !pElem ) {
        LOGGER_DEBUG ( _ ( "Empty StyledLayerDescriptor" ) );
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
            LOGGER_DEBUG ( _ ( "NamedLayer without Name" ) );
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
                LOGGER_DEBUG ( _ ( "NamedLayer without NamedStyle, use default style" ) );
                break;
            }
        }
        pElemNamedLayer = pElemNamedLayer->FirstChildElement();
        if ( !pElemNamedLayer && pElemNamedLayer->ValueStr().find ( "Name" ) ==std::string::npos && !pElemNamedLayer->GetText() ) {
            LOGGER_DEBUG ( _ ( "NamedStyle without Name" ) );
            return;
        } else {
            styles.append ( pElemNamedLayer->GetText() );
        }
        pElem= pElem->NextSiblingElement ( pElem->ValueStr() );

    }
    if ( layers.size() <=0 ) {
        LOGGER_DEBUG ( _ ( "StyledLayerDescriptor without NamedLayer" ) );
        return;
    }

    parameters.insert ( std::pair<std::string, std::string> ( "layers", layers ) );
    parameters.insert ( std::pair<std::string, std::string> ( "styles", styles ) );

    //CRS
    pElem=hGetMap.FirstChildElement().ToElement()->NextSiblingElement();
    ;
    if ( !pElem && pElem->ValueStr().find ( "CRS" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( _ ( "No CRS" ) );
        return;
    }

    parameters.insert ( std::pair<std::string, std::string> ( "crs", pElem->GetTextStr() ) );

    //BoundingBox

    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "BoundingBox" ) ==std::string::npos ) {
        LOGGER_DEBUG ( _ ( "No BoundingBox" ) );
        return;
    }

    //FIXME crs attribute might be different from CRS in the request
    //eg output image in EPSG:3857 BBox defined in EPSG:4326
    TiXmlElement * pElemBBox = pElem->FirstChildElement();
    if ( !pElemBBox && pElemBBox->ValueStr().find ( "LowerCorner" ) ==std::string::npos && !pElemBBox->GetText() ) {
        LOGGER_DEBUG ( _ ( "BoundingBox Invalid" ) );
        return;
    }

    bbox.append ( pElemBBox->GetText() );
    bbox.replace ( bbox.find ( " " ),1,"," );
    bbox.append ( "," );

    pElemBBox = pElemBBox->NextSiblingElement();
    if ( !pElemBBox && pElemBBox->ValueStr().find ( "UpperCorner" ) ==std::string::npos && !pElemBBox->GetText() ) {
        LOGGER_DEBUG ( _ ( "BoundingBox Invalid" ) );
        return;
    }
    bbox.append ( pElemBBox->GetText() );
    bbox.replace ( bbox.find ( " " ),1,"," );

    parameters.insert ( std::pair<std::string, std::string> ( "bbox", bbox ) );

    //Output
    {
        pElem = pElem->NextSiblingElement();
        if ( !pElem && pElem->ValueStr().find ( "Output" ) ==std::string::npos ) {
            LOGGER_DEBUG ( _ ( "No Output" ) );
            return;
        }
        TiXmlElement * pElemOut =  pElem->FirstChildElement();
        if ( !pElemOut && pElemOut->ValueStr().find ( "Size" ) ==std::string::npos ) {
            LOGGER_DEBUG ( _ ( "Output Invalid no Size" ) );
            return;
        }
        TiXmlElement * pElemOutTmp =  pElemOut->FirstChildElement();
        if ( !pElemOutTmp && pElemOutTmp->ValueStr().find ( "Width" ) ==std::string::npos && !pElemOutTmp->GetText() ) {
            LOGGER_DEBUG ( _ ( "Output Invalid, Width incorrect" ) );
            return;
        }
        parameters.insert ( std::pair<std::string, std::string> ( "width", pElemOutTmp->GetText() ) );

        pElemOutTmp =  pElemOutTmp->NextSiblingElement();
        if ( !pElemOutTmp && pElemOutTmp->ValueStr().find ( "Height" ) ==std::string::npos && !pElemOutTmp->GetText() ) {
            LOGGER_DEBUG ( "Output Invalid, Height incorrect" );
            return;
        }
        parameters.insert ( std::pair<std::string, std::string> ( "height", pElemOutTmp->GetText() ) );

        pElemOut =  pElemOut->NextSiblingElement();
        if ( !pElemOut && pElemOut->ValueStr().find ( "Format" ) ==std::string::npos && !pElemOut->GetText() ) {
            LOGGER_DEBUG ( _ ( "Output Invalid no Format" ) );
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
            parameters.insert ( std::pair<std::string, std::string> ( "exceptions", pElem->GetTextStr() ) );
            pElem = pElem->NextSiblingElement();
        }
    }

    //Time
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Time" ) !=std::string::npos && pElem->GetText() ) {
            parameters.insert ( std::pair<std::string, std::string> ( "time", pElem->GetTextStr() ) );
            pElem = pElem->NextSiblingElement();
        }
    }

    //Elevation
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Elevation" ) !=std::string::npos ) {
            pElem = pElem->FirstChildElement();
            if ( !pElem ) {
                LOGGER_DEBUG ( _ ( "Elevation incorrect" ) );
                return;
            }
            std::string elevation;
            if ( pElem->ValueStr().find ( "Interval" ) !=std::string::npos ) {
                pElem = pElem->FirstChildElement();
                if ( !pElem && pElem->ValueStr().find ( "Min" ) ==std::string::npos && !pElem->GetText() ) {
                    return;
                }
                elevation.append ( pElem->GetTextStr() );
                elevation.append ( "/" );
                pElem = pElem->NextSiblingElement();
                if ( !pElem && pElem->ValueStr().find ( "Max" ) ==std::string::npos && !pElem->GetText() ) {
                    return;
                }
                elevation.append ( pElem->GetTextStr() );

            } else if ( pElem->ValueStr().find ( "Value" ) !=std::string::npos ) {
                while ( pElem && pElem->GetText() ) {
                    if ( elevation.size() >0 ) {
                        elevation.append ( "," );
                    }
                    elevation.append ( pElem->GetTextStr() );
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
        LOGGER_INFO ( _ ( "POST request with invalid content" ) );
        return;
    }
    TiXmlElement *rootEl = doc.FirstChildElement();
    if ( !rootEl ) {
        LOGGER_INFO ( _ ( "Cannot retrieve XML root element" ) );
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

Request::Request ( char* strquery, char* hostName, char* path, char* https ) : hostName ( hostName ),path ( path ),service ( "" ),request ( "" ),scheme ( "http://" ) {
    LOGGER_DEBUG ( "QUERY="<<strquery );
    if ( https )
        scheme = ( strcmp ( https,"on" ) == 0 || strcmp ( https,"ON" ) ==0?"https://":"http://" );
    url_decode ( strquery );

    for ( int pos = 0; strquery[pos]; ) {
        char* key = strquery + pos;
        for ( ; strquery[pos] && strquery[pos] != '=' && strquery[pos] != '&'; pos++ ); // on trouve le premier "=", "&" ou 0
        char* value = strquery + pos;
        for ( ; strquery[pos] && strquery[pos] != '&'; pos++ ); // on trouve le suivant "&" ou 0
        if ( *value == '=' ) *value++ = 0; // on met un 0 à la place du '=' entre key et value
        if ( strquery[pos] ) strquery[pos++] = 0; // on met un 0 à la fin du char* value

        toLowerCase ( key );

        if ( strcmp ( key,"service" ) ==0 ) {
            toLowerCase ( value );
            service = value;
        } else if ( strcmp ( key,"request" ) ==0 ) {
            toLowerCase ( value );
            request = value;
        } else {
            params.insert ( std::pair<std::string, std::string> ( key, value ) );
        }
    }
    /*LOGGER_DEBUG("GET Params :");
    std::map<std::string, std::string>::iterator it;
    for (it=params.begin();it!=params.end();it++) {
        LOGGER_DEBUG(it->first << " = " << it->second);
    }*/
}


Request::Request ( char* strquery, char* hostName, char* path, char* https, std::string postContent ) : hostName ( hostName ),path ( path ),service ( "" ),request ( "" ),scheme ( "" ) {
    LOGGER_DEBUG ( "QUERY="<<strquery );
    scheme = ( https?"https://":"http://" );
    //url_decode(strquery);

    /*for (int pos = 0; strquery[pos];) {
        char* key = strquery + pos;
        for (;strquery[pos] && strquery[pos] != '=' && strquery[pos] != '&'; pos++); // on trouve le premier "=", "&" ou 0
        char* value = strquery + pos;
        for (;strquery[pos] && strquery[pos] != '&'; pos++); // on trouve le suivant "&" ou 0
        if (*value == '=') *value++ = 0; // on met un 0 à la place du '=' entre key et value
        if (strquery[pos]) strquery[pos++] = 0; // on met un 0 à la fin du char* value

        toLowerCase(key);

        if (strcmp(key,"service")==0) {
            toLowerCase(value);
            service = value;
        } else if (strcmp(key,"request")==0) {
            toLowerCase(value);
            request = value;
        } else {
            params.insert(std::pair<std::string, std::string> (key, value));
        }
    }*/
    if ( !postContent.empty() ) {
        parsePostContent ( postContent,params );
        //DEBUG

    }
    /*LOGGER_DEBUG("POST Params :");
    std::map<std::string, std::string>::iterator it;
    for (it=params.begin();it!=params.end();it++) {
        LOGGER_DEBUG(it->first << " = " << it->second);
    }*/
    std::map<std::string, std::string>::iterator it = params.find ( "service" );
    if ( it != params.end() ) {
        service = it->second;
    }
    it = params.find ( "request" );
    if ( it != params.end() ) {
        request = it->second;
    }

}


Request::~Request() {}

// Test if a parameter is part of the request
bool Request::hasParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = params.find ( paramName );
    if ( it == params.end() ) {
        return false;
    }
    return true;
}

// Get value for a parameter in the request
std::string Request::getParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = params.find ( paramName );
    if ( it == params.end() ) {
        return "";
    }
    return it->second;
}

DataSource* Request::getTileParam ( ServicesConf& servicesConf, std::map< std::string, TileMatrixSet* >& tmsList, std::map< std::string, Layer* >& layerList, Layer*& layer, std::string& tileMatrix, int& tileCol, int& tileRow, std::string& format, Style*& style, bool& noDataError ) {
    // VERSION
    std::string version=getParam ( "version" );
    if ( version=="" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre VERSION absent." ),"wmts" ) );
    if ( version.find ( servicesConf.getServiceTypeVersion() ) ==std::string::npos )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Valeur du parametre VERSION invalide (doit contenir " ) +servicesConf.getServiceTypeVersion() +_ ( ")" ),"wmts" ) );
    // LAYER
    std::string str_layer=getParam ( "layer" );
    if ( str_layer == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre LAYER absent." ),"wmts" ) );
    std::map<std::string, Layer*>::iterator it = layerList.find ( str_layer );
    if ( it == layerList.end() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Layer " ) +str_layer+_ ( " inconnu." ),"wmts" ) );
    layer = it->second;
    // TILEMATRIXSET
    std::string str_tms=getParam ( "tilematrixset" );
    if ( str_tms == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre TILEMATRIXSET absent." ),"wmts" ) );
    std::map<std::string, TileMatrixSet*>::iterator tms = tmsList.find ( str_tms );
    if ( tms == tmsList.end() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "TileMatrixSet " ) +str_tms+_ ( " inconnu." ),"wmts" ) );
    layer = it->second;
    // TILEMATRIX
    tileMatrix=getParam ( "tilematrix" );
    if ( tileMatrix == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre TILEMATRIX absent." ),"wmts" ) );

    std::map<std::string, TileMatrix>* pList=tms->second->getTmList();
    std::map<std::string, TileMatrix>::iterator tm = pList->find ( tileMatrix );
    if ( tm==pList->end() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "TileMatrix " ) +tileMatrix+_ ( " inconnu pour le TileMatrixSet " ) +str_tms,"wmts" ) );

    // TILEROW
    std::string strTileRow=getParam ( "tilerow" );
    if ( strTileRow == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre TILEROW absent." ),"wmts" ) );
    if ( sscanf ( strTileRow.c_str(),"%d",&tileRow ) !=1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre TILEROW est incorrecte." ),"wmts" ) );
    // TILECOL
    std::string strTileCol=getParam ( "tilecol" );
    if ( strTileCol == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre TILECOL absent." ),"wmts" ) );
    if ( sscanf ( strTileCol.c_str(),"%d",&tileCol ) !=1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre TILECOL est incorrecte." ),"wmts" ) );
    // FORMAT

    format=getParam ( "format" );

    LOGGER_DEBUG ( _ ( "format requete : " ) << format << _ ( " format pyramide : " ) << Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) );
    if ( format == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre FORMAT absent." ),"wmts" ) );
    // TODO : la norme exige la presence du parametre format. Elle ne precise pas que le format peut differer de la tuile, ce que ce service ne gere pas
    if ( format.compare ( Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) ) !=0 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le format " ) +format+_ ( " n'est pas gere pour la couche " ) +str_layer,"wmts" ) );
    //Style
    std::string styleName=getParam ( "style" );
    if ( styleName == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre STYLE absent." ),"wmts" ) );
    // TODO : Nom de style : inspire_common:DEFAULT en mode Inspire sinon default
    if ( layer->getStyles().size() != 0 ) {
        for ( unsigned int i=0; i < layer->getStyles().size(); i++ ) {
            if ( styleName == layer->getStyles() [i]->getId() )
                style=layer->getStyles() [i];
        }
    }
    if ( ! ( style ) )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le style " ) +styleName+_ ( " n'est pas gere pour la couche " ) +str_layer,"wmts" ) );
    //Nodata Error
    noDataError = hasParam ( "nodataashttpstatus" );

    return NULL;

}


/* *
 * \~french
 * \brief Découpe une chaîne de caractères selon un délimiteur
 * \param[in] str la chaîne à découper
 * \param[in] delim le délimiteur
 * \param[in,out] results la liste contenant les parties de la chaîne
 * \~english
 * \brief Split a string using a specified delimitor
 * \param[in] str the string to split
 * \param[in] delim the delimitor
 * \param[in,out] results the list with the splited string
 */
/*void stringSplit ( std::string str, std::string delim, std::vector<std::string> &results ) {
    int cutAt;
    while ( ( cutAt = str.find_first_of ( delim ) ) != str.npos ) {
        if ( cutAt > 0 ) {
            results.push_back ( str.substr ( 0,cutAt ) );
        }
        str = str.substr ( cutAt+1 );
    }
    if ( str.length() > 0 ) {
        results.push_back ( str );
    }
}*/

DataStream* Request::getMapParam ( ServicesConf& servicesConf, std::map< std::string, Layer* >& layerList, std::vector<Layer*>& layers,
                                   BoundingBox< double >& bbox, int& width, int& height, CRS& crs, std::string& format,
                                   std::vector<Style*>& styles, std::map< std::string, std::string >& format_option ) {
    // VERSION
    std::string version=getParam ( "version" );
    if ( version=="" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre VERSION absent." ),"wms" ) );
    if ( version!="1.3.0" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Valeur du parametre VERSION invalide (1.3.0 disponible seulement))" ),"wms" ) );
    // LAYER
    std::string str_layer=getParam ( "layers" );
    if ( str_layer == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre LAYERS absent." ),"wms" ) );
    //Split layer Element
    std::vector<std::string> layersString = split ( str_layer,',' );
    LOGGER_DEBUG ( _ ( "Nombre de couches demandees =" ) << layersString.size() );
    if ( layersString.size() > servicesConf.getLayerLimit() ) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le nombre de couche demande excede la valeur du LayerLimit." ),"wms" ) );
    }
    std::vector<std::string>::iterator itLayer = layersString.begin();
    for ( ; itLayer != layersString.end(); itLayer++ ) {
        std::map<std::string, Layer*>::iterator it = layerList.find ( *itLayer );
        if ( it == layerList.end() )
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,_ ( "Layer " ) +*itLayer+_ ( " inconnu." ),"wms" ) );
        layers.push_back ( it->second );
    }
    LOGGER_DEBUG ( _ ( "Nombre de couches =" ) << layers.size() );
    // WIDTH
    std::string strWidth=getParam ( "width" );
    if ( strWidth == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre WIDTH absent." ),"wms" ) );
    width=atoi ( strWidth.c_str() );
    if ( width == 0 || width == INT_MAX || width == INT_MIN )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre WIDTH n'est pas une valeur entiere." ),"wms" ) );
    if ( width<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre WIDTH est negative." ),"wms" ) );
    if ( width>servicesConf.getMaxWidth() )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre WIDTH est superieure a la valeur maximum autorisee par le service." ),"wms" ) );
    // HEIGHT
    std::string strHeight=getParam ( "height" );
    if ( strHeight == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre HEIGHT absent." ),"wms" ) );
    height=atoi ( strHeight.c_str() );
    if ( height == 0 || height == INT_MAX || height == INT_MIN )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre HEIGHT n'est pas une valeur entiere." ),"wms" ) ) ;
    if ( height<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre HEIGHT est negative." ),"wms" ) );
    if ( height>servicesConf.getMaxHeight() )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre HEIGHT est superieure a la valeur maximum autorisee par le service." ),"wms" ) );
    // CRS
    std::string str_crs=getParam ( "crs" );
    if ( str_crs == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre CRS absent." ),"wms" ) );
    // Existence du CRS dans la liste de CRS des layers
    crs.setRequestCode ( str_crs );
    bool crsNotFound = true;
    unsigned int k;
    for ( k=0; k<servicesConf.getGlobalCRSList()->size(); k++ )
        if ( crs.cmpRequestCode ( servicesConf.getGlobalCRSList()->at ( k ).getRequestCode() ) ) {
            crsNotFound = false;
            break;
        }
    if ( crsNotFound ) {
        for ( unsigned int j = 0; j < layers.size() ; j++ ) {
            for ( k=0; k<layers.at ( j )->getWMSCRSList().size(); k++ )
                if ( crs.cmpRequestCode ( layers.at ( j )->getWMSCRSList().at ( k ).getRequestCode() ) ) {
                    crsNotFound = false;
                    break;
                }
            if ( crsNotFound )
                return new SERDataStream ( new ServiceException ( "",WMS_INVALID_CRS,_ ( "CRS " ) +str_crs+_ ( " (equivalent PROJ4 " ) +crs.getProj4Code() +_ ( " ) inconnu pour le layer " ) +layersString.at ( j ) +".","wms" ) );
        }

    }

    // FORMAT
    format=getParam ( "format" );
    if ( format == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre FORMAT absent." ),"wms" ) );

    for ( k=0; k<servicesConf.getFormatList()->size(); k++ ) {
        if ( servicesConf.getFormatList()->at ( k ) ==format )
            break;
    }
    if ( k==servicesConf.getFormatList()->size() )
        return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,_ ( "Format " ) +format+_ ( " non gere par le service." ),"wms" ) );

    // BBOX
    std::string strBbox=getParam ( "bbox" );
    if ( strBbox == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre BBOX absent." ),"wms" ) );
    std::vector<std::string> coords = split ( strBbox,',' );

    if ( coords.size() !=4 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Parametre BBOX incorrect." ),"wms" ) );
    double bb[4];
    for ( int i = 0; i < 4; i++ ) {
        if ( sscanf ( coords[i].c_str(),"%lf",&bb[i] ) !=1 )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Parametre BBOX incorrect." ),"wms" ) );
	//Test NaN values
	if (bb[i]!=bb[i])
	  return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Parametre BBOX incorrect." ),"wms" ) );
    }
    if ( bb[0]>=bb[2] || bb[1]>=bb[3] )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Parametre BBOX incorrect." ),"wms" ) );
    bbox.xmin=bb[0];
    bbox.ymin=bb[1];
    bbox.xmax=bb[2];
    bbox.ymax=bb[3];
    // L'ordre des coordonnees (X,Y) de chque coin de la bbox doit suivre l'ordre des axes du CS associe au SRS
    // Implementation MapServer : l'ordre des axes est inverse pour les CRS de l'EPSG compris entre 4000 et 5000
    /*if (crs.getAuthority()=="EPSG" || crs.getAuthority()=="epsg") {
        int code=atoi(crs.getIdentifier().c_str());
        if (code>=4000 && code<5000){
                bbox.xmin=bb[1];
                        bbox.ymin=bb[0];
                        bbox.xmax=bb[3];
                        bbox.ymax=bb[2];
        }
    }*/

    // Data are stored in Long/Lat, Geographical system need to be inverted in EPSG registry
    if ( ( crs.getAuthority() =="EPSG" || crs.getAuthority() =="epsg" ) && crs.isLongLat() ) {
        bbox.xmin=bb[1];
        bbox.ymin=bb[0];
        bbox.xmax=bb[3];
        bbox.ymax=bb[2];
    }

//     if ( !(hasParam("disable_bbox_crs_check")) && !(crs.validateBBox(bbox))) {
//         return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_("Parametre BBOX invalide pour le CRS ")+str_crs+_("."),"wms" ) );
//     }
//     // Hypothese : les resolutions en X ET en Y doivent etre dans la plage de valeurs
//
//     // Resolution en x et y en unites du CRS demande
//     double resx= ( bbox.xmax-bbox.xmin ) /width, resy= ( bbox.ymax-bbox.ymin ) /height;
//
    //double resx= ( bbox.xmax-bbox.xmin ) /width, resy= ( bbox.ymax-bbox.ymin ) /height;

    // Resolution en x et y en m
    // Hypothese : les CRS en geographiques sont en degres
    /*if ( crs.isLongLat() ) {
        resx*=111319;
        resy*=111319;
    }*/

    // Le serveur ne doit pas renvoyer d'exception
    // Cf. WMS 1.3.0 - 7.2.4.6.9

    /*double epsilon=0.0000001;   // Gestion de la precision de la division
    if ( resx>0. )
        if ( resx+epsilon<layer->getMinRes() ||resy+epsilon<layer->getMinRes() ) {
            ;//return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La resolution de l'image est inferieure a la resolution minimum.","wms"));
        }
    if ( resy>0. )
        if ( resx>layer->getMaxRes() +epsilon||resy>layer->getMaxRes() +epsilon )
            ;//return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La resolution de l'image est superieure a la resolution maximum.","wms"));
    */
    // EXCEPTION
    std::string str_exception=getParam ( "exception" );
    if ( str_exception!=""&&str_exception!="XML" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Format d'exception " ) +str_exception+_ ( " non pris en charge" ),"wms" ) );

    //STYLES
    if ( ! ( hasParam ( "styles" ) ) )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre STYLES absent." ),"wms" ) );
    std::string str_styles=getParam ( "styles" );
    if ( str_styles == "" ) {
        str_styles.append ( layers.at ( 0 )->getDefaultStyle() );
        for ( int i = 1;  i < layers.size(); i++ ) {
            str_styles.append ( "," );
            str_styles.append ( layers.at ( i )->getDefaultStyle() );
        }
    }
    std::vector<std::string> stylesString = split ( str_styles,',' );
    LOGGER_DEBUG ( _ ( "Nombre de styles demandes =" ) << layers.size() );
    if ( stylesString.size() != layersString.size() ) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre STYLES incomplet." ),"wms" ) );
    }
    for ( int k = 0 ; k  < stylesString.size(); k++ ) {

        if ( layers.at ( k )->getStyles().size() != 0 ) {
            for ( unsigned int i=0; i < layers.at ( k )->getStyles().size(); i++ ) {
                if ( stylesString.at ( k ) == "" ) {
                    stylesString.at ( k ).append ( layers.at ( k )->getDefaultStyle() );
                }
                if ( stylesString.at ( k ) == layers.at ( k )->getStyles() [i]->getId() ) {
                    styles.push_back ( layers.at ( k )->getStyles() [i] );
                }
            }
        }

        if ( styles.size() < k+1 )
            return new SERDataStream ( new ServiceException ( "",WMS_STYLE_NOT_DEFINED,_ ( "Le style " ) +stylesString.at ( k ) +_ ( " n'est pas gere pour la couche " ) +layersString.at ( k ),"wms" ) );
    }

    std::string formatOptionString= getParam ( "format_options" ).c_str();
    char* formatOptionChar = new char[formatOptionString.size() +1];
    memset ( formatOptionChar,0,formatOptionString.size() +1 );
    memcpy ( formatOptionChar,formatOptionString.c_str(),formatOptionString.size() );

    for ( int pos = 0; formatOptionChar[pos]; ) {
        char* key = formatOptionChar + pos;
        for ( ; formatOptionChar[pos] && formatOptionChar[pos] != ':' && formatOptionChar[pos] != ';'; pos++ ); // on trouve le premier "=", "&" ou 0
        char* value = formatOptionChar + pos;
        for ( ; formatOptionChar[pos] && formatOptionChar[pos] != ';'; pos++ ); // on trouve le suivant "&" ou 0
        if ( *value == ':' ) *value++ = 0; // on met un 0 à la place du '=' entre key et value
        if ( formatOptionChar[pos] ) formatOptionChar[pos++] = 0; // on met un 0 à la fin du char* value

        toLowerCase ( key );
        toLowerCase ( value );
        format_option.insert ( std::pair<std::string, std::string> ( key, value ) );
    }
    delete[] formatOptionChar;
    formatOptionChar=NULL;
    return NULL;
}

// Parameters for WMS GetCapabilities
DataStream* Request::getCapWMSParam ( ServicesConf& servicesConf, std::string& version ) {
    if ( service.compare ( "wms" ) !=0 ) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le service " ) +service+_ ( " est inconnu pour ce serveur." ),"wms" ) );
    }

    version=getParam ( "version" );
    if ( version=="" ) {
        version = "1.3.0";
        return NULL;
    }
    // Version number negotiation for WMS (should not be done for WMTS)
    // Ref: http://cite.opengeospatial.org/OGCTestData/wms/1.1.1/spec/wms1.1.1.html#basic_elements.version.negotiation
    // - Version higher than supported version: send the highest supported version
    // - Version lower than supported version: send the lowest supported version
    // We compare the different values of the version number (l=left, m=middle, r=right)
    // Versions supported:
    std::string high_version = "1.3.0";
    int high_version_l = high_version[0]-48; //-48 is because of ASCII table, numbers start at position 48
    int high_version_m = high_version[2]-48;
    int high_version_r = high_version[4]-48;
    std::string low_version = "1.3.0"; // Currently high_version == low_version, ready to be changed later
    int low_version_l = low_version[0]-48;
    int low_version_m = low_version[2]-48;
    int low_version_r = low_version[4]-48;
    // Getting the request values to compare
    int request_l = version[0]-48;
    int request_m = version[2]-48;
    int request_r = version[4]-48;
    // We check the numbers from left to right
    if ( request_l > high_version_l || ( request_l == high_version_l && request_m > high_version_m ) || ( request_l == high_version_l && request_m == high_version_m && request_r > high_version_r ) ) {
        // Version asked is higher than supported version
        version = high_version;
        return NULL;
    }
    if ( request_l < low_version_l || ( request_l == low_version_l && request_m < low_version_m ) || ( request_l == low_version_l && request_m == low_version_m && request_r < low_version_r ) ) {
        // Version asked is lower than supported version
        version = low_version;
        return NULL;
    }

    if ( version!="1.3.0" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Valeur du parametre VERSION invalide (1.3.0 disponible seulement))" ),"wms" ) );
    return NULL;
}

// Parameters for WMTS GetCapabilities
DataStream* Request::getCapWMTSParam ( ServicesConf& servicesConf, std::string& version ) {
    if ( service.compare ( "wmts" ) !=0 ) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le service " ) +service+_ ( " est inconnu pour ce serveur." ),"wmts" ) );
    }
    version=getParam ( "version" );
    if ( version=="" ) {
        version=servicesConf.getServiceTypeVersion();
        return NULL;
    }
    if ( version.compare ( servicesConf.getServiceTypeVersion() ) !=0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Valeur du parametre VERSION invalide (1.0.0 disponible seulement)" ),"wmts" ) );
    return NULL;
}

