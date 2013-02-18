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

/**
 * \file CRS.cpp
 * \~french
 * \brief Implémente de la gestion des systèmes de référence
 * \~english
 * \brief implement the reference systems handler
 */

#include "CRS.h"
#include "Logger.h"
#include "Grid.h"
#include <proj_api.h>
#include "intl.h"
#include "config.h"

/**
 * \~french \brief Code utilisé en cas de non correspondance avec les référentiel de Proj
 * \~english \brief Used code when no corresponding Proj code is found
 */
#define NO_PROJ4_CODE "noProj4Code"

/**
 * \~french \brief Transforme la chaîne fournie en minuscule
 * \~english \brief Transform the string to lowercase
 */
std::string toLowerCase ( std::string str ) {
    std::string lc_str=str;
    for ( int i = 0; str[i]; i++ ) lc_str[i] = tolower ( str[i] );
    return lc_str;
}

/**
 * \~french \brief Transforme la chaîne fournie en majuscule
 * \~english \brief Transform the string to uppercase
 */
std::string toUpperCase ( std::string str ) {
    std::string uc_str=str;
    for ( int i = 0; str[i]; i++ ) uc_str[i] = toupper ( str[i] );
    return uc_str;
}

/**
 * \~french \brief Teste si la chaîne fournie est un CRS compatible avec les registres de Proj
 * \~english \brief Test whether the string represent a Proj compatible CRS
 */
bool isCrsProj4Compatible ( std::string crs ) {
    projCtx ctx = pj_ctx_alloc();
    projPJ pj=pj_init_plus_ctx ( ctx, ( "+init=" + crs +" +wktext" ).c_str() );
    if ( !pj ) {
        int err = pj_ctx_get_errno ( ctx );
        char *msg = pj_strerrno ( err );
        // LOGGER_DEBUG(_("erreur d initialisation ") << crs << " " << msg);
        pj_ctx_free ( ctx );
        return false;
    }
    bool isCompatible;
    if ( pj ) isCompatible=true;
    else isCompatible=false;
    pj_free ( pj );
    pj_ctx_free ( ctx );
    return isCompatible;
}

/**
 * \~french \brief Teste si la chaîne fournie est un CRS géographique
 * \~english \brief Test whether the string represent a geographic CRS
 */
bool isCrsLongLat ( std::string crs ) {
    projCtx ctx = pj_ctx_alloc();
    projPJ pj=pj_init_plus_ctx ( ctx, ( "+init=" + crs +" +wktext" ).c_str() );
    if ( !pj ) {
        int err = pj_ctx_get_errno ( ctx );
        char *msg = pj_strerrno ( err );
        // LOGGER_DEBUG(_("erreur d initialisation ") << crs << " " << msg);
        pj_ctx_free ( ctx );
        return false;
    }
    bool isLongLat=pj_is_latlong ( pj );
    pj_free ( pj );
    pj_ctx_free ( ctx );
    return isLongLat;
}

CRS::CRS() : definitionArea(-90.0,-180.0,90.0,180.0) {
    proj4Code = NO_PROJ4_CODE;
}

CRS::CRS ( std::string crs_code ) : definitionArea(-90.0,-180.0,90.0,180.0) {
    requestCode=crs_code;
    buildProj4Code();
    fetchDefinitionArea();
}

CRS::CRS ( const CRS& crs ) : definitionArea(crs.definitionArea) {
    requestCode=crs.requestCode;
    proj4Code=crs.proj4Code;
    fetchDefinitionArea();
}


CRS& CRS::operator= ( const CRS& other ) {
    if (this != &other) {
        this->proj4Code = other.proj4Code;
        this->requestCode = other.requestCode;
        this->definitionArea = other.definitionArea;
    }
    return *this;
}


