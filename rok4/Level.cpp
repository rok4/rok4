
#include "Level.h"
#include "FileManager.h"

#include "CompoundImage.h"
#include "ResampledImage.h"

#include "ReprojectedImage.h"

#include <cmath>
#include "Logger.h"
#include "Kernel.h"

#define EPS 1./256. // FIXME: La valeur 256 est liée au nombre de niveau de valeur d'un canal
                    //        Il faudra la changer lorsqu'on aura des images non 8bits.

TileMatrix const & Level::getTm(){return tm;}
std::string Level::getFormat(){return format;}
uint32_t Level::getMaxTileRow(){return maxTileRow;}
uint32_t Level::getMinTileRow(){return minTileRow;}
uint32_t Level::getMaxTileCol(){return maxTileCol;}
uint32_t Level::getMinTileCol(){return minTileCol;}
double Level::getRes(){return tm.getRes();}
std::string Level::getId(){return tm.getId();}
int Level::getTileCoding() {
  if (format.compare("TIFF_INT8")==0)
        return RAW_UINT8;
  else if (format.compare("TIFF_JPG_INT8")==0)
        return JPEG_UINT8;
  else if (format.compare("TIFF_PNG_INT8")==0)
        return PNG_UINT8;
  LOGGER_ERROR("Type d'encodage inconnu : "<<format); 
  return 0;
}

/*
 * A REFAIRE
 */
Image* Level::getbbox(BoundingBox<double> bbox, int width, int height, const char* dst_crs) {

  Grid* grid = new Grid(width, height, bbox);
  grid->bbox.print();

  grid->reproject(dst_crs, "IGNF:LAMB93"); // FIXME : prendre en compte le SRS du cache (pas forcément LAMB93)

  grid->bbox.print();

  BoundingBox<int64_t> bbox_int(floor((grid->bbox.xmin - tm.getX0())/tm.getRes() - 50),
                                floor((tm.getY0() - grid->bbox.ymax)/tm.getRes() - 50),
                                ceil ((grid->bbox.xmax - tm.getX0())/tm.getRes() + 50),
                                ceil ((tm.getY0() - grid->bbox.ymin)/tm.getRes() + 50));
  // TODO : remplacer 50 par un buffer calculé en fonction du noyau d'interpollation

  bbox_int.print();
  
  Image* image = getwindow(bbox_int);
  image->bbox.xmin = tm.getX0() + tm.getRes() * bbox_int.xmin;
  image->bbox.xmax = tm.getX0() + tm.getRes() * bbox_int.xmax;
  image->bbox.ymin = tm.getY0() - tm.getRes() * bbox_int.ymax;
  image->bbox.ymax = tm.getY0() - tm.getRes() * bbox_int.ymin;

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
//    return new ResampledImage<NearestNeighbour>(getwindow(bbox_int), width, height, bbox.xmin - bbox_int.xmin, bbox.ymin - bbox_int.ymin, ratio_x, ratio_y);
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
      LOGGER_DEBUG(" getwindow " << x << " " << y << " " << nbx << " " << nby << " " << left[x] << " " << right[x] << " " << top[y] << " " << bottom[y] );      
      StaticHttpResponse* tile = gettile(tile_xmin + x, tile_ymin + y);
      T[y][x] = new Tile(tm.getTileW(), tm.getTileH(), channels, tile, left[x], top[y], right[x], bottom[y],tileCoding);
    }

  if(nbx == 1 && nby == 1) return T[0][0];  
  else return new CompoundImage(T);
}






/*
 * Tableau statique des caractères Base64 (pour système de fichier)
 */
// static const char* Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
static const char* Base36 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*
 *
 *
 */
std::string Level::getfilepath(int tilex, int tiley) {
  LOGGER_DEBUG (" getfilepath " << tilex << " " << tiley << " " << tilesPerWidth << " " << tilesPerHeight) ;

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

  LOGGER_DEBUG(" getfilepath " << (path +pos));

  return baseDir + (path + pos);
}


StaticHttpResponse* Level::gettile(int x, int y)
{
  int tileCoding=getTileCoding();
  int typeSize=1;
  if (tileCoding==RAW_UINT8 || tileCoding==JPEG_UINT8 || tileCoding==PNG_UINT8)
	typeSize=1;
  else if(RAW_FLOAT)
	typeSize=4;
  else
	LOGGER_ERROR("Taille du type non connu");

  LOGGER_DEBUG( " Level: gettile " << x << " " << y );
  
  if(x < 0 || y < 0) {
    uint8_t* T = new uint8_t[tm.getTileW()*tm.getTileH()*channels*typeSize];
    return new StaticHttpResponse("image/jpeg", T, tm.getTileW()*tm.getTileH()*channels*typeSize);
  }

  std::string file_path = getfilepath(x, y);

  LOGGER_DEBUG( " Level: gettile " << file_path );

  uint32_t size;
  int n=(y%tilesPerHeight)*tilesPerWidth + (x%tilesPerWidth); // Index de la tuile
  uint32_t posoff=1024+4*n, possize=1024+4*n +tilesPerWidth*tilesPerHeight*4;

  LOGGER_DEBUG( " Level: gettile " << posoff << " " <<  possize );
  const uint8_t *data = FileManager::gettile(file_path, size, posoff, possize);
  LOGGER_DEBUG( " Level: gettile " << size );

  if(data) return new StaticHttpResponse("image/jpeg", data, size);
  else {
    LOGGER_DEBUG( " Level: gettile " << size );

    uint8_t* T = new uint8_t[tm.getTileW()*tm.getTileH()*channels*typeSize];
    return new StaticHttpResponse("image/jpeg", T, tm.getTileW()*tm.getTileH()*channels*typeSize);
  }
}

