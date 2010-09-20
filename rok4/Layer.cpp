#include "Layer.h"
#include "Logger.h"

Tile* Layer::gettile(int x, int y, std::string tmId) {
	//TODO: Ici il faudrait choisir la pyramide à utiliser en fonction
	//      du CRS de la requete ou du format d'image demandé.
	//      Mais là on prend juste la première du layer.
	return pyramids[0]->gettile(x, y, tmId);
}

Image* Layer::getbbox(BoundingBox<double> bbox, int width, int height, const char *dst_crs) {
	//TODO: Ici il faudrait choisir la pyramide à utiliser en fonction
	//      du CRS de la requete ou du format d'image demandé.
	//      Mais là on prend juste la première du layer.
	return pyramids[0]->getbbox(bbox, width, height, dst_crs);
}

std::string Layer::getId(){return id;}
