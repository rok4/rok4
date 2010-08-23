#ifndef GRID_H
#define GRID_H

#include "BoundingBox.h"
#include <string>

class Grid {
  private:


  // Pas d'interpollation : seules coordonnées des pixels dont les coordonnées image (en pixel) 
  // sont des multiples de step seront effectivement reporjetés. Les autres seront interpollés.
  static const int step = 16;

  /**
   * Met à jour la boundingBox de la grille
   * Est apellé après chaque transformations
   */
  void update_bbox();

  // Nombre de points reprojetés en X. nbx = 2 + (width-1)/step
  int nbx;
  
  // Nombre de points reprojetés en Y. nby = 2 + (height-1)/step
  int nby;

  // Grille d'interpollation des pixels
  // Le centre du pixel cible (i*step, j*step) correspond au pixel source (gridX[i + nbx*j], gridY[i + nbx*j])
  // Chaque tableau contient nbx*nby éléments
  double *gridX, *gridY;


  public:

  int width, height;

  BoundingBox<double> bbox;

  Grid(int width, int height, BoundingBox<double> bbox);

  ~Grid();


  

  /**
   * Effecture une reprojection sur les coordonnées de la grille
   * 
   * (GridX[i], GridY[i]) = proj(GridX[i], GridY[i])
   * ou proj est une projection de from_srs vers to_srs
   */
  void reproject(std::string from_srs, std::string to_srs);

  /**
   * Effectue une transformation affine sur tous les éléments de la grille
   * GridX[i] = Ax * GridX[i] + Bx
   * GridY[i] = Ay * GridY[i] + By
   */
  void affine_transform(double Ax, double Bx, double Ay, double By);

  int interpolate_line(int line, float* X, float* Y, int nb);

};

#endif
