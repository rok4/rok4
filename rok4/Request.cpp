#include "Request.h"
#include "Error.h"
#include <cstdlib>
#include <climits>
#include <vector>

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

Request::Request(char* strquery,char* serverName) : server(serverName),service(""),request("") {
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

Request::~Request() {
}

std::string Request::getParam(std::string paramName){
	std::map<std::string, std::string>::iterator it = params.find(paramName);
	if(it == params.end()){
		return "";
	}
	return it->second;
}

HttpResponse* Request::getTileParam(std::string &layer, std::string  &tileMatrixSet, std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format){
	layer=getParam("layer");
    if(layer == "")    {LOGGER_DEBUG("Missing parameter: layer"); return new Error("Missing parameter: layer");}
    tileMatrix=getParam("tilematrix");
    if(tileMatrix == ""){LOGGER_DEBUG("Missing parameter: tilematrix"); return new Error("Missing parameter: tilematrix");}
    std::string strTileRow=getParam("tilerow");
    if(strTileRow == "")    {LOGGER_DEBUG("Missing parameter: tilerow"); return new Error("Missing parameter: tilerow");}
    if (strTileRow=="0"){
    	tileRow = 0;
    }else{
        tileRow=atoi(strTileRow.c_str());
        if (tileRow == 0 || tileRow == INT_MAX || tileRow == INT_MIN){
        	LOGGER_DEBUG("tileRow is not an integer");
        	return new Error("tileRow is not an integer");
        }
    }
	std::string strTileCol=getParam("tilecol");
    if(strTileCol == "")    {LOGGER_DEBUG("Missing parameter: tilecol"); return new Error("Missing parameter: tilecol");}
    if (strTileCol=="0"){
    	tileCol = 0;
    }else{
        tileCol=atoi(strTileCol.c_str());
        if (tileCol == 0 || tileCol == INT_MAX || tileCol == INT_MIN){
        	LOGGER_DEBUG("tilecol is not an integer");
        	return new Error("tilecol is not an integer");
        }
    }

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

HttpResponse* Request::getMapParam(std::string &layers, BoundingBox<double> &bbox, int &width, int &height, std::string &crs, std::string &format){
	layers=getParam("layers");
    if(layers == "")    {LOGGER_DEBUG("Missing parameter: layers"); return new Error("Missing parameter: layers");}
    std::string strWidth=getParam("width");
    if(strWidth == ""){LOGGER_DEBUG("Missing parameter: width"); return new Error("Missing parameter: width");}
    width=atoi(strWidth.c_str());
    if (width == 0 || width == INT_MAX || width == INT_MIN){
      	LOGGER_DEBUG("width is not an integer");
       	return new Error("width is not an integer");
    }
    std::string strHeight=getParam("height");
    if(strHeight == ""){LOGGER_DEBUG("Missing parameter: height"); return new Error("Missing parameter: height");}
    height=atoi(strHeight.c_str());
    if (height == 0 || height == INT_MAX || height == INT_MIN){
      	LOGGER_DEBUG("height is not an integer");
       	return new Error("height is not an integer");
    }
	crs=getParam("crs");
	/* FIXME le CRS est sensé etre obligatoire, mais pour des raisons de
	 * "compatibilité" avec le prototype on est un peu laxiste pour le moment
	if(crs == "")    {LOGGER_DEBUG("Missing parameter: crs"); return new Error("Missing parameter: crs");}
	*/
	format=getParam("format");
    if(format == "")    {LOGGER_DEBUG("Missing parameter: format"); return new Error("Missing parameter: format");}
    std::string strBbox=getParam("bbox");
    if(strBbox == "")    {LOGGER_DEBUG("Missing parameter: bbox"); return new Error("Missing parameter: bbox");}
    std::vector<std::string> coords;
    stringSplit(strBbox,",",coords);
    if (coords.size()!=4){
    	LOGGER_DEBUG("bbox parameter is incorrect"); return new Error("bbox parameter is incorrect");
    }
    double bb[4];
    for(int i = 0; i < 4; i++) {
    	bb[i] = atof(coords[i].c_str());
    	if (bb[i] == 0.0 && coords[i] != "0.0"){
    		LOGGER_DEBUG("bbox parameter is incorrect"); return new Error("bbox parameter is incorrect");
    	}
    }
    bbox.xmin=bb[0];
    bbox.ymin=bb[1];
    bbox.xmax=bb[2];
    bbox.ymax=bb[3];

	return NULL;
}
