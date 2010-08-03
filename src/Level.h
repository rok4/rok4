#ifndef LEVEL_H
#define LEVEL_H

#include "Image.h"
#include "HttpResponse.h"
#include "Tile.h"

#include "BoundingBox.h"
#include "TileMatrix.h"

  /** La classe Level est une interface permettant d'utiliser les différents TiledLevel<Decoder>
   *  de façon unifié. Cette interface à vocation à disparaitre au profit d'un TiledLevel unique
   *  sans template. */
class Level {
  protected:
  /**
   * Renvoie une image de taille width, height
   *
   * le coin haut gauche de cette image est le pixel offsetx, offsety de la tuile tilex, tiley.
   * Toutes les coordonnées sont entières depuis le coin haut gauche.
   */
  virtual Image* getwindow(BoundingBox<int64_t> src_bbox) = 0;

  public:
  /** D */
  virtual Image* getbbox(BoundingBox<double> bbox, int width, int height)=0;

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
  virtual HttpResponse* gettile(int x, int y) = 0;
  virtual double        getRes()=0;

  /**
   * Destructeur virtuel. Permet de lancer les destructeurs des classes filles
   * lors de la destruction d'un pointeur Level.
   */
   virtual ~Level(){};
};




/** La classe TiledLevel utilise un template <Decoder>.
 *  C'est pour en simplifier l'utilisation que la classe Level a été créée.
 *  Il ne s'agit pas d'un type de Level particulier, il n'y aura pas de Level
 *  non tuilé.
 *  Il est prévu de simplifier cela en supprimant le template et en ne faisant
 *  qu'une seule classe Level. Cette modification n'étant pas simple, elle est
 *  prévue pour plus tard.*/
template<class Decoder>
class TiledLevel : public Level {
  private:

  typedef typename Decoder::data_t data_t;

  std::string baseDir;
  int pathDepth;
  TileMatrix  & tm;         // FIXME j'ai des problème de compil que je ne comprends pas si je mets un const ?!
  const std::string format; //format d'image des block de tuiles du cache.
  const int channels;
  const int32_t maxTileRow;
  const int32_t minTileRow;
  const int32_t maxTileCol;
  const int32_t minTileCol;
  /**
   * Nombre de tuiles par block est carré et est composé de 
   * tileblocksize * tileblocksize tuiles
   */
  uint32_t blockW; //largeur des blocs du cache en tuiles
  uint32_t blockH; //hauteur des blocs du cache en tuiles

  std::string getfilepath(int tilex, int tiley);

  protected:
  /**
   * Renvoie une image de taille width, height
   *
   * le coin haut gauche de cette image est le pixel offsetx, offsety de la tuile tilex, tilex.
   * Toutes les coordonnées sont entière depuis le coin haut gauche.
   */
   Image* getwindow(BoundingBox<int64_t> src_bbox);


  public:
  TileMatrix const & getTm();
  std::string getFormat();
  uint32_t    getMaxTileRow();
  uint32_t    getMinTileRow();
  uint32_t    getMaxTileCol();
  uint32_t    getMinTileCol();
  double      getRes();
  std::string getId();

  Image* getbbox(BoundingBox<double> bbox, int width, int height);
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
  StaticHttpResponse* gettile(int x, int y);

   
/** D */
 TiledLevel(TileMatrix &tm, int channels, std::string baseDir,
		    int blockW, int blockH,
		    uint32_t maxTileRow, uint32_t minTileRow, uint32_t maxTileCol, uint32_t minTileCol,
		    int pathDepth) :
	        Level(), tm(tm), channels(channels), baseDir(baseDir),
	        blockW(blockW), blockH(blockH),
		    maxTileRow(maxTileRow), minTileRow(minTileRow), maxTileCol(maxTileCol), minTileCol(minTileCol),
	        pathDepth(pathDepth) {}

};

#endif




