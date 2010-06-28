#include "Pyramid.h"
#include <cmath>
#include "Logger.h"

HttpResponse* Pyramid::gettile(int x, int y, int z) {
/*  assert(x >= 0);
  assert(y >= 0);
  assert(z >= 0);
  assert(z < (int) Levels.size());
*/
  return Levels[z]->gettile(x, y);
}


inline double dist(double x, double y) {
  return sqrt(x*x + y*y);
}


int Pyramid::best_scale(double resolution_x, double resolution_y) {

  // TODO: A REFAIRE !!!!

  double resolution = sqrt(resolution_x * resolution_y);

  int best_h = 0;
  double best = resolution_x / sqrt(Levels[0]->resolution_x * Levels[0]->resolution_y);

  for(unsigned int h = 1; h < nb_levels; h++) {
    double d = resolution / sqrt(Levels[h]->resolution_x * Levels[h]->resolution_y);


    if(best < 0.8 && d > best) {
      best = d;
      best_h = h;
    }
    else if(best >= 0.8 && d >= 0.8 && d < best) {
      best = d;
      best_h = h;
    }
  }
  return best_h;
}


Image* Pyramid::getbbox(BoundingBox<double> bbox, int width, int height, const char *dst_crs) {
/*  if(dst_crs) {
    LOGGER_DEBUG( "crs dest=" <<dst_crs << " crs ini=" << crs);
    if(strcmp(crs, dst_crs) == 0) dst_crs = 0;
    else { // reprojection

    }
  }
*/
  // on calcule la résolution de la requete dans le crs source selon une diagonale de l'image.
  double resolution_x = (bbox.xmax - bbox.xmin) / width;
  double resolution_y = (bbox.ymax - bbox.ymin) / height;
  int h = best_scale(resolution_x, resolution_y);
  return Levels[h]->getbbox(bbox, width, height);

/*
  LOGGER_DEBUG( "best_scale=" << h << " resolution requete=" << resolution );
  
  if(!dst_crs) return Levels[h]->getbbox(bbox, width, height);
  else return Levels[h]->getbbox(bbox, width, height, dst_crs);
  */
}


