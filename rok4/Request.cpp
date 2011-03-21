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
 * Verfication et recuperation des parametres d'une requete GetTile
 * @return message d'erreur en cas d'erreur (NULL sinon)
 */

DataSource* Request::getTileParam(ServicesConf& servicesConf, std::map<std::string,TileMatrixSet*>& tmsList, std::map<std::string, Layer*>& layerList, Layer*& layer,  std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format)
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

	std::map<std::string, TileMatrix>* pList=&tms->second->getTmList();
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
                return new SERDataSource(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre FORMAT absent.","wms"));
	// TODO : la norme exige la presence du parametre format. Elle ne precise pas que le format peut differer de la tuile, ce que ce service ne gere pas
	unsigned int k;
	for (k=0;k<layer->getMimeFormats().size();k++)
		if (layer->getMimeFormats().at(k)==format)
			break;
	if (k==layer->getMimeFormats().size())
		return new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Le format "+format+" n'est pas gere pour la couche "+str_layer,"wmts"));
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
                if (layer->getWMSCRSList().at(k)==crs.getProj4Code())
                        break;
        // FIXME : la methode vector::find plante (je ne comprends pas pourquoi)
        if (k==layer->getWMSCRSList().size())
                return new SERDataStream(new ServiceException("",WMS_INVALID_CRS,"CRS "+str_crs+" (equivalent PROJ4 "+crs.getProj4Code()+" ) inconnu pour le layer "+str_layer+".","wms"));
	
	// FORMAT
	format=getParam("format");
	if(format == "")
		return new SERDataStream(new ServiceException("",OWS_MISSING_PARAMETER_VALUE,"Parametre FORMAT absent.","wms"));
	/*
	FIXME : plantage inexplique
	for (k=0;k<servicesConf.getFormatList().size();k++)
		if (servicesConf.getFormatList().at(k)==format)
			break;
	if (k==servicesConf.getFormatList().size())
		return new SERDataStream(new ServiceException("",WMS_INVALID_FORMAT,"Format "+format+" non gere par le service.","wms"));*/
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
	// Gestion des resolutions minimum et maximum
	// Hypothese : les resolutions en X ET en Y doivent etre dans la plage de valeurs
	// TODO : cas ou resx, resy et layer->getMinRes() ne sont pas dans les memes unites
	double resx=(bbox.xmax-bbox.xmin)/width, resy=(bbox.ymax-bbox.ymin)/height;
	if (resx<layer->getMinRes()||resy<layer->getMinRes()){
		LOGGER_DEBUG("resx="<<" resy="<<resy<<" minres="<<layer->getMinRes());
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La resolution de l'image est inferieure a la resolution minimum.","wms"));
	}
	if (resx>layer->getMaxRes()||resy>layer->getMaxRes())
                return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"La resolution de l'image est superieure a la resolution maximum.","wms"));
	// EXCEPTION
	std::string str_exception=getParam("exception");
	if (str_exception!=""&&str_exception!="XML")
		return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Format d'exception "+str_exception+" non pris en charge","wms"));
	return NULL;
}
