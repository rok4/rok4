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
 * \brief Implémente de la de gestion des systèmes de référence
 * \~english
 * \brief implement the reference systems handler
 */

#include "CRS.h"
#include "Logger.h"
#include "Grid.h"
#include <proj_api.h>
#include "intl.h"
#include "config.h"

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

/**
 * \~french
 * \brief Crée un CRS sans correspondance avec une entrée du registre PROJ
 * \~english
 * \brief Create a CRS without Proj correspondance
 */
CRS::CRS() : definitionArea(-90.0,-180.0,90.0,180.0) {
    proj4Code = NO_PROJ4_CODE;
}

/**
 * \~french
 * \brief Crée un CRS à partir de son identifiant
 * \details La chaîne est comparée, sans prendre en compte la casse, avec les registres de Proj. Puis la zone de validité est récupérée dans le registre.
 * \param[in] crs_code identifiant du CRS 
 * \~english
 * \brief Create a CRS from its identifier
 * \details The string is compared, case insensitively, to Proj registry. Then the corresponding definition area is fetched from Proj.
 * \param[in] crs_code CRS identifier
 */

CRS::CRS ( std::string crs_code ) : definitionArea(-90.0,-180.0,90.0,180.0) {
    requestCode=crs_code;
    buildProj4Code();
    fetchDefinitionArea();
}

/**
 * \~french
 * \brief Constructeur de copie
 * \~english
 * \brief Copy constructor
 */

CRS::CRS ( const CRS& crs ) : definitionArea(crs.definitionArea) {
    requestCode=crs.requestCode;
    proj4Code=crs.proj4Code;
    fetchDefinitionArea();
}

/**
 * \~french
 * \brief Affectation
 * \~english
 * \brief Assignement
 */
CRS& CRS::operator= ( const CRS& other ) {
    if (this != &other) {
        this->proj4Code = other.proj4Code;
        this->requestCode = other.requestCode;
        this->definitionArea = other.definitionArea;
    }
    return *this;
}

/**
 * \~french
 * \brief Récupère l'emprise de définition du CRS dans les registres Proj
 * \~english
 * \brief Fetch the CRS definition area from Proj registries
 */
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

/**
 * \~french
 * \brief Détermine a partir du code du CRS passe dans la requete le code Proj correspondant
 * \~english
 * \brief Determine the Proj code from the requested CRS
 */
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

/**
 * \~french
 * \brief Test si le CRS possède un équivalent dans Proj
 * \return vrai si disponible dans Proj
 * \~english
 * \brief Test whether the CRS has a Proj equivalent
 * \return true if available in Proj
 */
bool CRS::isProj4Compatible() {
    return proj4Code!=NO_PROJ4_CODE;
}

/**
 * \~french
 * \brief Test si le CRS est géographique
 * \return vrai si géographique
 * \~english
 * \brief Test whether the CRS is geographic
 * \return true if geographic
 */
bool CRS::isLongLat() {
    return isCrsLongLat ( proj4Code );
}

/**
 * \~french
 * \brief Le nombre de mètre par unité du CRS
 * \return rapport entre le mètre et l'unité du CRS
 * \todo supporter les CRS autres qu'en degré et en mètre
 * \~english
 * \brief Amount of meter in one CRS's unit
 * \return quotient between meter and CRS's unit
 * \todo support all CRS types not only projected in meter and geographic in degree
 */
long double CRS::getMetersPerUnit() {
    // Hypothese : un CRS en long/lat est en degres
    // R=6378137m
    if ( isLongLat() )
        return 111319.49079327357264771338267056;
    else
        return 1.0;
}

/**
 * \~french
 * \brief Définit le nouveau code que le CRS représentera
 * \details La chaîne est comparée, sans prendre en compte la casse, avec les registres de Proj. Puis la zone de validité est récupérée dans le registre.
 * \param[in] crs_code identifiant du CRS 
 * \~english
 * \brief Assign a new code to the CRS
 * \details The string is compared, case insensitively, to Proj registry. Then the corresponding definition area is fetched from Proj.
 * \param[in] crs_code CRS identifier
 */
void CRS::setRequestCode ( std::string crs ) {
    requestCode=crs;
    buildProj4Code();
    fetchDefinitionArea();
}

/**
 * \~french
 * \brief Compare le code fournit lors de la création du CRS avec la chaîne
 * \param[in] crs chaîne à comparer
 * \return vrai si identique (insenble à la casse)
 * \~english
 * \brief Compare the CRS original code with the supplied string
 * \param[in] crs string for comparison 
 * \return true if identic (case insensitive)
 */
