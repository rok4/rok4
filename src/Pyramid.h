#ifndef PYRAMID_H
#define PYRAMID_H

#include "Layer.h"

  /**
   * D
   */

class Pyramid {  
  
  private:

  const int nb_layers;
  Layer** Layers;

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
  Pyramid(Layer** Layers, int nb_layers) : Layers(Layers), nb_layers(nb_layers) {}

  ~Pyramid() {}
};



#endif