void CRS::fetchDefinitionArea() {
    projCtx ctx = pj_ctx_alloc();
    projPJ pj=pj_init_plus_ctx ( ctx, ( "+init=" + proj4Code +" +wktext" ).c_str() );
    if ( !pj ) {
        int err = pj_ctx_get_errno ( ctx );
        char *msg = pj_strerrno ( err );
        // LOGGER_DEBUG(_("erreur d initialisation ") << crs << " " << msg);
        pj_ctx_free ( ctx );
        return;
    }
    pj_get_def_area(pj, &(definitionArea.xmin), &(definitionArea.ymin), &(definitionArea.xmax), &(definitionArea.ymax));
    //LOGGER_DEBUG(proj4Code); 
    //definitionArea.print();
    pj_free ( pj );
    pj_ctx_free ( ctx );
}


void CRS::buildProj4Code() {
    proj4Code=NO_PROJ4_CODE;
    if ( isCrsProj4Compatible ( requestCode ) )
        proj4Code=requestCode;
    else if ( isCrsProj4Compatible ( toLowerCase ( requestCode ) ) )
        proj4Code=toLowerCase ( requestCode );
    else if ( isCrsProj4Compatible ( toUpperCase ( requestCode ) ) )
        proj4Code=toUpperCase ( requestCode );
    // TODO : rajouter des tests en dur (ou charges depuis la conf), correspondance EPSG <-> IGNF
    // Commencer par ces tests (Ex : tout exprimer par defaut en EPSG)
    // ISO 19128 6.7.3.2
    else if ( requestCode=="CRS:84" )
        proj4Code="epsg:4326";
}


bool CRS::isProj4Compatible() {
    return proj4Code!=NO_PROJ4_CODE;
}


bool CRS::isLongLat() {
    return isCrsLongLat ( proj4Code );
}


long double CRS::getMetersPerUnit() {
    // Hypothese : un CRS en long/lat est en degres
    // R=6378137m
    if ( isLongLat() )
        return 111319.49079327357264771338267056;
    else
        return 1.0;
}


void CRS::setRequestCode ( std::string crs ) {
    requestCode=crs;
    buildProj4Code();
    fetchDefinitionArea();
}


bool CRS::cmpRequestCode ( std::string crs ) {
    return toLowerCase ( requestCode ) ==toLowerCase ( crs );
}


std::string CRS::getAuthority() {
    size_t pos=requestCode.find ( ':' );
    if ( pos<1 || pos >=requestCode.length() ) {
        LOGGER_ERROR ( _("Erreur sur le CRS ")<<requestCode<< _(" : absence de separateur") );
        pos=requestCode.length();
    }
    return ( requestCode.substr ( 0,pos ) );
}


std::string CRS::getIdentifier() {
    size_t pos=requestCode.find ( ':' );
    if ( pos<1 || pos >=requestCode.length() ) {
        LOGGER_ERROR ( _("Erreur sur le CRS ")<<requestCode<< _(" : absence de separateur") );
        pos=-1;
    }
    return ( requestCode.substr ( pos+1 ) );
}


bool CRS::operator== ( const CRS& crs ) const {
    return ( proj4Code==crs.proj4Code );
}



bool CRS::operator!= ( const CRS& crs ) const {
    return ! ( *this == crs );
}



BoundingBox<double> CRS::boundingBoxFromGeographic ( BoundingBox< double > geographicBBox ) {
    Grid* grid = new Grid ( 256,256,geographicBBox );
    grid->reproject ( "epsg:4326",proj4Code );
    BoundingBox<double> bbox = grid->bbox;
    delete grid;
    grid=0;
    return bbox;
}


BoundingBox< double > CRS::boundingBoxFromGeographic ( double minx, double miny, double maxx, double maxy ) {
    return boundingBoxFromGeographic ( BoundingBox<double> ( minx,miny,maxx,maxy ) );
}


BoundingBox<double> CRS::boundingBoxToGeographic ( BoundingBox< double > projectedBBox ) {
    Grid* grid = new Grid ( 256,256,projectedBBox );
    grid->reproject ( proj4Code,"epsg:4326" );
    BoundingBox<double> bbox = grid->bbox;
    delete grid;
    grid=0;
    return bbox;
}