bool CRS::cmpRequestCode ( std::string crs ) {
    return toLowerCase ( requestCode ) ==toLowerCase ( crs );
}

/**
 * \~french
 * \brief Retourne l'authorité du CRS
 * \return l'identifiant de l'authorité 
 * \~english
 * \brief Return the CRS authority
 * \return the authority identifier
 */
std::string CRS::getAuthority() {
    size_t pos=requestCode.find ( ':' );
    if ( pos<1 || pos >=requestCode.length() ) {
        LOGGER_ERROR ( _("Erreur sur le CRS ")<<requestCode<< _(" : absence de separateur") );
        pos=requestCode.length();
    }
    return ( requestCode.substr ( 0,pos ) );
}

/**
 * \~french
 * \brief Retourne l'identifiant du CRS sans l'authorité
 * \return l'identifiant du système
 * \~english
 * \brief Return the CRS identifier without the authority
 * \return the system identifier
 */
std::string CRS::getIdentifier() {
    size_t pos=requestCode.find ( ':' );
    if ( pos<1 || pos >=requestCode.length() ) {
        LOGGER_ERROR ( _("Erreur sur le CRS ")<<requestCode<< _(" : absence de separateur") );
        pos=-1;
    }
    return ( requestCode.substr ( pos+1 ) );
}

/**
 * \~french
 * \brief Test d'egalite de 2 CRS
 * \return true s'ils ont le meme code Proj, false sinon
 * \~english
 * \brief Test whether 2 CRS are equals
 * \return true if they share the same Proj identifier
 */
bool CRS::operator== ( const CRS& crs ) const {
    return ( proj4Code==crs.proj4Code );
}


/**
 * \~french
 * \brief Test d'inégalite de 2 CRS
 * \return true s'ils ont un code Proj différent, false sinon
 * \~english
 * \brief Test whether 2 CRS are different
 * \return true if the the Proj identifier is different
 */
bool CRS::operator!= ( const CRS& crs ) const {
    return ! ( *this == crs );
}


/**
 * \~french
 * \brief Calcule la BoundingBox dans le CRS courant à partir de la BoundingBox Géographique
 * \param[in] geographicBBox une emprise définit en WGS84
 * \return l'emprise dans le CRS courant
 * \~english
 * \brief Compute a BoundingBox in the current CRS from a geographic BoundingBox
 * \param[in] geographicBBox a BoundingBox in geographic coordinate WGS84
 * \return the same BoundingBox in the current CRS
 */
BoundingBox<double> CRS::boundingBoxFromGeographic ( BoundingBox< double > geographicBBox ) {
    Grid* grid = new Grid ( 256,256,geographicBBox );
    grid->reproject ( "epsg:4326",proj4Code );
    BoundingBox<double> bbox = grid->bbox;
    delete grid;
    grid=0;
    return bbox;
}

/**
 * \~french
 * \brief Calcule la BoundingBox dans le CRS courant à partir de la BoundingBox Géographique
 * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit en WGS84
 * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit en WGS84
 * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit en WGS84
 * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit en WGS84
 * \return l'emprise dans le CRS courant
 * \~english
 * \brief Compute a BoundingBox in the current CRS from a geographic BoundingBox
 * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in WGS84
 * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in WGS84
 * \param[in] maxx x-coordinate of the top right corner of the boundingBox in WGS84
 * \param[in] maxy y-coordinate of the top right corner of the boundingBox in WGS84
 * \return the same BoundingBox in the current CRS
 */
BoundingBox< double > CRS::boundingBoxFromGeographic ( double minx, double miny, double maxx, double maxy ) {
    return boundingBoxFromGeographic ( BoundingBox<double> ( minx,miny,maxx,maxy ) );
}

/**
 * \~french
 * \brief Calcule la BoundingBox Géographique à partir de la BoundingBox dans le CRS courant
 * \param[in] geographicBBox une emprise définit dans le CRS courant
 * \return l'emprise en WGS84
 * \~english
 * \brief Compute a geographic BoundingBox from a BoundingBox in the current CRS
 * \param[in] geographicBBox a BoundingBox in the current CRS
 * \return the same BoundingBox in WGS84
 */
BoundingBox<double> CRS::boundingBoxToGeographic ( BoundingBox< double > projectedBBox ) {
    Grid* grid = new Grid ( 256,256,projectedBBox );
    grid->reproject ( proj4Code,"epsg:4326" );
    BoundingBox<double> bbox = grid->bbox;
    delete grid;
    grid=0;
    return bbox;
}

