#ifndef PYRAMID_H
#define PYRAMID_H
#include <string>
#include <map>
#include "Level.h"
#include "TileMatrixSet.h"


class Pyramid {  
private:
	std::map<std::string, Level*> levels;
	const TileMatrixSet & tms;
	std::string best_level(double resolution_x, double resolution_y);

public:
	std::map<std::string, Level*> getLevels();
	Level * getFirstLevel();
	TileMatrixSet getTms();

	Tile* gettile(int x, int y, std::string tmId);
	Image* getbbox(BoundingBox<double> bbox, int width, int height, const char *dst_crs = 0);

	Pyramid(std::map<std::string, Level*> &levels, TileMatrixSet & tms) : levels(levels), tms(tms) {}
	~Pyramid() {}
};

#endif
