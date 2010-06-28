#ifndef PYRAMID_H
#define PYRAMID_H

#include "Level.h"

  /**
   * D
   */

class Pyramid {  
  
  private:

  const int nb_levels;
  Level** Levels;

  /**
   * D
   */
  int best_scale(double resolution_x, double resolution_y);

  public:
//  const char *crs;
//  const char *format;    // default image format
//  const bool transparent;

  /**
   * D
   */
  HttpResponse* gettile(int x, int y, int z);

  /**
   * D
   */
  Image* getbbox(BoundingBox<double> bbox, int width, int height, const char *dst_crs = 0);

  /**
   * D
   */
  Pyramid(Level** Levels, int nb_levels) : Levels(Levels), nb_levels(nb_levels) {}


  ~Pyramid() {}
};



#endif
