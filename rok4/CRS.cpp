#include "CRS.h"
#include "Logger.h"
#include "Grid.h"
#include <proj_api.h>

#define NO_PROJ4_CODE "noProj4Code"

std::string toLowerCase(std::string str){
        std::string lc_str=str;
        for(int i = 0; str[i]; i++) lc_str[i] = tolower(str[i]);
        return lc_str;
}
std::string toUpperCase(std::string str){
        std::string uc_str=str;
        for(int i = 0; str[i]; i++) uc_str[i] = toupper(str[i]);
        return uc_str;
}
bool isCrsProj4Compatible(std::string crs){
	projCtx ctx = pj_ctx_alloc();
	projPJ pj=pj_init_plus_ctx(ctx,("+init=" + crs +" +wktext").c_str());
        if (!pj){
        	int err = pj_ctx_get_errno(ctx);
        	char *msg = pj_strerrno(err);
		// LOGGER_DEBUG("erreur d initialisation " << crs << " " << msg);
		return false;
	}
	bool isCompatible;
        if (pj) isCompatible=true;
	else isCompatible=false;
	pj_free(pj);
        pj_ctx_free(ctx);
        return isCompatible;
}

bool isCrsLongLat(std::string crs){
	projCtx ctx = pj_ctx_alloc();
	projPJ pj=pj_init_plus_ctx(ctx,("+init=" + crs +" +wktext").c_str());
	if (!pj){
                int err = pj_ctx_get_errno(ctx);
                char *msg = pj_strerrno(err);
                // LOGGER_DEBUG("erreur d initialisation " << crs << " " << msg);
                return false;
        }
	bool isLongLat=pj_is_latlong(pj);
	pj_free(pj);
	pj_ctx_free(ctx);
	return isLongLat;
}

/*
* Contructeur
*/

CRS::CRS(std::string crs_code){
	requestCode=crs_code;
	buildProj4Code();
}

/**
* Contructeur de copie
*/

CRS::CRS(const CRS& crs){
	requestCode=crs.requestCode;
	proj4Code=crs.proj4Code;
}

/*
* Determine a partir du code du CRS passe dans la requete le code Proj4 correspondant
*/

void CRS::buildProj4Code(){
        if (isCrsProj4Compatible(requestCode))
                proj4Code=requestCode;
        else if (isCrsProj4Compatible(toLowerCase(requestCode)))
                proj4Code=toLowerCase(requestCode);
        else if (isCrsProj4Compatible(toUpperCase(requestCode)))
                proj4Code=toUpperCase(requestCode);
        // TODO : rajouter des tests en dur (ou charges depuis la conf), correspondance EPSG <-> IGNF
        // Commencer par ces tests (Ex : tout exprimer par defaut en EPSG)
	// ISO 19128 6.7.3.2
	else if (requestCode=="CRS:84")
		proj4Code="EPSG:4326";
        else
                proj4Code=NO_PROJ4_CODE;
}

bool CRS::isProj4Compatible(){
	return proj4Code!=NO_PROJ4_CODE;
}

bool CRS::isLongLat(){
        return isCrsLongLat(proj4Code);
}

long double CRS::getMetersPerUnit(){
	// Hypothese : un CRS en long/lat est en degres
	// R=6378137m
	if (isLongLat())
		return 111319.49079327357264771338267056;
	else
		return 1.0;
}

void CRS::setRequestCode(std::string crs){
	requestCode=crs;
	buildProj4Code();
}

bool CRS::cmpRequestCode(std::string crs){
	return toLowerCase(requestCode)==toLowerCase(crs);
}

std::string CRS::getAuthority(){
	size_t pos=requestCode.find(':');
        if (pos<1 || pos >=requestCode.length()){
                LOGGER_ERROR("Erreur sur le CRS "<<requestCode<< " : absence de separateur");
		pos=requestCode.length();
        }
        return(requestCode.substr(0,pos));
}

std::string CRS::getIdentifier(){
	size_t pos=requestCode.find(':');
        if (pos<1 || pos >=requestCode.length()){
                LOGGER_ERROR("Erreur sur le CRS "<<requestCode<< " : absence de separateur");
                pos=-1;
        }
        return(requestCode.substr(pos+1));
}

/**
* Test d'egalite de 2 CRS
* @return true s'ils ont le meme code Proj4, false sinon
*/

bool CRS::operator==(const CRS crs) const {
	return (proj4Code==crs.proj4Code);
}

/**
 * Calcule la BoundingBox dans le CRS courant à partir de la BoundingBox Géographique
 * @return La BoundingBox en projection
 */
BoundingBox<double> CRS::boundingBoxFromGeographic(BoundingBox< double > geographicBBox)
{
	Grid* grid = new Grid(256,256,geographicBBox);
	grid->reproject("epsg:4326",proj4Code);
	BoundingBox<double> bbox = grid->bbox;
	delete grid;
	grid=0;
	return bbox;
}


/**
 * Calcule la BoundingBox dans le CRS courant à partir de la BoundingBox Géographique
 * @return La BoundingBox en projection
 */
BoundingBox< double > CRS::boundingBoxFromGeographic(double minx, double miny, double maxx, double maxy)
{
	return boundingBoxFromGeographic(BoundingBox<double>(minx,miny,maxx,maxy));
}
