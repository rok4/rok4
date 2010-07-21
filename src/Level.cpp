
#include "Level.h"
#include "FileManager.h"

#include "CompoundImage.h"
#include "ResampledImage.h"

#include <cmath>
#include "Logger.h"
#include "Kernel.h"

#define EPS 1./256. // FIXME: La valeur 256 est liée au nombre de niveau de valeur d'un canal
                    //        Il faudra la changer lorsqu'on aura des images non 8bits.

template<class Decoder>
TileMatrix const & TiledLevel<Decoder>::getTm(){return tm;}
template<class Decoder>
std::string TiledLevel<Decoder>::getFormat(){return format;}
template<class Decoder>
uint32_t TiledLevel<Decoder>::getMaxTileRow(){return maxTileRow;}
template<class Decoder>
uint32_t TiledLevel<Decoder>::getMinTileRow(){return minTileRow;}
template<class Decoder>
uint32_t TiledLevel<Decoder>::getMaxTileCol(){return maxTileCol;}
template<class Decoder>
uint32_t TiledLevel<Decoder>::getMinTileCol(){return minTileCol;}
template<class Decoder>
double TiledLevel<Decoder>::getRes(){return tm.getRes();}
template<class Decoder>
std::string TiledLevel<Decoder>::getId(){return tm.getId();}

template<class Decoder>
Image* TiledLevel<Decoder>::getbbox(BoundingBox<double> bbox, int width, int height) {
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

  const Kernel& kk = Kernel::getInstance("Lanczos_2");

  bbox_int.xmin = floor(bbox.xmin - kk.size(ratio_x));
  bbox_int.xmax = ceil (bbox.xmax + kk.size(ratio_x));
  bbox_int.ymin = floor(bbox.ymin - kk.size(ratio_y));
  bbox_int.ymax = ceil (bbox.ymax + kk.size(ratio_y));
  return new ResampledImage(getwindow(bbox_int), width, height, bbox.xmin - bbox_int.xmin, bbox.ymin - bbox_int.ymin, ratio_x, ratio_y);
//    return new ResampledImage<NearestNeighbour>(getwindow(bbox_int), width, height, bbox.xmin - bbox_int.xmin, bbox.ymin - bbox_int.ymin, ratio_x, ratio_y);
}


template<class Decoder>
Image* TiledLevel<Decoder>::getwindow(BoundingBox<int64_t> bbox) {

  int tile_xmin = bbox.xmin / tm.getTileW();
  int tile_xmax = (bbox.xmax -1)/ tm.getTileW();
  int nbx = tile_xmax - tile_xmin + 1; 
  LOGGER_DEBUG(" getwindow tile_xmin:" << tile_xmin << " tile_xmax:" << tile_xmax << " nb_x:" << nbx << " tileW:" << tm.getTileW() << " " );

  int tile_ymin = bbox.ymin / tm.getTileH();
  int tile_ymax = (bbox.ymax-1) / tm.getTileH();
  int nby = tile_ymax - tile_ymin + 1;

  int left[nbx];   memset(left,   0, nbx*sizeof(int)); left[0] = bbox.xmin % tm.getTileW();
  int top[nby];    memset(top,    0, nby*sizeof(int)); top[0]  = bbox.ymin % tm.getTileH();
  int right[nbx];  memset(right,  0, nbx*sizeof(int)); right[nbx - 1] = tm.getTileW() - ((bbox.xmax -1) % tm.getTileW()) - 1;
  int bottom[nby]; memset(bottom, 0, nby*sizeof(int)); bottom[nby- 1] = tm.getTileH() - ((bbox.ymax -1) % tm.getTileH()) - 1;

  std::vector<std::vector<Image*> > T(nby, std::vector<Image*>(nbx));
  for(int y = 0; y < nby; y++)
    for(int x = 0; x < nbx; x++) {
      LOGGER_DEBUG(" getwindow " << x << " " << y << " " << nbx << " " << nby << " " << left[x] << " " << right[x] << " " << top[y] << " " << bottom[y] );      
      StaticHttpResponse* tile = gettile(tile_xmin + x, tile_ymin + y);
      T[y][x] = new Tile<Decoder>(tm.getTileW(), tm.getTileH(), channels, tile, left[x], top[y], right[x], bottom[y]);
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
template<class Decoder>
std::string TiledLevel<Decoder>::getfilepath(int tilex, int tiley) {
  LOGGER_DEBUG (" getfilepath " << tilex << " " << tiley) ;

  int x = tilex / blockW;
  int y = tiley / blockH;
  char path[32];
  path[sizeof(path) - 5] = '.';
  path[sizeof(path) - 4] = 't';
  path[sizeof(path) - 3] = 'i';
  path[sizeof(path) - 2] = 'f';
  path[sizeof(path) - 1] = 0;
  int pos = sizeof(path) - 6;

  for(int d = 0; d < pathDepth; d++) {
    path[pos--] = Base36[y & 35];
    path[pos--] = Base36[x & 35];
    path[pos--] = '/';
    x >>= 6;
    y >>= 6;
  }
  do {
    path[pos--] = Base36[y & 35];
    path[pos--] = Base36[x & 35];
    x >>= 6;
    y >>= 6;
  } while(x || y);
  path[pos] = '/';

  LOGGER_DEBUG(" getfilepath " << (path +pos));


  return baseDir + (path + pos);
}


template<class Decoder>
StaticHttpResponse* TiledLevel<Decoder>::gettile(int x, int y)
{
  LOGGER_DEBUG( " TiledLevel: gettile " << x << " " << y );
  
  if(x < 0 || y < 0) {
    data_t* T = new data_t[tm.getTileW()*tm.getTileH()*channels];
    return new StaticHttpResponse("bubu", T, tm.getTileW()*tm.getTileH()*channels*sizeof(data_t));
  }

  std::string file_path = getfilepath(x, y);

//  LOGGER_DEBUG( " TiledLevel: gettile " << file_path );

  uint32_t size;
  int n=(y%blockH)*blockW + (x%blockW); // Index de la tuile
  uint32_t posoff=1024+4*n, possize=1024+4*n +blockW*blockH*4;

  LOGGER_DEBUG( " TiledLevel: gettile " << posoff << " " <<  possize );
  const uint8_t *data = FileManager::gettile(file_path, size, posoff, possize);
  LOGGER_DEBUG( " TiledLevel: gettile " << size );

  if(data) return new StaticHttpResponse("bubu", data, size);
  else {
    LOGGER_DEBUG( " TiledLevel: gettile " << size );

    data_t* T = new data_t[tm.getTileW()*tm.getTileH()*channels];
    return new StaticHttpResponse("bubu", T, tm.getTileW()*tm.getTileH()*channels*sizeof(data_t));
  }
}




template class TiledLevel<RawDecoder>;
//template class Level<pixel_gray>;
//template class Level<pixel_float>;

//template class TiledFileLevel<RawTile ,pixel_rgb>;
//template class TiledFileLevel<RawTile ,pixel_float>;
//template class TiledFileLevel<PngTile ,pixel_rgb>;
//template class TiledFileLevel<JpegTile,pixel_rgb>;

//template class TiledFileLevel<RawTile ,pixel_gray>;
//template class TiledFileLevel<PngTile ,pixel_gray>;
//template class TiledFileLevel<JpegTile,pixel_gray>;


/*
template class TiledFileLevel<pixel_rgb>;
template class RawTiffLevel<pixel_rgb>;
template class JpegTiffLevel<pixel_rgb>;
template class PngTiffLevel<pixel_gray>;
*/



