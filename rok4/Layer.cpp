#include "Layer.h"
#include "Pyramid.h"
#include "Logger.h"

DataSource* Layer::gettile(int x, int y, std::string tmId) {
	return dataPyramid->getTile(x, y, tmId);
}

Image* Layer::getbbox(BoundingBox<double> bbox, int width, int height, CRS dst_crs, int& error) {
	error=0;
	return dataPyramid->getbbox(bbox, width, height, dst_crs, error);
}

std::string Layer::getId(){return id;}

Layer::~Layer()
{
		delete dataPyramid;
}
