#include "Level.h"
#include "FileDataSource.h"
#include "CompoundImage.h"
#include "ResampledImage.h"
#include "ReprojectedImage.h"
#include "RawImage.h"
#include "Decoder.h"
#include "TiffEncoder.h"
#include <cmath>
#include "Logger.h"
#include "Kernel.h"
#include <vector>
#include "Pyramid.h"

#define EPS 1./256. // FIXME: La valeur 256 est liée au nombre de niveau de valeur d'un canal
//        Il faudra la changer lorsqu'on aura des images non 8bits.

/*
 * A REFAIRE
 */
Image* Level::getbbox(BoundingBox<double> bbox, int width, int height, CRS src_crs, CRS dst_crs) {
	Grid* grid = new Grid(width, height, bbox);

	grid->reproject(dst_crs.getProj4Code(), src_crs.getProj4Code());

	// Calcul de la taille du noyau
	// TODO : type de noyau a parametrer
	const Kernel& kk = Kernel::getInstance(Kernel::LANCZOS_2);
	double ratio_x = (grid->bbox.xmax - grid->bbox.xmin) / (tm.getRes()*double(width));
        double ratio_y = (grid->bbox.ymax - grid->bbox.ymin) / (tm.getRes()*double(height));
	double bufx=kk.size(ratio_x);
	double bufy=kk.size(ratio_y);
	bufx<50?bufx=50:0;bufy<50?bufy=50:0; // Pour etre sur de ne pas regresser

	BoundingBox<int64_t> bbox_int(floor((grid->bbox.xmin - tm.getX0())/tm.getRes() - bufx),
			floor((tm.getY0() - grid->bbox.ymax)/tm.getRes() - bufy),
			ceil ((grid->bbox.xmax - tm.getX0())/tm.getRes() + bufx),
			ceil ((tm.getY0() - grid->bbox.ymin)/tm.getRes() + bufy));

	Image* image = getwindow(bbox_int);
	image->setbbox( BoundingBox<double>(tm.getX0() + tm.getRes() * bbox_int.xmin, tm.getY0() - tm.getRes() * bbox_int.ymax, tm.getX0() + tm.getRes() * bbox_int.xmax, tm.getY0() - tm.getRes() * bbox_int.ymin));

	return new ReprojectedImage(image, bbox, grid/*,Kernel::LINEAR*/);
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

	// Rappel : les coordonnees de la bbox sont ici en pixels
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

	int tile_ymin = bbox.ymin / tm.getTileH();
	int tile_ymax = (bbox.ymax-1) / tm.getTileH();
	int nby = tile_ymax - tile_ymin + 1;
	int left[nbx];   memset(left,   0, nbx*sizeof(int)); left[0] = bbox.xmin % tm.getTileW();
	int top[nby];    memset(top,    0, nby*sizeof(int)); top[0]  = bbox.ymin % tm.getTileH();
	int right[nbx];  memset(right,  0, nbx*sizeof(int)); right[nbx - 1] = tm.getTileW() - ((bbox.xmax -1) % tm.getTileW()) - 1;
	int bottom[nby]; memset(bottom, 0, nby*sizeof(int)); bottom[nby- 1] = tm.getTileH() - ((bbox.ymax -1) % tm.getTileH()) - 1;

	std::vector<std::vector<Image*> > T(nby, std::vector<Image*>(nbx));
	for(int y = 0; y < nby; y++)
		for(int x = 0; x < nbx; x++)
			T[y][x] = getTile(tile_xmin + x, tile_ymin + y, left[x], top[y], right[x], bottom[y]);

	if(nbx == 1 && nby == 1) return T[0][0];
	else return new CompoundImage(T);
}

/*
 * Tableau statique des caractères Base36 (pour système de fichier non cas-sensitive)
 */
// static const char* Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
static const char* Base36 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*
 * Recuperation du nom de fichier de la dalle du cache en fonction de son indice
 */
std::string Level::getFilePath(int tilex, int tiley)
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

	return baseDir + (path + pos);
}

/*
 * @ return la tuile d'indice (x,y) du niveau
 */



DataSource* Level::getEncodedTile(int x, int y) {
	// TODO: return 0 sur des cas d'erreur..
	// Index de la tuile (cf. ordre de rangement des tuiles)
	int n=(y%tilesPerHeight)*tilesPerWidth + (x%tilesPerWidth);
	// Les index sont stockés à partir de l'octet 2048
	uint32_t posoff=2048+4*n, possize=2048+4*n +tilesPerWidth*tilesPerHeight*4;
	std::string path=getFilePath(x, y);
	LOGGER_DEBUG(path);
	return new FileDataSource(path.c_str(),posoff,possize,getMimeType(format));
}

DataSource* Level::getDecodedTile(int x, int y)
{
	DataSource* encData = getEncodedTile(x, y);

	if (format.compare("TIFF_INT8")==0 || format.compare("TIFF_FLOAT32")==0)
		return encData;
	else if (format.compare("TIFF_JPG_INT8")==0)
		return new DataSourceDecoder<JpegDecoder>(encData);
	else if (format.compare("TIFF_PNG_INT8")==0)
		return new DataSourceDecoder<PngDecoder>(encData);

	LOGGER_ERROR("Type d'encodage inconnu : "<<format); 
	return 0;
}


DataSource* Level::getTile(int x, int y) {
	DataSource* source=getEncodedTile(x, y);
	size_t size;
	if (format.compare("TIFF_INT8")==0 && source!=0 && source->getData(size)!=0){
                RawImage* raw=new RawImage(tm.getTileW(),tm.getTileH(),channels,source);
                TiffEncoder TiffStream(raw);
                return new DataSourceProxy(new BufferedDataSource(TiffStream),*noDataSource);
        }
	return new DataSourceProxy(source, *noDataSource);
}

Image* Level::getTile(int x, int y, int left, int top, int right, int bottom) {
	int pixel_size=1;
	if (format.compare("TIFF_FLOAT32")==0)
		pixel_size=4;
	return new ImageDecoder(getDecodedTile(x,y), tm.getTileW(), tm.getTileH(), channels,			
			BoundingBox<double>(tm.getX0() + x * tm.getTileW() * tm.getRes(),
				tm.getY0() + y * tm.getTileH() * tm.getRes(), 
				tm.getX0() + (x+1) * tm.getTileW() * tm.getRes(),
				tm.getY0() + (y+1) * tm.getTileH() * tm.getRes()),
			left, top, right, bottom, pixel_size);
}

