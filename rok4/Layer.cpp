#include "Layer.h"
#include "Logger.h"

DataSource* Layer::gettile(int x, int y, std::string tmId) {
	//TODO: Ici il faudrait choisir la pyramide à utiliser en fonction
	//      du CRS de la requete ou du format d'image demandé.
	//      Mais là on prend juste la première du layer.
	return pyramids[0]->getTile(x, y, tmId);
}

Image* Layer::getbbox(BoundingBox<double> bbox, int width, int height, CRS dst_crs, int& error) {
	//TODO: Ici il faudrait choisir la pyramide à utiliser en fonction
	//      du CRS de la requete ou du format d'image demandé.
	//      Mais là on prend juste la première du layer.
	error=0;
	return pyramids[0]->getbbox(bbox, width, height, dst_crs, error);
}

std::string Layer::getId(){return id;}


/**
 * Cette fonction retourne les formats mime des images des pyramides associées à ce layer.
 * Il s'agit donc des formats que l'on peut demander en WMTS.
 */
std::vector<std::string> Layer::getMimeFormats(){
	std::vector<std::string> formats;
	for (unsigned int i=0; i < pyramids.size();i++){
		std::string format=pyramids[i]->getFirstLevel()->getFormat();
		if (format.find("JPG")!=std::string::npos){
			formats.push_back("image/jpeg");
		}else if (format.find("PNG")!=std::string::npos){
			formats.push_back("image/png");
		}else if (format.find("GIF")!=std::string::npos){
			formats.push_back("image/gif");
		}else if (format.find("TIFF_FLOAT32")!=std::string::npos){
			formats.push_back("image/x-bil");
			formats.push_back("image/x-bil;bits=32");
		}
		{
			formats.push_back("image/tiff");
		}
	}
	return formats;
}

Layer::~Layer()
{
	for (int i=0;i<pyramids.size();i++)
		delete pyramids[i];
}
