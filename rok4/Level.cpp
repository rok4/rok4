
#include "Level.h"
#include "FileDataSource.h"

#include "CompoundImage.h"
#include "ResampledImage.h"
#include "ReprojectedImage.h"

#include <cmath>
#include "Logger.h"
#include "Kernel.h"

#define EPS 1./256. // FIXME: La valeur 256 est liée au nombre de niveau de valeur d'un canal
//        Il faudra la changer lorsqu'on aura des images non 8bits.


/** Constructeur */
Level::Level(TileMatrix tm, int channels, std::string baseDir, int tilesPerWidth, int tilesPerHeight, uint32_t maxTileRow, uint32_t minTileRow, uint32_t maxTileCol, uint32_t minTileCol, int pathDepth, std::string format) :
tm(tm), channels(channels), baseDir(baseDir), tilesPerWidth(tilesPerWidth), tilesPerHeight(tilesPerHeight), maxTileRow(maxTileRow), minTileRow(minTileRow), maxTileCol(maxTileCol), minTileCol(minTileCol), pathDepth(pathDepth), format(format)
{
	if (getType()=="image/jpeg")
		noDataSource = new FileDataSource("../config/nodata/nodata_tiled_jpeg.tif",2048,2052,"image/jpeg");
	else if(getType()=="image/tiff")
		noDataSource = new FileDataSource("../config/nodata/nodata_tiled_raw.tif",2048,2052,/*getType()*/"image/tiff");
	else if(getType()=="image/png")
                noDataSource = new FileDataSource("../config/nodata/nodata_tiled_png.tif",2048,2052,/*getType()*/"image/png");
	else
		LOGGER_ERROR("Pas de nodatasource disponible");
}

TileMatrix const Level::getTm(){return tm;}
std::string Level::getFormat(){return format;}
uint32_t Level::getMaxTileRow(){return maxTileRow;}
uint32_t Level::getMinTileRow(){return minTileRow;}
uint32_t Level::getMaxTileCol(){return maxTileCol;}
uint32_t Level::getMinTileCol(){return minTileCol;}
double Level::getRes(){return tm.getRes();}
std::string Level::getId(){return tm.getId();}


/*
 * @return le type MIME du format
 */

std::string Level::getType() {
	if (format.compare("TIFF_INT8")==0)
		return "image/tiff";
	else if (format.compare("TIFF_JPG_INT8")==0)
		return "image/jpeg";
	else if (format.compare("TIFF_PNG_INT8")==0)
		return "image/png";
	return "text/plain";
}

/*
 * @return le type de codage de la tuile en fonction du format
 * @ return -1 en cas d'erreur
 */

int Level::getTileCoding() {
	if (format.compare("TIFF_INT8")==0)
		return RAW_UINT8;
	else if (format.compare("TIFF_JPG_INT8")==0)
		return JPEG_UINT8;
	else if (format.compare("TIFF_PNG_INT8")==0)
		return PNG_UINT8;
	LOGGER_ERROR("Type d'encodage inconnu : "<<format); 
	return -1;
}

/*
 * A REFAIRE
 */
Image* Level::getbbox(BoundingBox<double> bbox, int width, int height, CRS src_crs, CRS dst_crs) {
	Grid* grid = new Grid(width, height, bbox);
	grid->bbox.print();

	grid->reproject(dst_crs.getProj4Code(), src_crs.getProj4Code());

	grid->bbox.print();
	BoundingBox<int64_t> bbox_int(floor((grid->bbox.xmin - tm.getX0())/tm.getRes() - 50),
			floor((tm.getY0() - grid->bbox.ymax)/tm.getRes() - 50),
			ceil ((grid->bbox.xmax - tm.getX0())/tm.getRes() + 50),
			ceil ((tm.getY0() - grid->bbox.ymin)/tm.getRes() + 50));
	// TODO : remplacer 50 par un buffer calculé en fonction du noyau d'interpolation

	bbox_int.print();
	Image* image = getwindow(bbox_int);
	image->setbbox( BoundingBox<double>(tm.getX0() + tm.getRes() * bbox_int.xmin, tm.getY0() - tm.getRes() * bbox_int.ymax, tm.getX0() + tm.getRes() * bbox_int.xmax, tm.getY0() - tm.getRes() * bbox_int.ymin));
	LOGGER_DEBUG("Reprojection");
	return new ReprojectedImage(image, bbox, grid);
}


Image* Level::getbbox(BoundingBox<double> bbox, int width, int height) {
	// On convertit les coordonnées en nombre de pixels depuis l'origine X0,Y0
	bbox.xmin = (bbox.xmin - tm.getX0())/tm.getRes();
	bbox.xmax = (bbox.xmax - tm.getX0())/tm.getRes();
	double tmp = bbox.ymin;
	bbox.ymin = (tm.getY0() - bbox.ymax)/tm.getRes();
	bbox.ymax = (tm.getY0() - tmp)/tm.getRes();

	//A VERIFIER !!!!
	BoundingBox<int64_t> bbox_int(floor(bbox.xmin + EPS),
			floor(bbox.ymin + EPS),
			ceil(bbox.xmax - EPS),
			ceil(bbox.ymax - EPS));

	if(bbox_int.xmax - bbox_int.xmin == width && bbox_int.ymax - bbox_int.ymin == height &&
			bbox.xmin - bbox_int.xmin < EPS && bbox.ymin - bbox_int.ymin < EPS &&
			bbox_int.xmax - bbox.xmax < EPS && bbox_int.ymax - bbox.ymax < EPS) {
		return getwindow(bbox_int);
	}

	double ratio_x = (bbox.xmax - bbox.xmin) / width;
	double ratio_y = (bbox.ymax - bbox.ymin) / height;

	const Kernel& kk = Kernel::getInstance(Kernel::LANCZOS_3);

	bbox_int.xmin = floor(bbox.xmin - kk.size(ratio_x));
	bbox_int.xmax = ceil (bbox.xmax + kk.size(ratio_x));
	bbox_int.ymin = floor(bbox.ymin - kk.size(ratio_y));
	bbox_int.ymax = ceil (bbox.ymax + kk.size(ratio_y));

	return new ResampledImage(getwindow(bbox_int), width, height, bbox.xmin - bbox_int.xmin, bbox.ymin - bbox_int.ymin, ratio_x, ratio_y);
}


