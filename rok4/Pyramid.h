#ifndef PYRAMID_H
#define PYRAMID_H
#include <string>
#include <map>
#include "Level.h"
#include "TileMatrixSet.h"
#include "CRS.h"


class Pyramid {  
private:
	std::map<std::string, Level*> levels;
	const TileMatrixSet tms;
	std::string best_level(double resolution_x, double resolution_y);

public:

	Level * getFirstLevel();
	TileMatrixSet getTms();

	DataSource* getTile(int x, int y, std::string tmId);	
	Image* getbbox(BoundingBox<double> bbox, int width, int height, CRS dst_crs);

	Pyramid(std::map<std::string, Level*> &levels, TileMatrixSet tms) : levels(levels), tms(tms) {}
	~Pyramid() {}
};

#endif
