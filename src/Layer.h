#ifndef LAYER_H
#define LAYER_H

#include "Image.h"
#include "HttpResponse.h"
#include "Tile.h"

#include "BoundingBox.h"

  /** D */
class Layer {  
  protected:
  /**
   * Renvoie une image de taille width, height
   *
   * le coin haut gauche de cette image est le pixel offsetx, offsety de la tuile tilex, tilex.
   * Toutes les coordonnées sont entière depuis le coin haut gauche.
   */
  virtual Image* getwindow(BoundingBox<int64_t> src_bbox) = 0;

  public:
  /**
   * Système de coordonnées de la couche.
   * Correspond au code RIG, doit être compréhensible par proj.4
   * Exemple : "IGNF:LAMB93"
   */
  const char *crs;


  const int channels;

  /**
   * Résolutions X et Y des données en unité de projection par pixel
   */
  const double resolution_x;
  const double resolution_y;

  /**
   * La grille de tuile est alignée sur une origine (Point haut gauche)
   * Coordonnées X et Y de l'origine dans le SRS 
   */
  const double X0;
  const double Y0;

  /** D */
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
  virtual HttpResponse* gettile(int x, int y) = 0;

  /** D */
   Layer(const char *crs, int channels, double resolution_x, double resolution_y, double X0, double Y0) : crs(crs), channels(channels), resolution_x(resolution_x), resolution_y(resolution_y), X0(X0), Y0(Y0) {};
};




/** D */
template<class Decoder>
class TiledLayer : public Layer {
  private:

  typedef typename Decoder::data_t data_t;

 // RawTile<data_t> nodataTile;

  std::string basedir;
  int path_depth;

  /**
   * Nombre de tuiles par block est carré et est composé de 
   * tileblocksize * tileblocksize tuiles
   */
  uint32_t tiles_per_width;
  uint32_t tiles_per_height;

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

  /*
   * Dimensions (largeur et hauteur) des tuiles en nombre de pixel
   * Nous ne supportons que des tuiles carrées
   * Les tailles typiques seront 256*256 ou 512*512
   */
  const int tile_width;
  const int tile_height;

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
 TiledLayer(const char *crs, int tile_width, int tile_height, int channels, double resolution_x, double resolution_y, double X0, double Y0, std::string basedir, int tpw, int tph, int path_depth) : Layer(crs, channels, resolution_x, resolution_y, X0, Y0), tile_width(tile_width), tile_height(tile_height), basedir(basedir), tiles_per_width(tpw), tiles_per_height(tph), path_depth(path_depth) {}

/** D */



};

#endif




