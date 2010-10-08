#ifndef LEVEL_H
#define LEVEL_H

#include "Image.h"
#include "Tile.h"
#include "BoundingBox.h"
#include "TileMatrix.h"
#include "Data.h"
#include "FileDataSource.h"

/**
 */

class Level {
private:

	std::string   baseDir;
	int           pathDepth;
	TileMatrix    tm;         // FIXME j'ai des problème de compil que je ne comprends pas si je mets un const ?!
	const std::string format; //format d'image des block de tuiles du cache.
	const int     channels;
	const int32_t maxTileRow;
	const int32_t minTileRow;
	const int32_t maxTileCol;
	const int32_t minTileCol;
	uint32_t      tilesPerWidth;   //nombre de tuiles par dalle dans le sens de la largeur
	uint32_t      tilesPerHeight;  //nombre de tuiles par dalle dans le sens de la hauteur
	const std::string noDataFile;

	std::string getfilepath(int tilex, int tiley);
	FileDataSource* noDataSource;

protected:
	/**
	 * Renvoie une image de taille width, height
	 *
	 * le coin haut gauche de cette image est le pixel offsetx, offsety de la tuile tilex, tilex.
	 * Toutes les coordonnées sont entière depuis le coin haut gauche.
	 */
	Image* getwindow(BoundingBox<int64_t> src_bbox);


public:
	TileMatrix const getTm();
	std::string getFormat();
	uint32_t    getMaxTileRow();
	uint32_t    getMinTileRow();
	uint32_t    getMaxTileCol();
	uint32_t    getMinTileCol();
	double      getRes();
	std::string getId();
	int	      getTileCoding();
	std::string getType();


	Image* getbbox(BoundingBox<double> bbox, int width, int height);

	Image* getbbox(BoundingBox<double> bbox, int width, int height, const char* dst_crs);
	/**
	 * Renvoie la tuile x, y numéroté depuis l'origine.
	 * Le coin haut gauche de la tuile (0,0) est (Xorigin, Yorigin)
	 * Les indices de tuiles augmentes vers la droite et vers le bas.
	 * Des indices de tuiles négatifs sont interdits
	 *
	 * La tuile contenant la coordonnées (X, Y) dans le srs d'origine a pour indice :
	 * x = floor((X - X0) / (tile_width * resolution_x))
	 * y = floor((Y - Y0) / (tile_height * resolution_y))
	 */
	Tile* gettile(int x, int y);


	/** D */
	Level(TileMatrix tm, int channels, std::string baseDir,
			int tilesPerWidth, int tilesPerHeight,
			uint32_t maxTileRow, uint32_t minTileRow, uint32_t maxTileCol, uint32_t minTileCol,
			int pathDepth, std::string format);

	/*
	 * Destructeur
	 */
	~Level()
	{
		delete noDataSource;
	}

};

#endif




