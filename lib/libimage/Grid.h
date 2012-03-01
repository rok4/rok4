/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière 
 * 
 * Géoportail SAV <geop_services@geoportail.fr>
 * 
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 * 
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * 
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

#ifndef GRID_H
#define GRID_H

#include "BoundingBox.h"
#include <string>

class Grid {
  private:


  // Pas d'interpolation : seules coordonnées des pixels dont les coordonnées image (en pixel) 
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

  // Grille d'interpolation des pixels
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
   bool reproject(std::string from_srs, std::string to_srs);

  /**
   * Effectue une transformation affine sur tous les éléments de la grille
   * GridX[i] = Ax * GridX[i] + Bx
   * GridY[i] = Ay * GridY[i] + By
   */
  void affine_transform(double Ax, double Bx, double Ay, double By);

  int interpolate_line(int line, float* X, float* Y, int nb);

};

#endif
