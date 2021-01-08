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
#include <cmath>
#include "TileMatrixSet.h"
#include "Pyramid.h"
#include "intl.h"


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

DataStream* Rok4Server::getMapParamWMS (
    Request* request, std::vector<Layer*>& layers, BoundingBox< double >& bbox, int& width, int& height, CRS& crs,
    std::string& format, std::vector<Style*>& styles, std::map< std::string, std::string >& format_option,int& dpi
) {
    // VERSION
    std::string version = request->getParam ( "version" );
    if ( version == "" ) {
        //---- WMS 1.1.1
        //le parametre version est prioritaire sur wmtver
        version = request->getParam("wmtver");
        if ( version == "") {
            return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre VERSION absent." ),"wms" ) );
        }
        //----
    }

    if ( version != "1.3.0" && version != "1.1.1")
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Valeur du parametre VERSION invalide (1.1.1 et 1.3.0 disponibles seulement))" ),"wms" ) );

    // LAYER
    std::string str_layers = request->getParam ( "layers" );
    if ( str_layers == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre LAYERS absent." ),"wms" ) );
    
    //Split layer Element
    std::vector<std::string> vector_layers = split ( str_layers,',' );
    LOGGER_DEBUG ( _ ( "Nombre de couches demandees =" ) << vector_layers.size() );

    if ( vector_layers.size() > servicesConf->getLayerLimit() ) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le nombre de couche demande excede la valeur du LayerLimit." ),"wms" ) );
    }

    for (unsigned int i = 0 ; i < vector_layers.size(); i++ ) {
        if ( Request::containForbiddenChars(vector_layers.at(i)) ) {
            // On a détecté un caractère interdit, on ne met pas le layer fourni dans la réponse pour éviter une injection
            LOGGER_WARN("Forbidden char detected in WMS layers: " << vector_layers.at(i));
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,_ ( "Layer inconnu." ),"wms" ) );
        }

        Layer* lay = serverConf->getLayer(vector_layers.at(i));
        if ( lay == NULL || ! lay->getWMSAuthorized())
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,_ ( "Layer " ) +vector_layers.at(i)+_ ( " inconnu." ),"wms" ) );
       
        layers.push_back ( lay );
    }
    LOGGER_DEBUG ( _ ( "Nombre de couches =" ) << layers.size() );

    // WIDTH
    std::string strWidth = request->getParam ( "width" );
    if ( strWidth == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre WIDTH absent." ),"wms" ) );
    width=atoi ( strWidth.c_str() );
    if ( width == 0 || width == INT_MAX || width == INT_MIN )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre WIDTH n'est pas une valeur entiere." ),"wms" ) );
    if ( width < 0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre WIDTH est negative." ),"wms" ) );
    if ( width > servicesConf->getMaxWidth() )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre WIDTH est superieure a la valeur maximum autorisee par le service." ),"wms" ) );

    // HEIGHT
    std::string strHeight = request->getParam ( "height" );
    if ( strHeight == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre HEIGHT absent." ),"wms" ) );
    height=atoi ( strHeight.c_str() );
    if ( height == 0 || height == INT_MAX || height == INT_MIN )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre HEIGHT n'est pas une valeur entiere." ),"wms" ) ) ;
    if ( height<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre HEIGHT est negative." ),"wms" ) );
    if ( height>servicesConf->getMaxHeight() )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre HEIGHT est superieure a la valeur maximum autorisee par le service." ),"wms" ) );
    
    // CRS
    std::string str_crs;
    if (version == "1.3.0") {
        str_crs = request->getParam ( "crs" );
        if ( str_crs == "" )
            return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre CRS absent." ),"wms" ) );
    } else {
        // WMS 1.1.1
        str_crs = request->getParam ( "srs" );
        if ( str_crs == "" )
            return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre SRS absent." ),"wms" ) );
    }

    if ( Request::containForbiddenChars(str_crs) ) {
        // On a détecté un caractère interdit, on ne met pas le crs fourni dans la réponse pour éviter une injection
        LOGGER_WARN("Forbidden char detected in WMS crs: " << str_crs);
        return new SERDataStream ( new ServiceException ( "",WMS_INVALID_CRS,_ ( "CRS  inconnu" ),"wms" ) );
    }
   
    // Existence du CRS dans la liste globale de CRS ou de chaque layers
    crs.setRequestCode ( str_crs );
    if ( servicesConf->isInGlobalCRSList(&crs) ) {
        for ( unsigned int j = 0; j < layers.size() ; j++ ) {
            if ( layers.at ( j )->isInWMSCRSList(&crs) )
                return new SERDataStream ( new ServiceException ( "",WMS_INVALID_CRS,_ ( "CRS " ) +str_crs+_ ( " (equivalent PROJ4 " ) +crs.getProj4Code() +_ ( " ) inconnu pour le layer " ) +vector_layers.at ( j ) +".","wms" ) );
        }
    }

    // FORMAT
    format = request->getParam ( "format" );
    if ( format == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre FORMAT absent." ),"wms" ) );

    if ( Request::containForbiddenChars(format) ) {
        // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
        LOGGER_WARN("Forbidden char detected in WMS format: " << format);
        return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,_ ( "Format non gere par le service." ),"wms" ) );
    }

    if ( ! servicesConf->isInFormatList(format) )
        return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,_ ( "Format " ) +format+_ ( " non gere par le service." ),"wms" ) );

    // BBOX
    std::string strBbox = request->getParam ( "bbox" );
    if ( strBbox == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre BBOX absent." ),"wms" ) );
    std::vector<std::string> coords = split ( strBbox,',' );

    if ( coords.size() != 4 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Parametre BBOX incorrect." ),"wms" ) );
    double bb[4];
    for ( int i = 0; i < 4; i++ ) {
        if ( sscanf ( coords[i].c_str(),"%lf",&bb[i] ) !=1 )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Parametre BBOX incorrect." ),"wms" ) );
        //Test NaN values
        if (bb[i]!=bb[i])
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Parametre BBOX incorrect." ),"wms" ) );
    }
    if ( bb[0] >= bb[2] || bb[1] >= bb[3] )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Parametre BBOX incorrect." ),"wms" ) );

    bbox.xmin=bb[0];
    bbox.ymin=bb[1];
    bbox.xmax=bb[2];
    bbox.ymax=bb[3];

    // Data are stored in Long/Lat, Geographical system need to be inverted in EPSG registry
    if ( ( crs.getAuthority() =="EPSG" || crs.getAuthority() =="epsg" ) && crs.isLongLat() && version == "1.3.0" ) {
        bbox.xmin=bb[1];
        bbox.ymin=bb[0];
        bbox.xmax=bb[3];
        bbox.ymax=bb[2];
    }

    // EXCEPTION
    std::string str_exception = request->getParam ( "exception" );
    if ( str_exception != "" && str_exception != "XML" ) {

        if ( Request::containForbiddenChars(str_exception) ) {
            // On a détecté un caractère interdit, on ne met pas le str_exception fourni dans la réponse pour éviter une injection
            LOGGER_WARN("Forbidden char detected in WMS exception: " << str_exception);
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Format d'exception non pris en charge" ),"wms" ) );
        }

        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Format d'exception " ) +str_exception+_ ( " non pris en charge" ),"wms" ) );
    }

    //STYLES
    if ( ! request->hasParam ( "styles" ) )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre STYLES absent." ),"wms" ) );

    std::string str_styles = request->getParam ( "styles" );
    if ( str_styles == "" ) {
        str_styles.append ( layers.at ( 0 )->getDefaultStyle() );
        for ( int i = 1;  i < layers.size(); i++ ) {
            str_styles.append ( "," );
            str_styles.append ( layers.at ( i )->getDefaultStyle() );
        }
    }

    std::vector<std::string> vector_styles = split ( str_styles,',' );
    LOGGER_DEBUG ( _ ( "Nombre de styles demandes =" ) << vector_styles.size() );
    if ( vector_styles.size() != vector_layers.size() ) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre STYLES incomplet." ),"wms" ) );
    }
    for ( int k = 0 ; k  < vector_styles.size(); k++ ) {
        if ( vector_styles.at ( k ) == "" ) {
            vector_styles.at (k) = layers.at ( k )->getDefaultStyle();
        }


        if ( Request::containForbiddenChars(vector_styles.at ( k )) ) {
            // On a détecté un caractère interdit, on ne met pas le style fourni dans la réponse pour éviter une injection
            LOGGER_WARN("Forbidden char detected in WMS styles: " << vector_styles.at ( k ));
            return new SERDataStream ( new ServiceException ( "",WMS_STYLE_NOT_DEFINED,_ ( "Le style n'est pas gere pour la couche " ) +vector_layers.at ( k ),"wms" ) );
        }

        Style* s = layers.at ( k )->getStyleByIdentifier(vector_styles.at ( k ));
        if ( s == NULL )
            return new SERDataStream ( new ServiceException ( "",WMS_STYLE_NOT_DEFINED,_ ( "Le style " ) +vector_styles.at ( k ) +_ ( " n'est pas gere pour la couche " ) +vector_layers.at ( k ),"wms" ) );

        styles.push_back(s);
    }

    //DPI
    std::string strDPI = request->getParam("dpi");
    if (strDPI != "") {
        dpi = atoi(strDPI.c_str());
        if ( dpi == 0 || dpi == INT_MAX || dpi == INT_MIN ) {
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre DPI n'est pas une valeur entiere." ),"wms" ) );
        }
        if ( dpi<0 ) {
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre DPI est negative." ),"wms" ) );
        }

    } else {
        dpi = 0;
    }

    std::string formatOptionString = request->getParam ( "format_options" ).c_str();
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

        Request::toLowerCase ( key );
        Request::toLowerCase ( value );
        format_option.insert ( std::pair<std::string, std::string> ( key, value ) );
    }
    delete[] formatOptionChar;
    formatOptionChar = NULL;
    return NULL;
}

