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
        projPJ pj=pj_init_plus(("+init=" + crs +" +wktext").c_str());
        int *err = pj_get_errno_ref();
        char *msg = pj_strerrno(*err);

        if (pj) return true;
        else return false;
}

/*
* Contructeur
*/

CRS::CRS(std::string crs_code){
	requestCode=crs_code;
	buildProj4Code();
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
        else
                proj4Code=NO_PROJ4_CODE;
}

bool CRS::isProj4Compatible(){
	return proj4Code!=NO_PROJ4_CODE;
}

void CRS::setRequestCode(std::string crs){
	requestCode=crs;
	buildProj4Code();
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
* Test d'egalite de 2 CRS
* @return true s'ils ont le meme code Proj4, false sinon
*/

bool CRS::operator==(const CRS crs) const {
	return (proj4Code==crs.proj4Code);
}
