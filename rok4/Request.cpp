#include "Request.h"
#include "Message.h"
#include "CRS.h"
#include <cstdlib>
#include <climits>
#include <vector>
#include <cstdio> 

/* converts hex char (0-9, A-Z, a-z) to decimal.
 * returns 0xFF on invalid input.
 */
char hex2int(unsigned char hex) {
	hex = hex - '0';
	if (hex > 9) {
		hex = (hex + '0' - 1) | 0x20;
		hex = hex - 'a' + 11;
	}
	if (hex > 15)
		hex = 0xFF;

	return hex;
}

void Request::url_decode(char *src) {
	unsigned char high, low;
	char* dst = src;

	while ((*src) != '\0') {
		if (*src == '+') {
			*dst = ' ';
		} else if (*src == '%') {
			*dst = '%';

			high = hex2int(*(src + 1));
			if (high != 0xFF) {
				low = hex2int(*(src + 2));
				if (low != 0xFF) {
					high = (high << 4) | low;

					/* map control-characters out */
					if (high < 32 || high == 127) high = '_';

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

void toLowerCase(char* str){
        if(str) for(int i = 0; str[i]; i++) str[i] = tolower(str[i]);
}

Request::Request(char* strquery,char* hostName, char* path) : hostName(hostName),path(path),service(""),request("") {
	LOGGER_DEBUG("QUERY="<<strquery);
	url_decode(strquery);
	for(int pos = 0; strquery[pos];) {
		char* key = strquery + pos;
		for(;strquery[pos] && strquery[pos] != '=' && strquery[pos] != '&'; pos++); // on trouve le premier "=", "&" ou 0
		char* value = strquery + pos;
		for(;strquery[pos] && strquery[pos] != '&'; pos++); // on trouve le suivant "&" ou 0
		if(*value == '=') *value++ = 0;  // on met un 0 à la place du '=' entre key et value
		if(strquery[pos]) strquery[pos++] = 0; // on met un 0 à la fin du char* value

		toLowerCase(key);

		if (strcmp(key,"service")==0){
			toLowerCase(value);
			service = value;
		}else if (strcmp(key,"request")==0){
			toLowerCase(value);
			request = value;
		}else{
			params.insert(std::pair<std::string, std::string> (key, value));
		}
	}
}

Request::~Request() {}

std::string Request::getParam(std::string paramName){
	std::map<std::string, std::string>::iterator it = params.find(paramName);
	if(it == params.end()){
		return "";
	}
	return it->second;
}

/*
 * Récuperation des parametres d'une requete GetTile
 * @return message d'erreur en cas d'erreur (NULL sinon)
 */

DataSource* Request::getTileParam(std::string &layer, std::string  &tileMatrixSet, std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format)
{
	layer=getParam("layer");
	if(layer == "")
		return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre LAYER absent.","wmts"));
	tileMatrix=getParam("tilematrix");
	if(tileMatrix == "")
		return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEMATRIX absent.","wmts"));
	std::string strTileRow=getParam("tilerow");
	if(strTileRow == "")
		return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEROW absent.","wmts"));
	if (strTileRow=="0")
		tileRow = 0;
	else
		tileRow=atoi(strTileRow.c_str());
	if (tileRow == 0 || tileRow == INT_MAX || tileRow == INT_MIN)
		return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILEROW n'est pas une valeur entiere.","wmts"));
	std::string strTileCol=getParam("tilecol");
	if(strTileCol == "")
		return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre TILECOL absent.","wmts"));
	if (strTileCol=="0")
		tileCol = 0;
	else
		tileCol=atoi(strTileCol.c_str());
	if (tileCol == 0 || tileCol == INT_MAX || tileCol == INT_MIN)
		return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILECOL n'est pas une valeur entiere.","wmts"));

	format=getParam("format"); // FIXME: on ne controle pas la présence parce qu'on ne s'en sert pas pour le moment...
	tileMatrixSet=getParam("tilematrixset"); // FIXME: on ne controle pas la présence parce qu'on ne s'en sert pas pour le moment...

	return NULL;
}

void stringSplit(std::string str, std::string delim, std::vector<std::string> &results){
	int cutAt;
	while( (cutAt = str.find_first_of(delim)) != str.npos ){
		if(cutAt > 0){
			results.push_back(str.substr(0,cutAt));
		}
		str = str.substr(cutAt+1);
	}
	if(str.length() > 0){
		results.push_back(str);
	}
}

/*
 * Récuperation et vérification des parametres d'une requete GetMap
 * @return message d'erreur en cas d'erreur (NULL sinon)
 */

DataStream* Request::getMapParam(ServicesConf& servicesConf, std::map<std::string, Layer*>& layerList, Layer*& layer, BoundingBox<double> &bbox, int &width, int &height, CRS& crs, std::string &format){
	// LAYER
	std::string str_layer=getParam("layers");
	if(str_layer == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre LAYERS absent.","wms"));
        std::map<std::string, Layer*>::iterator it = layerList.find(str_layer);
        if(it == layerList.end())
                return new SERDataStream(new ServiceException("",WMS_LAYER_NOT_DEFINED,"Layer "+str_layer+" inconnu.","wms"));
        layer = it->second;
	// WIDTH
	std::string strWidth=getParam("width");
	if(strWidth == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre WIDTH absent.","wms"));
	width=atoi(strWidth.c_str());
	if (width == 0 || width == INT_MAX || width == INT_MIN)
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre WIDTH n'est pas une valeur entiere.","wms"));
	// HEIGHT
	std::string strHeight=getParam("height");
	if(strHeight == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre HEIGHT absent.","wms"));
	height=atoi(strHeight.c_str());
	if (height == 0 || height == INT_MAX || height == INT_MIN)
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT n'est pas une valeur entiere.","wms")) ;
	// CRS
	std::string str_crs=getParam("crs");
	if(str_crs == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre CRS absent.","wms"));
        // Existence du CRS dans la liste de CRS du layer
        crs.setRequestCode(str_crs);
        unsigned int k;
        for (k=0;k<layer->getWMSCRSList().size();k++)
                if (layer->getWMSCRSList().at(k)==crs.getProj4Code())
                        break;
        // FIXME : la methode vector::find plante (je ne comprends pas pourquoi)
        if (k==layer->getWMSCRSList().size())
                return new SERDataStream(new ServiceException("",WMS_INVALID_CRS,"CRS "+str_crs+" (equivalent PROJ4 "+crs.getProj4Code()+" ) inconnu pour le layer "+str_layer+".","wms"));
	
	// FORMAT
	format=getParam("format");
	if(format == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre FORMAT absent.","wms"));
	// BBOX
	std::string strBbox=getParam("bbox");
	if(strBbox == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre BBOX absent.","wms"));
	std::vector<std::string> coords;
	stringSplit(strBbox,",",coords);
	if (coords.size()!=4)
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms"));
	double bb[4];
	for(int i = 0; i < 4; i++) {
		if (sscanf(coords[i].c_str(),"%lf",&bb[i])!=1)
				return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms"));
	}
	if (bb[0]>=bb[2] || bb[1]>=bb[3])
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms"));
	bbox.xmin=bb[0];
	bbox.ymin=bb[1];
	bbox.xmax=bb[2];
	bbox.ymax=bb[3];
	// TODO : a refaire
	if (crs.getProj4Code()=="epsg:4326") {
		bbox.xmin=bb[1];
        	bbox.ymin=bb[0];
        	bbox.xmax=bb[3];
        	bbox.ymax=bb[2];
	}
	// EXCEPTION
	std::string str_exception=getParam("exception");
	if (str_exception!=""&&str_exception!="XML")
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Format d'exception "+str_exception+" non pris en charge","wms"));

	return NULL;
}