// Parameters for WMS GetFeatureInfo
DataStream* Rok4Server::getFeatureInfoParamWMS (
    Request* request, std::vector<Layer*>& layers, std::vector<Layer*>& query_layers,
    BoundingBox< double >& bbox, int& width, int& height, CRS& crs, std::string& format,
    std::vector<Style*>& styles, std::string& info_format, int& X, int& Y, int& feature_count,std::map <std::string, std::string >& format_option
){

    int dpi;
    DataStream* getMapError = getMapParamWMS(request,layers, bbox,width,height,crs,format,styles,format_option,dpi);
    
    if (getMapError) {
        return getMapError;
    }
    
    // VERSION
    std::string version = request->getParam ( "version" );
    if ( version == "" ) {
        //le parametre version est prioritaire sur wmtver
        version = request->getParam("wmtver");
        // Pas de verification car déjà fait dans getMapParam
    }
    
    // QUERY_LAYERS
    std::string str_query_layer = request->getParam ( "query_layers" );
    if ( str_query_layer == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre QUERY_LAYERS absent." ),"wms" ) );
    //Split layer Element
    std::vector<std::string> queryLayersString = split ( str_query_layer,',' );
    LOGGER_DEBUG ( _ ( "Nombre de couches demandees =" ) << queryLayersString.size() );
    if ( queryLayersString.size() > 1 ) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le nombre de couche interrogée est limité à 1." ),"wms" ) );
    }

    for (unsigned u1 = 0; u1 < queryLayersString.size(); u1++) {

        if ( Request::containForbiddenChars(queryLayersString.at(u1))) {
            LOGGER_WARN("Forbidden char detected in WMS query_layer : " << queryLayersString.at(u1));
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,_ ( "Query_Layer inconnu." ),"wms" ) );
        }

        Layer* lay = serverConf->getLayer(queryLayersString.at(u1));
        if ( lay == NULL )
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,_ ( "Query_Layer " ) +queryLayersString.at(u1)+_ ( " inconnu." ),"wms" ) );
    
        bool querylay_is_in_layer = false;
        std::vector<Layer*>::iterator itLay = layers.begin();
        for (unsigned int u2 = 0 ; u2 < layers.size(); u2++ ){
            if ( lay->getId() == layers.at(u2)->getId() ) {
                querylay_is_in_layer = true;
                break;
            }
        }
        if (querylay_is_in_layer == false){
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Query_Layer " ) +queryLayersString.at(u1)+_ ( " absent de layer." ),"wms" ) );
        }
    
        if (lay->isGetFeatureInfoAvailable() == false){
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_QUERYABLE,_ ( "Query_Layer " ) +queryLayersString.at(u1)+_ ( " non interrogeable." ),"wms" ) );
        }
        query_layers.push_back ( lay );
    }

    LOGGER_DEBUG ( _ ( "Nombre de couches requetées =" ) << query_layers.size() );


    // FEATURE_COUNT (facultative)
    std::string strFeatureCount = request->getParam ( "feature_count" );
    if ( strFeatureCount == "" ){
        feature_count = 1;
    } else {
        feature_count = atoi ( strFeatureCount.c_str() );
        if ( feature_count == 0 || feature_count == INT_MAX || feature_count == INT_MIN )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre FEATURE_COUNT n'est pas une valeur entiere." ),"wms" ) ) ;
        if ( feature_count<0 )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre FEATURE_COUNT est negative." ),"wms" ) );
    }


    // X ou I
    std::string xi = "i";
    if (version == "1.1.1") {
        // version 1.1.1
        xi = "x";
    } 
    std::string strX = request->getParam ( xi );
    if ( strX == "" ) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre X/I absent." ),"wms" ) );
    }
    char c;
    if (sscanf(strX.c_str(), "%d%c", &X, &c) != 1) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre X/I n'est pas un entier." ),"wms" ) );
    }
    if ( X<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre X/I est negative." ),"wms" ) );
    if ( X>width )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre X/I est superieure a la largeur fournie (width)." ),"wms" ) );
    
   

    // Y
    std::string yj = "j";
    if (version == "1.1.1") {
        // version 1.1.1
        yj = "y";
    }
    // version 1.3.0
    std::string strY = request->getParam ( yj );
    if ( strY == "" ) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre J/Y absent." ),"wms" ) );
    }
    if (sscanf(strY.c_str(), "%d%c", &Y, &c) != 1) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre J/Y n'est pas un entier." ),"wms" ) );
    }
    if ( Y<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre J/Y est negative." ),"wms" ) );
    if ( Y>height )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "La valeur du parametre J/Y est superieure a la largeur fournie (height)." ),"wms" ) );

    
    
    // INFO_FORMAT (facultative)
    unsigned int k;
    info_format = request->getParam ( "info_format" );
    if ( info_format == "" ){
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Parametre INFO_FORMAT vide." ),"wms" ) );
    } else {
        if ( Request::containForbiddenChars(info_format)) {
            LOGGER_WARN("Forbidden char detected in WMS info_format: " << info_format);
            return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,_ ( "Info_Format non gere par le service." ),"wms" ) );
        }
        if ( ! servicesConf->isInInfoFormatList(info_format) )
            return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,_ ( "Info_Format " ) +info_format+_ ( " non gere par le service." ),"wms" ) );
    }

    return NULL;
}