/**
 * \~french
 * \brief Calcule la BoundingBox Géographique à partir de la BoundingBox dans le CRS courant
 * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit dans le CRS courant
 * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit dans le CRS courant
 * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit dans le CRS courant
 * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit dans le CRS courant
 * \return l'emprise en WGS84
 * \~english
 * \brief Compute a geographic BoundingBox from a BoundingBox in the current CRS
 * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in the current CRS
 * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in the current CRS
 * \param[in] maxx x-coordinate of the top right corner of the boundingBox in the current CRS
 * \param[in] maxy y-coordinate of the top right corner of the boundingBox in the current CRS
 * \return the same BoundingBox in WGS84
 */
BoundingBox<double> CRS::boundingBoxToGeographic ( double minx, double miny, double maxx, double maxy ) {
    return boundingBoxToGeographic ( BoundingBox<double> ( minx,miny,maxx,maxy ) );
}

/**
 * \~french
 * \brief Vérifie que la BoundingBox est dans le domaine de définition de la projection
 * \param[in] geographicBBox une emprise définit dans le CRS courant
 * \return true si incluse
 * \~english
 * \brief Verify if the supplied BoundingBox is in the CRS definition area
 * \param[in] geographicBBox a BoundingBox in the current CRS
 * \return true if inside
 */
bool CRS::validateBBox ( BoundingBox< double > BBox ) {
    return validateBBoxGeographic(boundingBoxToGeographic( BBox));
}

/**
 * \~french
 * \brief Vérifie que la BoundingBox est dans le domaine de définition de la projection
 * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit dans le CRS courant
 * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit dans le CRS courant
 * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit dans le CRS courant
 * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit dans le CRS courant
 * \return true si incluse
 * \~english
 * \brief Verify if the supplied BoundingBox is in the CRS definition area
 * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in the current CRS
 * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in the current CRS
 * \param[in] maxx x-coordinate of the top right corner of the boundingBox in the current CRS
 * \param[in] maxy y-coordinate of the top right corner of the boundingBox in the current CRS
 * \return true if inside
 */
bool CRS::validateBBox ( double minx, double miny, double maxx, double maxy ) {
    return validateBBoxGeographic( boundingBoxToGeographic ( minx,miny,maxx,maxy ) );
}

/**
 * \~french
 * \brief Vérifie que la BoundingBox est dans le domaine de définition de la projection
 * \param[in] geographicBBox une emprise en WGS84
 * \return true si incluse
 * \~english
 * \brief Verify if the supplied BoundingBox is in the CRS definition area
 * \param[in] geographicBBox a BoundingBox in WGS84
 * \return true if inside
 */
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

/**
 * \~french
 * \brief Vérifie que la BoundingBox est dans le domaine de définition de la projection
 * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit en WGS84
 * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit en WGS84
 * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit en WGS84
 * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit en WGS84
 * \return true si incluse
 * \~english
 * \brief Verify if the supplied BoundingBox is in the CRS definition area
 * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in WGS84
 * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in WGS84
 * \param[in] maxx x-coordinate of the top right corner of the boundingBox in WGS84
 * \param[in] maxy y-coordinate of the top right corner of the boundingBox in WGS84
 * \return true if inside
 */
bool CRS::validateBBoxGeographic ( double minx, double miny, double maxx, double maxy ) {
    return validateBBoxGeographic( BoundingBox<double> ( minx,miny,maxx,maxy ) );
}

/**
 * \~french
 * \brief Calcule la BoundingBox incluse dans le domaine de définition du CRS courant
 * \param[in] geographicBBox une emprise définit dans le CRS courant
 * \return l'emprise recadrée
 * \~english
 * \brief Compute a BoundingBox included in the current CRS definition area
 * \param[in] geographicBBox a BoundingBox in the current CRS
 * \return the cropped BoundingBox
 */
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

/**
 * \~french
 * \brief Calcule la BoundingBox incluse dans le domaine de définition du CRS courant
 * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit dans le CRS courant
 * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit dans le CRS courant
 * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit dans le CRS courant
 * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit dans le CRS courant
 * \return l'emprise recadrée
 * \~english
 * \brief Compute a BoundingBox included in the current CRS definition area
 * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in the current CRS
 * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in the current CRS
 * \param[in] maxx x-coordinate of the top right corner of the boundingBox in the current CRS
 * \param[in] maxy y-coordinate of the top right corner of the boundingBox in the current CRS
 * \return the cropped BoundingBox
 */
BoundingBox< double > CRS::cropBBox ( double minx, double miny, double maxx, double maxy ) {
    return cropBBox( BoundingBox<double> ( minx,miny,maxx,maxy ) );
}