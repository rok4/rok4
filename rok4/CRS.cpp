#include "CRS.h"
#include "Logger.h"
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
        projPJ pj=pj_init_plus(("+init=" + crs +" +wktext" ).c_str());
        if (pj) return true;
	else return false;
}

/*
* Contructeur
* @brief Determine a partir du code du CRS passe dans la requete le code Porj4 correspondant
*/

CRS::CRS(std::string crs_code){
	requestCode=crs_code;
	if (isCrsProj4Compatible(requestCode))
		proj4Code=requestCode;
	else if (isCrsProj4Compatible(toLowerCase(requestCode)))
		proj4Code=toLowerCase(requestCode);
	else if (isCrsProj4Compatible(toLowerCase(requestCode)))
                proj4Code=toLowerCase(requestCode);
	// TODO : rajouter des tests en dur (ou charges depuis la conf), correspondance EPSG <-> IGNF
	// Commencer par ces tests (Ex : tout exprimer par defaut en EPSG)
	else
		proj4Code=NO_PROJ4_CODE;


LOGGER_DEBUG(requestCode);

}

bool CRS::isProj4Compatible(){
	return proj4Code!=NO_PROJ4_CODE;
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

/*
* Test d'egelite de 2 CRS
* @return true s'ils ont le meme code Proj4, false sinon
*/

bool CRS::operator==(const CRS crs) const {
	return (proj4Code==crs.proj4Code);
}