// Parameters for WMS GetCapabilities
DataStream* Rok4Server::getCapParamWMS ( Request* request, std::string& version ) {

    std::string location = request->getParam("location");
    if (location !="") {
        request->path = location;
    }
    version = request->getParam ( "version" );
    if ( version=="" ) {
        //---- WMS 1.1.1
        //le parametre version est prioritaire sur wmtver
        version = request->getParam("wmtver");
        if ( version=="") {
            version = "1.3.0";
            return NULL;
        }
        //----
    }
    //Do we have the requested version ?
    if ( version == "1.3.0" || version == "1.1.1" ){
        return NULL;
    }
    
    // Version number negotiation for WMS (should not be done for WMTS)
    // Ref: http://cite.opengeospatial.org/OGCTestData/wms/1.1.1/spec/wms1.1.1.html#basic_elements.version.negotiation
    // - Version higher than supported version: send the highest supported version
    // - Version lower than supported version: send the lowest supported version
    // - "If a version unknow to the server is requested, the server shall send the highest version less than the requested version." => 1.1.1
    // We compare the different values of the version number (l=left, m=middle, r=right)
    // Versions supported:
    std::string high_version = "1.3.0";
    int high_version_l = high_version[0]-48; //-48 is because of ASCII table, numbers start at position 48
    int high_version_m = high_version[2]-48;
    int high_version_r = high_version[4]-48;
    std::string low_version = "1.1.1";
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
    }else{
        version = low_version;
    }

    if ( version!="1.3.0" && version!="1.1.1"){
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Valeur du parametre VERSION invalide (1.1.1 et 1.3.0 disponibles seulement))" ),"wms" ) );
    }
    //normally unreachable code
    return NULL;
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
    if ( servicesConf->isInspire() ) {
        capabilitiesEl->SetAttribute ( "xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs/1.0" );
        capabilitiesEl->SetAttribute ( "xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
        capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd  http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd http://inspire.ec.europa.eu/schemas/common/1.0 http://inspire.ec.europa.eu/schemas/common/1.0/common.xsd" );
    }



    // Traitement de la partie service
    //----------------------------------
    TiXmlElement * serviceEl = new TiXmlElement ( "Service" );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "Name",servicesConf->getName() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "Title",servicesConf->getTitle() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "Abstract",servicesConf->getAbstract() ) );
    //KeywordList
    if ( servicesConf->getKeyWords()->size() != 0 ) {
        TiXmlElement * kwlEl = new TiXmlElement ( "KeywordList" );
        TiXmlElement * kwEl;
        for ( unsigned int i=0; i < servicesConf->getKeyWords()->size(); i++ ) {
            kwEl = new TiXmlElement ( "Keyword" );
            kwEl->LinkEndChild ( new TiXmlText ( servicesConf->getKeyWords()->at ( i ).getContent() ) );
            const std::map<std::string,std::string>* attributes = servicesConf->getKeyWords()->at ( i ).getAttributes();
            for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
            }
            kwlEl->LinkEndChild ( kwEl );
        }
        //kwlEl->LinkEndChild ( DocumentXML::buildTextNode ( "Keyword", ROK4_INFO ) );
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
    contactPersonPrimaryEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactPerson",servicesConf->getIndividualName() ) );
    contactPersonPrimaryEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactOrganization",servicesConf->getServiceProvider() ) );

    contactInformationEl->LinkEndChild ( contactPersonPrimaryEl );

    contactInformationEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactPosition",servicesConf->getIndividualPosition() ) );

    TiXmlElement * contactAddressEl = new TiXmlElement ( "ContactAddress" );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "AddressType",servicesConf->getAddressType() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "Address",servicesConf->getDeliveryPoint() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "City",servicesConf->getCity() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "StateOrProvince",servicesConf->getAdministrativeArea() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "PostCode",servicesConf->getPostCode() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "Country",servicesConf->getCountry() ) );

    contactInformationEl->LinkEndChild ( contactAddressEl );

    contactInformationEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactVoiceTelephone",servicesConf->getVoice() ) );

    contactInformationEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactFacsimileTelephone",servicesConf->getFacsimile() ) );

    contactInformationEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactElectronicMailAddress",servicesConf->getElectronicMailAddress() ) );

    serviceEl->LinkEndChild ( contactInformationEl );

    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "Fees",servicesConf->getFee() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "AccessConstraints",servicesConf->getAccessConstraint() ) );

    os << servicesConf->getLayerLimit();
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "LayerLimit",os.str() ) );
    os.str ( "" );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "MaxWidth",numToStr ( servicesConf->getMaxWidth() ) ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "MaxHeight",numToStr ( servicesConf->getMaxHeight() ) ) );

    capabilitiesEl->LinkEndChild ( serviceEl );



    // Traitement de la partie Capability
    //-----------------------------------
    TiXmlElement * capabilityEl = new TiXmlElement ( "Capability" );
    TiXmlElement * requestEl = new TiXmlElement ( "Request" );
    TiXmlElement * getCapabilitiestEl = new TiXmlElement ( "GetCapabilities" );

    getCapabilitiestEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format","text/xml" ) );
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

    if ( servicesConf->isPostEnabled() ) {
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
    for ( unsigned int i=0; i<servicesConf->getFormatList()->size(); i++ ) {
        getMapEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format",servicesConf->getFormatList()->at ( i ) ) );
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

    if ( servicesConf->isPostEnabled() ) {
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
    
    
    TiXmlElement * getFeatureInfoEl = new TiXmlElement ( "GetFeatureInfo" );
    for ( unsigned int i=0; i<servicesConf->getInfoFormatList()->size(); i++ ) {
        getFeatureInfoEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format",servicesConf->getInfoFormatList()->at ( i ) ) );
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

    if ( servicesConf->isPostEnabled() ) {
        TiXmlElement * PostEl = new TiXmlElement ( "Post" );
        onlineResourceEl = new TiXmlElement ( "OnlineResource" );
        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
        onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
        PostEl->LinkEndChild ( onlineResourceEl );
        HTTPEl->LinkEndChild ( PostEl );
    }

    DCPTypeEl->LinkEndChild ( HTTPEl );
    getFeatureInfoEl->LinkEndChild ( DCPTypeEl );

    requestEl->LinkEndChild ( getFeatureInfoEl );

    capabilityEl->LinkEndChild ( requestEl );

    //Exception
    TiXmlElement * exceptionEl = new TiXmlElement ( "Exception" );
    exceptionEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format","XML" ) );
    capabilityEl->LinkEndChild ( exceptionEl );

    // Inspire (extended Capability)
    if ( servicesConf->isInspire() ) {
        // TODO : en dur. A mettre dans la configuration du service (prevoir differents profils d'application possibles)
        TiXmlElement * extendedCapabilititesEl = new TiXmlElement ( "inspire_vs:ExtendedCapabilities" );

        // MetadataURL
        TiXmlElement * metadataUrlEl = new TiXmlElement ( "inspire_common:MetadataUrl" );
        metadataUrlEl->LinkEndChild ( DocumentXML::buildTextNode ( "inspire_common:URL", servicesConf->getWMSMetadataURL()->getHRef() ) );
        metadataUrlEl->LinkEndChild ( DocumentXML::buildTextNode ( "inspire_common:MediaType", servicesConf->getWMSMetadataURL()->getType() ) );
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

        capabilityEl->LinkEndChild ( extendedCapabilititesEl );
    }
    // Layer
    if ( serverConf->layersList.empty() ) {
        LOGGER_WARN ( _ ( "Liste de layers vide" ) );
    } else {
        // Parent layer
        TiXmlElement * parentLayerEl = new TiXmlElement ( "Layer" );
        // Title
        parentLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Title", "cache IGN" ) );
        // Abstract
        parentLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Abstract", "Cache IGN" ) );
        // Global CRS
        for ( unsigned int i=0; i < servicesConf->getGlobalCRSList()->size(); i++ ) {
            parentLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "CRS", servicesConf->getGlobalCRSList()->at ( i ).getRequestCode() ) );
        }
        // Child layers
        std::map<std::string, Layer*>::iterator it;
        for ( it=serverConf->layersList.begin(); it!=serverConf->layersList.end(); it++ ) {
            //Look if the layer is published in WMS
            if (it->second->getWMSAuthorized()) {
                TiXmlElement * childLayerEl = new TiXmlElement ( "Layer" );
                Layer* childLayer = it->second;
                // queryable
                if (childLayer->isGetFeatureInfoAvailable()){
                 childLayerEl->SetAttribute ( "queryable","1" ); 
                }
                // Name
                childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Name", childLayer->getId() ) );
                // Title
                childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Title", childLayer->getTitle() ) );
                // Abstract
                childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Abstract", childLayer->getAbstract() ) );
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
                std::vector<CRS> vectorCRS = childLayer->getWMSCRSList();
                int layerSize = vectorCRS.size();
                for (int i=0; i < layerSize; i++ ) {
                    childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "CRS", vectorCRS[i].getRequestCode() ) );
                }

                // GeographicBoundingBox
                TiXmlElement * gbbEl = new TiXmlElement ( "EX_GeographicBoundingBox" );

                os.str ( "" );
                os<<childLayer->getGeographicBoundingBox().minx;
                gbbEl->LinkEndChild ( DocumentXML::buildTextNode ( "westBoundLongitude", os.str() ) );
                os.str ( "" );
                os<<childLayer->getGeographicBoundingBox().maxx;
                gbbEl->LinkEndChild ( DocumentXML::buildTextNode ( "eastBoundLongitude", os.str() ) );
                os.str ( "" );
                os<<childLayer->getGeographicBoundingBox().miny;
                gbbEl->LinkEndChild ( DocumentXML::buildTextNode ( "southBoundLatitude", os.str() ) );
                os.str ( "" );
                os<<childLayer->getGeographicBoundingBox().maxy;
                gbbEl->LinkEndChild ( DocumentXML::buildTextNode ( "northBoundLatitude", os.str() ) );
                os.str ( "" );
                childLayerEl->LinkEndChild ( gbbEl );


                // BoundingBox
                if ( servicesConf->isInspire() ) {
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
                    for ( unsigned int i=0; i < servicesConf->getGlobalCRSList()->size(); i++ ) {
                        BoundingBox<double> bbox ( 0,0,0,0 );
                        if ( servicesConf->getGlobalCRSList()->at ( i ).validateBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) ) {
                            bbox = servicesConf->getGlobalCRSList()->at ( i ).boundingBoxFromGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy );
                        } else {
                            bbox = servicesConf->getGlobalCRSList()->at ( i ).boundingBoxFromGeographic ( servicesConf->getGlobalCRSList()->at ( i ).cropBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) );
                        }
                        CRS crs = servicesConf->getGlobalCRSList()->at ( i );
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
                        bbEl->SetAttribute ( "CRS",servicesConf->getGlobalCRSList()->at ( i ).getRequestCode() );
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

                    os.str ( "" );
                    os<< std::fixed << std::setprecision ( 9 );
                    os<<childLayer->getBoundingBox().minx;
                    bbEl->SetAttribute ( "minx",os.str() );
                    os.str ( "" );
                    os<<childLayer->getBoundingBox().miny;
                    bbEl->SetAttribute ( "miny",os.str() );
                    os.str ( "" );
                    os<<childLayer->getBoundingBox().maxx;
                    bbEl->SetAttribute ( "maxx",os.str() );
                    os.str ( "" );
                    os<<childLayer->getBoundingBox().maxy;
                    bbEl->SetAttribute ( "maxy",os.str() );

                    childLayerEl->LinkEndChild ( bbEl );
                }
                //MetadataURL
                if ( childLayer->getMetadataURLs().size() != 0 ) {
                    for ( unsigned int i=0; i < childLayer->getMetadataURLs().size(); ++i ) {
                        TiXmlElement * mtdURLEl = new TiXmlElement ( "MetadataURL" );
                        MetadataURL mtdUrl = childLayer->getMetadataURLs().at ( i );
                        mtdURLEl->SetAttribute ( "type", mtdUrl.getType() );
                        mtdURLEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format",mtdUrl.getFormat() ) );

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
                        styleEl->LinkEndChild ( DocumentXML::buildTextNode ( "Name", style->getIdentifier().c_str() ) );
                        int j;
                        for ( j=0 ; j < style->getTitles().size(); ++j ) {
                            styleEl->LinkEndChild ( DocumentXML::buildTextNode ( "Title", style->getTitles() [j].c_str() ) );
                        }
                        for ( j=0 ; j < style->getAbstracts().size(); ++j ) {
                            styleEl->LinkEndChild ( DocumentXML::buildTextNode ( "Abstract", style->getAbstracts() [j].c_str() ) );
                        }
                        for ( j=0 ; j < style->getLegendURLs().size(); ++j ) {
                            LOGGER_DEBUG ( _ ( "LegendURL" ) << style->getId() );
                            LegendURL legendURL = style->getLegendURLs() [j];
                            TiXmlElement* legendURLEl = new TiXmlElement ( "LegendURL" );

                            TiXmlElement* onlineResourceEl = new TiXmlElement ( "OnlineResource" );
                            onlineResourceEl->SetAttribute ( "xlink:type","simple" );
                            onlineResourceEl->SetAttribute ( "xlink:href", legendURL.getHRef() );
                            legendURLEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format", legendURL.getFormat() ) );
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
                childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "MinScaleDenominator", os.str() ) );
                os.str ( "" );
                os<<childLayer->getMaxRes() *1000/0.28;
                childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "MaxScaleDenominator", os.str() ) );

                // TODO : gerer le cas des CRS avec des unites en degres

                /* TODO:
                 *
                 layer->getAuthority();
                 layer->getOpaque();

                */
                LOGGER_DEBUG ( _ ( "Layer Fini" ) );
                parentLayerEl->LinkEndChild ( childLayerEl );
            }
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
    std::string dtdTag="]DTD[";  ///Tag à remplacer par la déclaration de DTD, tinyXMl ne gère pas les DTD. #HackDeLaMortQuiTue par Thibbo
    
    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration ( "1.0", "UTF-8", "" );
    doc.LinkEndChild ( decl );
    

    
    TiXmlComment * dtdEl = new TiXmlComment ( dtdTag.c_str() );
    doc.LinkEndChild ( dtdEl );
    
    std::ostringstream os;

    TiXmlElement * capabilitiesEl = new TiXmlElement ( "WMT_MS_Capabilities" );
    capabilitiesEl->SetAttribute ( "version","1.1.1" );


    // Traitement de la partie service
    //----------------------------------
    TiXmlElement * serviceEl = new TiXmlElement ( "Service" );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "Name",servicesConf->getName() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "Title",servicesConf->getTitle() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "Abstract",servicesConf->getAbstract() ) );
    //KeywordList
    if ( servicesConf->getKeyWords()->size() != 0 ) {
        TiXmlElement * kwlEl = new TiXmlElement ( "KeywordList" );
        TiXmlElement * kwEl;
        for ( unsigned int i=0; i < servicesConf->getKeyWords()->size(); i++ ) {
            kwEl = new TiXmlElement ( "Keyword" );
            kwEl->LinkEndChild ( new TiXmlText ( servicesConf->getKeyWords()->at ( i ).getContent() ) );
            const std::map<std::string,std::string>* attributes = servicesConf->getKeyWords()->at ( i ).getAttributes();
            for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
            }
            kwlEl->LinkEndChild ( kwEl );
        }
        //kwlEl->LinkEndChild ( DocumentXML::buildTextNode ( "Keyword", ROK4_INFO ) );
        serviceEl->LinkEndChild ( kwlEl );
    }
    //OnlineResource
    TiXmlElement * onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
    onlineResourceEl->SetAttribute ( "xlink:href",hostNameTag );
    
    serviceEl->LinkEndChild ( onlineResourceEl );
    // ContactInformation
    TiXmlElement * contactInformationEl = new TiXmlElement ( "ContactInformation" );

    TiXmlElement * contactPersonPrimaryEl = new TiXmlElement ( "ContactPersonPrimary" );
    contactPersonPrimaryEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactPerson",servicesConf->getIndividualName() ) );
    contactPersonPrimaryEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactOrganization",servicesConf->getServiceProvider() ) );

    contactInformationEl->LinkEndChild ( contactPersonPrimaryEl );

    contactInformationEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactPosition",servicesConf->getIndividualPosition() ) );

    TiXmlElement * contactAddressEl = new TiXmlElement ( "ContactAddress" );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "AddressType",servicesConf->getAddressType() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "Address",servicesConf->getDeliveryPoint() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "City",servicesConf->getCity() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "StateOrProvince",servicesConf->getAdministrativeArea() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "PostCode",servicesConf->getPostCode() ) );
    contactAddressEl->LinkEndChild ( DocumentXML::buildTextNode ( "Country",servicesConf->getCountry() ) );

    contactInformationEl->LinkEndChild ( contactAddressEl );

    contactInformationEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactVoiceTelephone",servicesConf->getVoice() ) );

    contactInformationEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactFacsimileTelephone",servicesConf->getFacsimile() ) );

    contactInformationEl->LinkEndChild ( DocumentXML::buildTextNode ( "ContactElectronicMailAddress",servicesConf->getElectronicMailAddress() ) );

    serviceEl->LinkEndChild ( contactInformationEl );

    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "Fees",servicesConf->getFee() ) );
    serviceEl->LinkEndChild ( DocumentXML::buildTextNode ( "AccessConstraints",servicesConf->getAccessConstraint() ) );

    capabilitiesEl->LinkEndChild ( serviceEl );



    // Traitement de la partie Capability
    //-----------------------------------
    TiXmlElement * capabilityEl = new TiXmlElement ( "Capability" );
    TiXmlElement * requestEl = new TiXmlElement ( "Request" );
    TiXmlElement * getCapabilitiestEl = new TiXmlElement ( "GetCapabilities" );

    getCapabilitiestEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format","text/xml" ) );
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

    if ( servicesConf->isPostEnabled() ) {
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
    for ( unsigned int i=0; i<servicesConf->getFormatList()->size(); i++ ) {
        getMapEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format",servicesConf->getFormatList()->at ( i ) ) );
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

    if ( servicesConf->isPostEnabled() ) {
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
    
    TiXmlElement * getFeatureInfoEl = new TiXmlElement ( "GetFeatureInfo" );
    for ( unsigned int i=0; i<servicesConf->getInfoFormatList()->size(); i++ ) {
        getFeatureInfoEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format",servicesConf->getInfoFormatList()->at ( i ) ) );
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

    if ( servicesConf->isPostEnabled() ) {
        TiXmlElement * PostEl = new TiXmlElement ( "Post" );
        onlineResourceEl = new TiXmlElement ( "OnlineResource" );
        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
        onlineResourceEl->SetAttribute ( "xlink:href",pathTag );
        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
        PostEl->LinkEndChild ( onlineResourceEl );
        HTTPEl->LinkEndChild ( PostEl );
    }

    DCPTypeEl->LinkEndChild ( HTTPEl );
    getFeatureInfoEl->LinkEndChild ( DCPTypeEl );

    requestEl->LinkEndChild ( getFeatureInfoEl );

    capabilityEl->LinkEndChild ( requestEl );

    //Exception
    TiXmlElement * exceptionEl = new TiXmlElement ( "Exception" );
    exceptionEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format","XML" ) );
    capabilityEl->LinkEndChild ( exceptionEl );

    // Layer
    if ( serverConf->layersList.empty() ) {
        LOGGER_WARN ( _ ( "Liste de layers vide" ) );
    } else {
        // Parent layer
        TiXmlElement * parentLayerEl = new TiXmlElement ( "Layer" );
        // Title
        parentLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Title", "cache IGN" ) );
        // Abstract
        parentLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Abstract", "Cache IGN" ) );
        // Global CRS
        for ( unsigned int i=0; i < servicesConf->getGlobalCRSList()->size(); i++ ) {
            parentLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "SRS", servicesConf->getGlobalCRSList()->at ( i ).getRequestCode() ) );
        }
        // Child layers
        std::map<std::string, Layer*>::iterator it;
        for ( it=serverConf->layersList.begin(); it!=serverConf->layersList.end(); it++ ) {
            //Look if the layer is published in WMS
            if (it->second->getWMSAuthorized()) {
                TiXmlElement * childLayerEl = new TiXmlElement ( "Layer" );
                Layer* childLayer = it->second;
        if (childLayer->isGetFeatureInfoAvailable()){
         childLayerEl->SetAttribute ( "queryable","1" ); 
        }
                // Name
                childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Name", childLayer->getId() ) );
                // Title
                childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Title", childLayer->getTitle() ) );
                // Abstract
                childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "Abstract", childLayer->getAbstract() ) );
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
                std::vector<CRS> vectorCRS = childLayer->getWMSCRSList();
                int layerSize = vectorCRS.size();
                for (int i=0; i < layerSize; i++ ) {
                    childLayerEl->LinkEndChild ( DocumentXML::buildTextNode ( "SRS", vectorCRS[i].getRequestCode() ) );
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
                if ( servicesConf->isInspire() ) {
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
                    for ( unsigned int i=0; i < servicesConf->getGlobalCRSList()->size(); i++ ) {
                        BoundingBox<double> bbox ( 0,0,0,0 );
                        if ( servicesConf->getGlobalCRSList()->at ( i ).validateBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) ) {
                            bbox = servicesConf->getGlobalCRSList()->at ( i ).boundingBoxFromGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy );
                        } else {
                            bbox = servicesConf->getGlobalCRSList()->at ( i ).boundingBoxFromGeographic ( servicesConf->getGlobalCRSList()->at ( i ).cropBBoxGeographic ( childLayer->getGeographicBoundingBox().minx,childLayer->getGeographicBoundingBox().miny,childLayer->getGeographicBoundingBox().maxx,childLayer->getGeographicBoundingBox().maxy ) );
                        }
                        CRS crs = servicesConf->getGlobalCRSList()->at ( i );
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
                        bbEl->SetAttribute ( "SRS",servicesConf->getGlobalCRSList()->at ( i ).getRequestCode() );
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
                        mtdURLEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format",mtdUrl.getFormat() ) );

                        TiXmlElement* onlineResourceEl = new TiXmlElement ( "OnlineResource" );
                        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
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
                        styleEl->LinkEndChild ( DocumentXML::buildTextNode ( "Name", style->getIdentifier().c_str() ) );
                        int j;
                        for ( j=0 ; j < style->getTitles().size(); ++j ) {
                            styleEl->LinkEndChild ( DocumentXML::buildTextNode ( "Title", style->getTitles() [j].c_str() ) );
                        }
                        for ( j=0 ; j < style->getAbstracts().size(); ++j ) {
                            styleEl->LinkEndChild ( DocumentXML::buildTextNode ( "Abstract", style->getAbstracts() [j].c_str() ) );
                        }
                        for ( j=0 ; j < style->getLegendURLs().size(); ++j ) {
                            LOGGER_DEBUG ( _ ( "LegendURL" ) << style->getId() );
                            LegendURL legendURL = style->getLegendURLs() [j];
                            TiXmlElement* legendURLEl = new TiXmlElement ( "LegendURL" );

                            TiXmlElement* onlineResourceEl = new TiXmlElement ( "OnlineResource" );
                            onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
                            onlineResourceEl->SetAttribute ( "xlink:type","simple" );
                            onlineResourceEl->SetAttribute ( "xlink:href", legendURL.getHRef() );
                            legendURLEl->LinkEndChild ( DocumentXML::buildTextNode ( "Format", legendURL.getFormat() ) );
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
            }
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
    

    // Suite du hack, on remplace le commentaire
    wmsCapaTemplate.replace ( wmsCapaTemplate.find("<!--" + dtdTag + "-->") ,dtdTag.length()+7 , "<!DOCTYPE WMT_MS_Capabilities SYSTEM \"http://schemas.opengis.net/wms/1.1.1/WMS_MS_Capabilities.dtd\">" );
    
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

DataStream* Rok4Server::WMSGetCapabilities ( Request* request ) {
    if ( ! serverConf->supportWMS ) {
        // Return Error
    }
    std::string version;
    DataStream* errorResp = getCapParamWMS ( request, version );
    if ( errorResp ) {
        LOGGER_ERROR ( _ ( "Probleme dans les parametres de la requete getCapabilities" ) );
        return errorResp;
    }

    /* concaténation des fragments invariant de capabilities en intercalant les
     * parties variables dépendantes de la requête */
    std::string capa;
    std::vector<std::string> capaFrag;
    std::map<std::string,std::vector<std::string> >::iterator it = wmsCapaFrag.find(version);
    capaFrag = it->second;
    capa = capaFrag[0] + request->scheme + request->hostName;
    for ( int i=1; i < capaFrag.size()-1; i++ ) {
        capa = capa + capaFrag[i] + request->scheme + request->hostName + request->path + "?";
    }
    capa = capa + capaFrag.back();

    return new MessageDataStream ( capa,"text/xml" );
}
