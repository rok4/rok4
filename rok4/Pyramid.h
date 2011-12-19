#ifndef PYRAMID_H
#define PYRAMID_H
#include <string>
#include <map>
#include "Level.h"
#include "TileMatrixSet.h"
#include "CRS.h"
#include "format.h"

//std::string getMimeType(std::string format);

/**
* @class Pyramid
* @brief Implementation des pyramides
* Une pyramide est associee a un layer et comporte plusieurs niveaux
*/

class Pyramid {  
private:
	std::map<std::string, Level*> levels;
	const TileMatrixSet tms;
	std::map<std::string, DataSource*> noDataSources;
	std::string best_level(double resolution_x, double resolution_y);
	const eformat_data format; //format d'image des tuiles
	const int     channels;
public:
	Level* getFirstLevel();
	TileMatrixSet getTms();
	std::map<std::string, Level*>& getLevels() {return levels;}
	eformat_data getFormat() {return format;}
	int getChannels(){return channels;}

	DataSource* getTile(int x, int y, std::string tmId);	
	Image* getbbox(BoundingBox<double> bbox, int width, int height, CRS dst_crs, int& error);

	Pyramid(std::map<std::string, Level*> &levels, TileMatrixSet tms, eformat_data format, int channels);
	~Pyramid();
};

#endif