Image* Level::getwindow(BoundingBox<int64_t> bbox) {

	int tile_xmin = bbox.xmin / tm.getTileW();
	int tile_xmax = (bbox.xmax -1)/ tm.getTileW();
	int nbx = tile_xmax - tile_xmin + 1;
	LOGGER_DEBUG(" getwindow bbox.xmin:" <<bbox.xmin << "tile_xmin:" << tile_xmin << " tile_xmax:" << tile_xmax << " nb_x:" << nbx << " tileW:" << tm.getTileW() << " " );

	int tile_ymin = bbox.ymin / tm.getTileH();
	int tile_ymax = (bbox.ymax-1) / tm.getTileH();
	int nby = tile_ymax - tile_ymin + 1;

	int left[nbx];   memset(left,   0, nbx*sizeof(int)); left[0] = bbox.xmin % tm.getTileW();
	int top[nby];    memset(top,    0, nby*sizeof(int)); top[0]  = bbox.ymin % tm.getTileH();
	int right[nbx];  memset(right,  0, nbx*sizeof(int)); right[nbx - 1] = tm.getTileW() - ((bbox.xmax -1) % tm.getTileW()) - 1;
	int bottom[nby]; memset(bottom, 0, nby*sizeof(int)); bottom[nby- 1] = tm.getTileH() - ((bbox.ymax -1) % tm.getTileH()) - 1;

	int tileCoding=getTileCoding();

	std::vector<std::vector<Image*> > T(nby, std::vector<Image*>(nbx));
	for(int y = 0; y < nby; y++)
		for(int x = 0; x < nbx; x++) {
//			LOGGER_DEBUG(" getwindow " << x << " " << y << " " << nbx << " " << nby << " " << left[x] << " " << right[x] << " " << top[y] << " " << bottom[y] );
			Tile* tile = gettile(tile_xmin + x, tile_ymin + y);
			T[y][x] = new Tile(tm.getTileW(), tm.getTileH(), channels, tile->getDataSource(), tile->getNoDataSource(), left[x], top[y], right[x], bottom[y],tileCoding);
		}

	if(nbx == 1 && nby == 1) return T[0][0];
	else return new CompoundImage(T);
}

/*
 * Tableau statique des caractères Base36 (pour système de fichieri non cas-sensitive)
 */
// static const char* Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
static const char* Base36 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*
 * Recuperation du nom de fichier de la dalle du cache en fonction de son indice
 */
std::string Level::getfilepath(int tilex, int tiley)
{
	// Cas normalement filtré en amont (exception WMS/WMTS)
	if (tilex < 0 || tiley < 0)
	{
		LOGGER_ERROR("Indice de tuile négatif");
		return "";
	}

	int x = tilex / tilesPerWidth;
	int y = tiley / tilesPerHeight;

	char path[32];
	path[sizeof(path) - 5] = '.';
	path[sizeof(path) - 4] = 't';
	path[sizeof(path) - 3] = 'i';
	path[sizeof(path) - 2] = 'f';
	path[sizeof(path) - 1] = 0;
	int pos = sizeof(path) - 6;

	for(int d = 0; d < pathDepth; d++) {;
	path[pos--] = Base36[y % 36];
	path[pos--] = Base36[x % 36];
	path[pos--] = '/';
	x = x / 36;
	y = y / 36;
	}
	do {
		path[pos--] = Base36[y % 36];
		path[pos--] = Base36[x % 36];
		x = x / 36;
		y = y / 36;
	} while(x || y);
	path[pos] = '/';
LOGGER_DEBUG(path<<" "<<y<<" "<<tiley<<" "<<tilesPerHeight);
	return baseDir + (path + pos);
}

/*
 * @ return la tuile d'indice (x,y) du niveau
 */

Tile* Level::gettile(int x, int y)
{
	int tileCoding=getTileCoding();
	if (tileCoding<0)
		return 0;

	// Index de la tuile (cf. ordre de rangement des tuiles)
	int n=(y%tilesPerHeight)*tilesPerWidth + (x%tilesPerWidth);
	// Les index sont stockés à partir de l'octet 2048
	uint32_t posoff=2048+4*n, possize=2048+4*n +tilesPerWidth*tilesPerHeight*4;
	LOGGER_DEBUG(getfilepath(x, y));
	FileDataSource* dataSource = new FileDataSource(getfilepath(x, y).c_str(),posoff,possize,getType());

	return new Tile(tm.getTileW(),tm.getTileH(),channels,dataSource,noDataSource, 0,0,tm.getTileW(),tm.getTileH(),getTileCoding());
}

