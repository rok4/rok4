#include "Request.h"
#include "Message.h"
#include "CRS.h"
#include "Pyramid.h"
#include <cstdlib>
#include <climits>
#include <vector>
#include <cstdio>
#include "config.h"

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

Request::Request(char* strquery, char* hostName, char* path, char* https) : hostName(hostName),path(path),service(""),request(""),scheme("") {
	LOGGER_DEBUG("QUERY="<<strquery);
	scheme = (https?"https://":"http://");
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

/**
 * @vriedf test de la présence de paramName dans la requete
 * @return true si présent
 */
bool Request::hasParam(std::string paramName){
	std::map<std::string, std::string>::iterator it = params.find(paramName);
	if(it == params.end()){
		return false;
	}
	return true;
}


/**
 * @vriedf récupération du parametre paramName dans la requete
 * @return la valeur du parametre si existant "" sinon
 */
std::string Request::getParam(std::string paramName){
	std::map<std::string, std::string>::iterator it = params.find(paramName);
	if(it == params.end()){
		return "";
	}
	return it->second;
}

/**
* @vrief Verification et recuperation des parametres d'une requete GetTile
* @return message d'erreur en cas d'erreur (NULL sinon)
*/

DataSource* Request::getTileParam(ServicesConf& servicesConf, std::map<std::string,TileMatrixSet*>& tmsList, std::map<std::string, Layer*>& layerList, Layer*& layer,  std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format, std::string &style)
{
	// VERSION
	std::string version=getParam("version");
	if (version=="")
		return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre VERSION absent.","wmts"));
	if (version.find(servicesConf.getServiceTypeVersion())==std::string::npos)
		return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (doit contenir "+servicesConf.getServiceTypeVersion()+")","wmts"));
	// LAYER
	std::string str_layer=getParam("layer");
	if(str_layer == "")
		return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre LAYER absent.","wmts"));
	std::map<std::string, Layer*>::iterator it = layerList.find(str_layer);
        if(it == layerList.end())
                return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Layer "+str_layer+" inconnu.","wmts"));
        layer = it->second;
	// TILEMATRIXSET
	std::string str_tms=getParam("tilematrixset");
	if(str_tms == "")
                return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEMATRIXSET absent.","wmts"));
	std::map<std::string, TileMatrixSet*>::iterator tms = tmsList.find(str_tms);
        if(tms == tmsList.end())
                return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"TileMatrixSet "+str_tms+" inconnu.","wmts"));
        layer = it->second;	
	// TILEMATRIX
	tileMatrix=getParam("tilematrix");
	if(tileMatrix == "")
		return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEMATRIX absent.","wmts"));

	std::map<std::string, TileMatrix>* pList=tms->second->getTmList();
	std::map<std::string, TileMatrix>::iterator tm = pList->find(tileMatrix);
	if (tm==pList->end())
		return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"TileMatrix "+tileMatrix+" inconnu pour le TileMatrixSet "+str_tms,"wmts"));

	// TILEROW
	std::string strTileRow=getParam("tilerow");
	if(strTileRow == "")
		return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEROW absent.","wmts"));
	if (sscanf(strTileRow.c_str(),"%d",&tileRow)!=1)
		return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILEROW est incorrecte.","wmts"));
	// TILECOL
	std::string strTileCol=getParam("tilecol");
	if(strTileCol == "")
		return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre TILECOL absent.","wmts"));
	if (sscanf(strTileCol.c_str(),"%d",&tileCol)!=1)
		return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILECOL est incorrecte.","wmts"));
	// FORMAT
	format=getParam("format");
        if(format == "")
                return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre FORMAT absent.","wmts"));
	// TODO : la norme exige la presence du parametre format. Elle ne precise pas que le format peut differer de la tuile, ce que ce service ne gere pas
	if (format != getMimeType(layer->getDataPyramid()->getFormat()))
		return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Le format "+format+" n'est pas gere pour la couche "+str_layer,"wmts"));
	//Style
	style=getParam("style");
	if(style == "")
                return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre STYLE absent.","wmts"));
	// TODO : Nom de style : inspire_common:DEFAULT en mode Inspire sinon default
	bool styleFound =false;
	if (layer->getStyles().size() != 0){
		for (unsigned int i=0; i < layer->getStyles().size(); i++){
			if (style == layer->getStyles()[i])
				styleFound=true;
		}
	}
	if (!(styleFound))
		return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Le style "+style+" n'est pas gere pour la couche "+str_layer,"wmts"));
	
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

/**
 * @brife Recuperation et verification des parametres d'une requete GetMap
 * @return message d'erreur en cas d'erreur (NULL sinon)
 */