BoundingBox<double> CRS::boundingBoxToGeographic ( double minx, double miny, double maxx, double maxy ) {
    return boundingBoxToGeographic ( BoundingBox<double> ( minx,miny,maxx,maxy ) );
}


bool CRS::validateBBox ( BoundingBox< double > BBox ) {
    return validateBBoxGeographic(boundingBoxToGeographic( BBox));
}


bool CRS::validateBBox ( double minx, double miny, double maxx, double maxy ) {
    return validateBBoxGeographic( boundingBoxToGeographic ( minx,miny,maxx,maxy ) );
}


bool CRS::validateBBoxGeographic ( BoundingBox< double > BBox ) {
    bool valid = true;
    if (BBox.xmin > definitionArea.xmax || BBox.xmin < definitionArea.xmin ||
        BBox.xmax > definitionArea.xmax || BBox.xmax < definitionArea.xmin ||
        BBox.ymin > definitionArea.ymax || BBox.ymin < definitionArea.ymin ||
        BBox.ymax > definitionArea.ymax || BBox.ymax < definitionArea.ymin ){
            valid = false;
        }
    
    
    return valid;
}


bool CRS::validateBBoxGeographic ( double minx, double miny, double maxx, double maxy ) {
    return validateBBoxGeographic( BoundingBox<double> ( minx,miny,maxx,maxy ) );
}


BoundingBox< double > CRS::cropBBox ( BoundingBox< double > BBox ) {
    BoundingBox<double> defArea = boundingBoxFromGeographic(definitionArea);
    double minx = BBox.xmin, miny = BBox.ymin, maxx = BBox.xmax, maxy = BBox.ymax;
    if (BBox.xmin < defArea.xmin) {
        minx = defArea.xmin;
    } 
    if (BBox.xmax > defArea.xmax) {
        maxx = defArea.xmax;
    }
    if (BBox.xmin > defArea.xmax || BBox.xmax < defArea.xmin) {
        minx = maxx = 0;
    }
    if (BBox.ymin < defArea.ymin) {
        miny = defArea.ymin;
    }
    if (BBox.ymax > defArea.ymax) {
        maxy = defArea.ymax;
    }
    if (BBox.ymin > defArea.ymax || BBox.ymax < defArea.ymin) {
        miny = maxy = 0;
    }
    return BoundingBox<double> ( minx,miny,maxx,maxy );
}


BoundingBox< double > CRS::cropBBox ( double minx, double miny, double maxx, double maxy ) {
    return cropBBox( BoundingBox<double> ( minx,miny,maxx,maxy ) );
}

BoundingBox< double > CRS::cropBBoxGeographic ( BoundingBox< double > BBox ) {
    double minx = BBox.xmin, miny = BBox.ymin, maxx = BBox.xmax, maxy = BBox.ymax;
    if (BBox.xmin < definitionArea.xmin) {
        minx = definitionArea.xmin;
    } 
    if (BBox.xmax > definitionArea.xmax) {
        maxx = definitionArea.xmax;
    }
    if (BBox.xmin > definitionArea.xmax || BBox.xmax < definitionArea.xmin) {
        minx = maxx = 0;
    }
    if (BBox.ymin < definitionArea.ymin) {
        miny = definitionArea.ymin;
    }
    if (BBox.ymax > definitionArea.ymax) {
        maxy = definitionArea.ymax;
    }
    if (BBox.ymin > definitionArea.ymax || BBox.ymax < definitionArea.ymin) {
        miny = maxy = 0;
    }
    return BoundingBox<double> ( minx,miny,maxx,maxy );
}

BoundingBox< double > CRS::cropBBoxGeographic ( double minx, double miny, double maxx, double maxy ) {
    return cropBBoxGeographic( BoundingBox<double> ( minx,miny,maxx,maxy ) );
}



CRS::~CRS() {

}
