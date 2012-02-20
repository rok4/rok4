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

/* converts hex char (0-9, A-Z, a-z) to decimal.
 * returns 0xFF on invalid input.
 */
char hex2int ( unsigned char hex ) {
    hex = hex - '0';
    if ( hex > 9 ) {
        hex = ( hex + '0' - 1 ) | 0x20;
        hex = hex - 'a' + 11;
    }
    if ( hex > 15 )
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

void toLowerCase ( char* str ) {
    if ( str ) for ( int i = 0; str[i]; i++ ) str[i] = tolower ( str[i] );
}

void removeNameSpace ( std::string& elementName ) {
    size_t pos = elementName.find ( ":" );
    if ( elementName.size() <= pos ) {
        return;
    }
    elementName.erase ( elementName.begin(),elementName.begin() +pos );
}
void parseGetCapabilitiesPost ( TiXmlHandle& hGetCap, std::map< std::string, std::string >& parameters ) {
    LOGGER_DEBUG ( "Parse GetCapabilities Request" );
    std::string version;
    std::string service;

    parameters.insert ( std::pair<std::string, std::string> ( "request", "getcapabilities" ) );

    TiXmlElement* pElem = hGetCap.ToElement();

    if ( pElem->QueryStringAttribute ( "service",&service ) != TIXML_SUCCESS ) {
        LOGGER_DEBUG ( "No service attribute" );
        return;
    }

    std::transform ( service.begin(), service.end(), service.begin(), tolower );

    parameters.insert ( std::pair<std::string, std::string> ( "service", service ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        LOGGER_DEBUG ( "No version attribute" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );

}

void parseGetTilePost ( TiXmlHandle& hGetTile, std::map< std::string, std::string >& parameters ) {
    LOGGER_DEBUG ( "Parse GetTile Request" );
    TiXmlElement* pElem = hGetTile.ToElement();
    std::string version;
    std::string service;

    parameters.insert ( std::pair<std::string, std::string> ( "request", "gettile" ) );

    if ( pElem->QueryStringAttribute ( "service",&service ) != TIXML_SUCCESS ) {
        LOGGER_DEBUG ( "No service attribute" );
        return;
    }
    std::transform ( service.begin(), service.end(), service.begin(), tolower );
    parameters.insert ( std::pair<std::string, std::string> ( "service", service ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        LOGGER_DEBUG ( "No version attribute" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );
    //Layer
    pElem = hGetTile.FirstChildElement().ToElement();

    if ( !pElem && pElem->ValueStr().find ( "Layer" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( "No Layer" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "layer", pElem->GetText() ) );

    //Style
    pElem = pElem->NextSiblingElement();

    if ( !pElem && pElem->ValueStr().find ( "Style" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( "No Style" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "style", pElem->GetText() ) );

    //Format
    pElem = pElem->NextSiblingElement();

    if ( !pElem && pElem->ValueStr().find ( "Format" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( "No Format" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "format", pElem->GetText() ) );

    //DimensionNameValue name="NAME" OPTIONAL
    pElem = pElem->NextSiblingElement();
    while ( pElem && pElem->ValueStr().find ( "DimensionNameValue" ) !=std::string::npos && pElem->GetText() ) {
        std::string dimensionName;
        if ( pElem->QueryStringAttribute ( "name",&dimensionName ) != TIXML_SUCCESS ) {
            LOGGER_DEBUG ( "No Name attribute" );
            continue;
        } else {
            parameters.insert ( std::pair<std::string, std::string> ( dimensionName, pElem->GetText() ) );
        }
        pElem = pElem->NextSiblingElement();
    }

    //TileMatrixSet
    if ( !pElem && pElem->ValueStr().find ( "TileMatrixSet" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( "No TileMatrixSet" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilematrixset", pElem->GetText() ) );

    //TileMatrix
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileMatrix" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( "No TileMatrix" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilematrix", pElem->GetText() ) );

    //TileRow
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileRow" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( "No TileRow" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilerow", pElem->GetText() ) );
    //TileCol
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileCol" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( "No TileCol" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilecol", pElem->GetText() ) );

}

void parseGetMapPost ( TiXmlHandle& hGetMap, std::map< std::string, std::string >& parameters ) {
    LOGGER_DEBUG ( "Parse GetMap Request" );
    TiXmlElement* pElem = hGetMap.ToElement();
    std::string version;
    std::string sldVersion;
    std::string layers;
    std::string styles;
    std::string bbox;

    parameters.insert ( std::pair<std::string, std::string> ( "service", "wms" ) );
    parameters.insert ( std::pair<std::string, std::string> ( "request", "getmap" ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        LOGGER_DEBUG ( "No version attribute" );
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );

    //StyledLayerDescriptor Layers and Style

    pElem=hGetMap.FirstChildElement().ToElement();
    if ( !pElem && pElem->ValueStr().find ( "StyledLayerDescriptor" ) ==std::string::npos ) {
        LOGGER_DEBUG ( "No StyledLayerDescriptor" );
        return;
    }
    pElem=hGetMap.FirstChildElement().FirstChildElement().ToElement();
    if ( !pElem ) {
        LOGGER_DEBUG ( "Empty StyledLayerDescriptor" );
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
            LOGGER_DEBUG ( "NamedLayer without Name" );
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
                LOGGER_DEBUG ( "NamedLayer without NamedStyle, use default style" );
                break;
            }
        }
        pElemNamedLayer = pElemNamedLayer->FirstChildElement();
        if ( !pElemNamedLayer && pElemNamedLayer->ValueStr().find ( "Name" ) ==std::string::npos && !pElemNamedLayer->GetText() ) {
            LOGGER_DEBUG ( "NamedStyle without Name" );
            return;
        } else {
            styles.append ( pElemNamedLayer->GetText() );
        }
        pElem= pElem->NextSiblingElement ( pElem->ValueStr() );

    }
    if ( layers.size() <=0 ) {
        LOGGER_DEBUG ( "StyledLayerDescriptor without NamedLayer" );
        return;
    }

    parameters.insert ( std::pair<std::string, std::string> ( "layers", layers ) );
    parameters.insert ( std::pair<std::string, std::string> ( "styles", styles ) );

    //CRS
    pElem=hGetMap.FirstChildElement().ToElement()->NextSiblingElement();
    ;
    if ( !pElem && pElem->ValueStr().find ( "CRS" ) ==std::string::npos && !pElem->GetText() ) {
        LOGGER_DEBUG ( "No CRS" );
        return;
    }

    parameters.insert ( std::pair<std::string, std::string> ( "crs", pElem->GetText() ) );

    //BoundingBox

    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "BoundingBox" ) ==std::string::npos ) {
        LOGGER_DEBUG ( "No BoundingBox" );
        return;
    }

    //FIXME crs attribute might be different from CRS in the request
    //eg output image in EPSG:3857 BBox defined in EPSG:4326
    TiXmlElement * pElemBBox = pElem->FirstChildElement();
    if ( !pElemBBox && pElemBBox->ValueStr().find ( "LowerCorner" ) ==std::string::npos && !pElemBBox->GetText() ) {
        LOGGER_DEBUG ( "BoundingBox Invalid" );
        return;
    }

    bbox.append ( pElemBBox->GetText() );
    bbox.replace ( bbox.find ( " " ),1,"," );
    bbox.append ( "," );

    pElemBBox = pElemBBox->NextSiblingElement();
    if ( !pElemBBox && pElemBBox->ValueStr().find ( "UpperCorner" ) ==std::string::npos && !pElemBBox->GetText() ) {
        LOGGER_DEBUG ( "BoundingBox Invalid" );
        return;
    }
    bbox.append ( pElemBBox->GetText() );
    bbox.replace ( bbox.find ( " " ),1,"," );

    parameters.insert ( std::pair<std::string, std::string> ( "bbox", bbox ) );

    //Output
    {
        pElem = pElem->NextSiblingElement();
        if ( !pElem && pElem->ValueStr().find ( "Output" ) ==std::string::npos ) {
            LOGGER_DEBUG ( "No Output" );
            return;
        }
        TiXmlElement * pElemOut =  pElem->FirstChildElement();
        if ( !pElemOut && pElemOut->ValueStr().find ( "Size" ) ==std::string::npos ) {
            LOGGER_DEBUG ( "Output Invalid no Size" );
            return;
        }
        TiXmlElement * pElemOutTmp =  pElemOut->FirstChildElement();
        if ( !pElemOutTmp && pElemOutTmp->ValueStr().find ( "Width" ) ==std::string::npos && !pElemOutTmp->GetText() ) {
            LOGGER_DEBUG ( "Output Invalid, Width incorrect" );
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
            LOGGER_DEBUG ( "Output Invalid no Format" );
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
            }
        }

    }


    //OPTIONAL

    //Exceptions
    pElem = pElem->NextSiblingElement();
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Exceptions" ) !=std::string::npos && pElem->GetText() ) {
            parameters.insert ( std::pair<std::string, std::string> ( "exceptions", pElem->GetText() ) );
            pElem = pElem->NextSiblingElement();
        }
    }

    //Time
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Time" ) !=std::string::npos && pElem->GetText() ) {
            parameters.insert ( std::pair<std::string, std::string> ( "time", pElem->GetText() ) );
            pElem = pElem->NextSiblingElement();
        }
    }

    //Elevation
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Elevation" ) !=std::string::npos ) {
            pElem = pElem->FirstChildElement();
            if ( !pElem ) {
                LOGGER_DEBUG ( "Elevation incorrect" );
                return;
            }
            std::string elevation;
            if ( pElem->ValueStr().find ( "Interval" ) !=std::string::npos ) {
                pElem = pElem->FirstChildElement();
                if ( !pElem && pElem->ValueStr().find ( "Min" ) ==std::string::npos && !pElem->GetText() ) {
                    return;
                }
                elevation.append ( pElem->GetText() );
                elevation.append ( "/" );
                pElem = pElem->NextSiblingElement();
                if ( !pElem && pElem->ValueStr().find ( "Max" ) ==std::string::npos && !pElem->GetText() ) {
                    return;
                }
                elevation.append ( pElem->GetText() );

            } else if ( pElem->ValueStr().find ( "Value" ) !=std::string::npos ) {
                while ( pElem && pElem->GetText() ) {
                    if ( elevation.size() >0 ) {
                        elevation.append ( "," );
                    }
                    elevation.append ( pElem->GetText() );
                    pElem = pElem->NextSiblingElement ( pElem->ValueStr() );
                }
            }
            parameters.insert ( std::pair<std::string, std::string> ( "elevation", elevation ) );
        }
    }

}