DataStream* Request::getMapParam(ServicesConf& servicesConf, std::map<std::string, Layer*>& layerList, Layer*& layer, BoundingBox<double> &bbox, int &width, int &height, CRS& crs, std::string &format, std::string &styles){
        // VERSION
        std::string version=getParam("version");
        if (version=="")
                return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre VERSION absent.","wms"));
        if (version!="1.3.0")
                return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (1.3.0 disponible seulement))","wmts"));
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
	if (width<0)
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre WIDTH est negative.","wms"));
	if (width>servicesConf.getMaxWidth())
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre WIDTH est superieure a la valeur maximum autorisee par le service.","wms"));
	// HEIGHT
	std::string strHeight=getParam("height");
	if(strHeight == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre HEIGHT absent.","wms"));
	height=atoi(strHeight.c_str());
	if (height == 0 || height == INT_MAX || height == INT_MIN)
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT n'est pas une valeur entiere.","wms")) ;
	if (height<0)
                return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT est negative.","wms"));
        if (height>servicesConf.getMaxHeight())
                return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT est superieure a la valeur maximum autorisee par le service.","wms"));
	// CRS
	std::string str_crs=getParam("crs");
	if(str_crs == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre CRS absent.","wms"));
        // Existence du CRS dans la liste de CRS du layer
        crs.setRequestCode(str_crs);
        unsigned int k;
        for (k=0;k<layer->getWMSCRSList().size();k++)
                if (crs.cmpRequestCode(layer->getWMSCRSList().at(k)))
                        break;
        // FIXME : la methode vector::find plante (je ne comprends pas pourquoi)
        if (k==layer->getWMSCRSList().size())
                return new SERDataStream(new ServiceException("",WMS_INVALID_CRS,"CRS "+str_crs+" (equivalent PROJ4 "+crs.getProj4Code()+" ) inconnu pour le layer "+str_layer+".","wms"));

	// FORMAT
	format=getParam("format");
	if(format == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre FORMAT absent.","wms"));

	for (k=0;k<servicesConf.getFormatList()->size();k++)
	{
		if (servicesConf.getFormatList()->at(k)==format)
			break;
	}
	if (k==servicesConf.getFormatList()->size())
		return new SERDataStream(new ServiceException("",WMS_INVALID_FORMAT,"Format "+format+" non gere par le service.","wms"));

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
	// L'ordre des coordonnees (X,Y) de chque coin de la bbox doit suivre l'ordre des axes du CS associe au SRS
	// Implementation MapServer : l'ordre des axes est inverse pour les CRS de l'EPSG compris entre 4000 et 5000
	if (crs.getAuthority()=="EPSG" || crs.getAuthority()=="epsg") {
		int code=atoi(crs.getIdentifier().c_str());
		if (code>=4000 && code<5000){	
			bbox.xmin=bb[1];
        		bbox.ymin=bb[0];
        		bbox.xmax=bb[3];
        		bbox.ymax=bb[2];	
		}
	}

	// SCALE DENOMINATORS
	
	// Hypothese : les resolutions en X ET en Y doivent etre dans la plage de valeurs

	// Resolution en x et y en unites du CRS demande
	double resx=(bbox.xmax-bbox.xmin)/width, resy=(bbox.ymax-bbox.ymin)/height;

	// Resolution en x et y en m
	// Hypothese : les CRS en geographiques sont en degres
	if (crs.isLongLat()){
		resx*=111319;
		resy*=111319;
	}

	// Le serveur ne doit pas renvoyer d'exception
	// Cf. WMS 1.3.0 - 7.2.4.6.9

	double epsilon=0.0000001;	// Gestion de la precision de la division
	if (resx>0.)
		if (resx+epsilon<layer->getMinRes()||resy+epsilon<layer->getMinRes()){
			;//return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La resolution de l'image est inferieure a la resolution minimum.","wms"));
		}
	if (resy>0.)
		if (resx>layer->getMaxRes()+epsilon||resy>layer->getMaxRes()+epsilon)
                	;//return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La resolution de l'image est superieure a la resolution maximum.","wms"));

	// EXCEPTION
	std::string str_exception=getParam("exception");
	if (str_exception!=""&&str_exception!="XML")
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Format d'exception "+str_exception+" non pris en charge","wms"));
	
	if (!(hasParam("styles")))
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre STYLES absent.","wms"));
	styles=getParam("styles");
	if(styles == "") // Gestion du style par défaut
                styles=(servicesConf.isInspire()?DEFAULT_STYLE_INSPIRE:DEFAULT_STYLE);
	// TODO : récuperer les styles supporté par la couche
	bool styleFound =false;
	if (layer->getStyles().size() != 0){
		for (unsigned int i=0; i < layer->getStyles().size(); i++){
			if (styles == layer->getStyles()[i])
				styleFound=true;
		}
	}
	if (!(styleFound))
		return new SERDataStream(new ServiceException("",WMS_STYLE_NOT_DEFINED,"Le style "+styles+" n'est pas gere pour la couche "+str_layer,"wms"));
	return NULL;
}