void parsePostContent ( std::string content, std::map< std::string, std::string >& parameters ) {
    TiXmlDocument doc ( "request" );
    content.append ( "\n" );
    if ( !doc.Parse ( content.c_str() ) ) {
        LOGGER_INFO ( "POST request with invalid content" );
        return;
    }
    TiXmlElement *rootEl = doc.FirstChildElement();
    if ( !rootEl ) {
        LOGGER_INFO ( "Cannot retrieve XML root element" );
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

/**
 * Get Request Constructor
 * @param strquery http query arguments
 * @param hostName hostname declared in the request
 * @param path webserver path to the ROK4 Server
 * @param https https request if defined
 */
Request::Request ( char* strquery, char* hostName, char* path, char* https ) : hostName ( hostName ),path ( path ),service ( "" ),request ( "" ),scheme ( "" ) {
    LOGGER_DEBUG ( "QUERY="<<strquery );
    scheme = ( https?"https://":"http://" );
    url_decode ( strquery );

    for ( int pos = 0; strquery[pos]; ) {
        char* key = strquery + pos;
        for ( ;strquery[pos] && strquery[pos] != '=' && strquery[pos] != '&'; pos++ ); // on trouve le premier "=", "&" ou 0
        char* value = strquery + pos;
        for ( ;strquery[pos] && strquery[pos] != '&'; pos++ ); // on trouve le suivant "&" ou 0
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

/**
 * Post Request Constructor
 * @param strquery http query arguments
 * @param hostName hostname declared in the request
 * @param path webserver path to the ROK4 Server
 * @param https https request if defined
 * @param postContent the http Post Content
 */
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

/**
 * @vriedf test de la présence de paramName dans la requete
 * @return true si présent
 */
bool Request::hasParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = params.find ( paramName );
    if ( it == params.end() ) {
        return false;
    }
    return true;
}


/**
 * @vriedf récupération du parametre paramName dans la requete
 * @return la valeur du parametre si existant "" sinon
 */
std::string Request::getParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = params.find ( paramName );
    if ( it == params.end() ) {
        return "";
    }
    return it->second;
}

/**
* @vrief Verification et recuperation des parametres d'une requete GetTile
* @return message d'erreur en cas d'erreur (NULL sinon)
*/

DataSource* Request::getTileParam ( ServicesConf& servicesConf, std::map< std::string, TileMatrixSet* >& tmsList, std::map< std::string, Layer* >& layerList, Layer*& layer, std::string& tileMatrix, int& tileCol, int& tileRow, std::string& format, Style*& style ) {
    // VERSION
    std::string version=getParam ( "version" );
    if ( version=="" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre VERSION absent.","wmts" ) );
    if ( version.find ( servicesConf.getServiceTypeVersion() ) ==std::string::npos )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (doit contenir "+servicesConf.getServiceTypeVersion() +")","wmts" ) );
    // LAYER
    std::string str_layer=getParam ( "layer" );
    if ( str_layer == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre LAYER absent.","wmts" ) );
    std::map<std::string, Layer*>::iterator it = layerList.find ( str_layer );
    if ( it == layerList.end() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Layer "+str_layer+" inconnu.","wmts" ) );
    layer = it->second;
    // TILEMATRIXSET
    std::string str_tms=getParam ( "tilematrixset" );
    if ( str_tms == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEMATRIXSET absent.","wmts" ) );
    std::map<std::string, TileMatrixSet*>::iterator tms = tmsList.find ( str_tms );
    if ( tms == tmsList.end() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"TileMatrixSet "+str_tms+" inconnu.","wmts" ) );
    layer = it->second;
    // TILEMATRIX
    tileMatrix=getParam ( "tilematrix" );
    if ( tileMatrix == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEMATRIX absent.","wmts" ) );

    std::map<std::string, TileMatrix>* pList=tms->second->getTmList();
    std::map<std::string, TileMatrix>::iterator tm = pList->find ( tileMatrix );
    if ( tm==pList->end() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"TileMatrix "+tileMatrix+" inconnu pour le TileMatrixSet "+str_tms,"wmts" ) );

    // TILEROW
    std::string strTileRow=getParam ( "tilerow" );
    if ( strTileRow == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEROW absent.","wmts" ) );
    if ( sscanf ( strTileRow.c_str(),"%d",&tileRow ) !=1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILEROW est incorrecte.","wmts" ) );
    // TILECOL
    std::string strTileCol=getParam ( "tilecol" );
    if ( strTileCol == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre TILECOL absent.","wmts" ) );
    if ( sscanf ( strTileCol.c_str(),"%d",&tileCol ) !=1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILECOL est incorrecte.","wmts" ) );
    // FORMAT

    format=getParam ( "format" );

    LOGGER_DEBUG ( "format requete : " << format << " format pyramide : " << format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) );
    if ( format == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre FORMAT absent.","wmts" ) );
    // TODO : la norme exige la presence du parametre format. Elle ne precise pas que le format peut differer de la tuile, ce que ce service ne gere pas
    if ( format.compare ( format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) ) !=0 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Le format "+format+" n'est pas gere pour la couche "+str_layer,"wmts" ) );
    //Style
    std::string styleName=getParam ( "style" );
    if ( styleName == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre STYLE absent.","wmts" ) );
    // TODO : Nom de style : inspire_common:DEFAULT en mode Inspire sinon default
    if ( layer->getStyles().size() != 0 ) {
        for ( unsigned int i=0; i < layer->getStyles().size(); i++ ) {
            if ( styleName == layer->getStyles() [i]->getId() )
                style=layer->getStyles() [i];
        }
    }
    if ( ! ( style ) )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Le style "+styleName+" n'est pas gere pour la couche "+str_layer,"wmts" ) );

    return NULL;

}

void stringSplit ( std::string str, std::string delim, std::vector<std::string> &results ) {
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
}

/**
 * @brief Recuperation et verification des parametres d'une requete GetMap
 * @return message d'erreur en cas d'erreur (NULL sinon)
 */

DataStream* Request::getMapParam ( ServicesConf& servicesConf, std::map< std::string, Layer* >& layerList, Layer*& layer, BoundingBox< double >& bbox, int& width, int& height, CRS& crs, std::string& format, Style*& style ) {
    // VERSION
    std::string version=getParam ( "version" );
    if ( version=="" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre VERSION absent.","wms" ) );
    if ( version!="1.3.0" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (1.3.0 disponible seulement))","wms" ) );
    // LAYER
    std::string str_layer=getParam ( "layers" );
    if ( str_layer == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre LAYERS absent.","wms" ) );
    std::map<std::string, Layer*>::iterator it = layerList.find ( str_layer );
    if ( it == layerList.end() )
        return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,"Layer "+str_layer+" inconnu.","wms" ) );
    layer = it->second;
    // WIDTH
    std::string strWidth=getParam ( "width" );
    if ( strWidth == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre WIDTH absent.","wms" ) );
    width=atoi ( strWidth.c_str() );
    if ( width == 0 || width == INT_MAX || width == INT_MIN )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre WIDTH n'est pas une valeur entiere.","wms" ) );
    if ( width<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre WIDTH est negative.","wms" ) );
    if ( width>servicesConf.getMaxWidth() )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre WIDTH est superieure a la valeur maximum autorisee par le service.","wms" ) );
    // HEIGHT
    std::string strHeight=getParam ( "height" );
    if ( strHeight == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre HEIGHT absent.","wms" ) );
    height=atoi ( strHeight.c_str() );
    if ( height == 0 || height == INT_MAX || height == INT_MIN )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT n'est pas une valeur entiere.","wms" ) ) ;
    if ( height<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT est negative.","wms" ) );
    if ( height>servicesConf.getMaxHeight() )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT est superieure a la valeur maximum autorisee par le service.","wms" ) );
    // CRS
    std::string str_crs=getParam ( "crs" );
    if ( str_crs == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre CRS absent.","wms" ) );
    // Existence du CRS dans la liste de CRS du layer
    crs.setRequestCode ( str_crs );
    unsigned int k;
    for ( k=0;k<layer->getWMSCRSList().size();k++ )
        if ( crs.cmpRequestCode ( layer->getWMSCRSList().at ( k )->getRequestCode() ) )
            break;
    // FIXME : la methode vector::find plante (je ne comprends pas pourquoi)
    if ( k==layer->getWMSCRSList().size() )
        return new SERDataStream ( new ServiceException ( "",WMS_INVALID_CRS,"CRS "+str_crs+" (equivalent PROJ4 "+crs.getProj4Code() +" ) inconnu pour le layer "+str_layer+".","wms" ) );

    // FORMAT
    format=getParam ( "format" );
    if ( format == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre FORMAT absent.","wms" ) );

    for ( k=0;k<servicesConf.getFormatList()->size();k++ ) {
        if ( servicesConf.getFormatList()->at ( k ) ==format )
            break;
    }
    if ( k==servicesConf.getFormatList()->size() )
        return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,"Format "+format+" non gere par le service.","wms" ) );

    // BBOX
    std::string strBbox=getParam ( "bbox" );
    if ( strBbox == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre BBOX absent.","wms" ) );
    std::vector<std::string> coords;
    stringSplit ( strBbox,",",coords );
    if ( coords.size() !=4 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms" ) );
    double bb[4];
    for ( int i = 0; i < 4; i++ ) {
        if ( sscanf ( coords[i].c_str(),"%lf",&bb[i] ) !=1 )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms" ) );
    }
    if ( bb[0]>=bb[2] || bb[1]>=bb[3] )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms" ) );
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

    // Data are stored in Long/Lat, Geographical system need to be inverted except CRS:84
    if ( crs.isLongLat() && ! ( crs.getAuthority() =="CRS" ) ) {
        bbox.xmin=bb[1];
        bbox.ymin=bb[0];
        bbox.xmax=bb[3];
        bbox.ymax=bb[2];
    }


    // SCALE DENOMINATORS

    // Hypothese : les resolutions en X ET en Y doivent etre dans la plage de valeurs

    // Resolution en x et y en unites du CRS demande
    double resx= ( bbox.xmax-bbox.xmin ) /width, resy= ( bbox.ymax-bbox.ymin ) /height;

    // Resolution en x et y en m
    // Hypothese : les CRS en geographiques sont en degres
    if ( crs.isLongLat() ) {
        resx*=111319;
        resy*=111319;
    }

    // Le serveur ne doit pas renvoyer d'exception
    // Cf. WMS 1.3.0 - 7.2.4.6.9

    double epsilon=0.0000001;   // Gestion de la precision de la division
    if ( resx>0. )
        if ( resx+epsilon<layer->getMinRes() ||resy+epsilon<layer->getMinRes() ) {
            ;//return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La resolution de l'image est inferieure a la resolution minimum.","wms"));
        }
    if ( resy>0. )
        if ( resx>layer->getMaxRes() +epsilon||resy>layer->getMaxRes() +epsilon )
            ;//return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La resolution de l'image est superieure a la resolution maximum.","wms"));

    // EXCEPTION
    std::string str_exception=getParam ( "exception" );
    if ( str_exception!=""&&str_exception!="XML" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Format d'exception "+str_exception+" non pris en charge","wms" ) );

    if ( ! ( hasParam ( "styles" ) ) )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre STYLES absent.","wms" ) );
    std::string styles=getParam ( "styles" );
    if ( styles == "" ) //TODO Gestion du style par défaut
        styles= ( servicesConf.isInspire() ?DEFAULT_STYLE_INSPIRE:DEFAULT_STYLE );
    if ( layer->getStyles().size() != 0 ) {
        for ( unsigned int i=0; i < layer->getStyles().size(); i++ ) {
            if ( styles == layer->getStyles() [i]->getId() ) {
                style = layer->getStyles() [i];
            }
        }
    }
    if ( ! ( style ) )
        return new SERDataStream ( new ServiceException ( "",WMS_STYLE_NOT_DEFINED,"Le style "+styles+" n'est pas gere pour la couche "+str_layer,"wms" ) );
    return NULL;
}
