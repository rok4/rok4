#ifndef REPROJECT_H
#define REPROJECT_H

#include "Image.h"
#include "Grid.h"
#include "Kernel.h"

class ReprojectedImage : public Image {
  private:
  // Image source à rééchantilloner.
  Image* image;

  // Noyau de la méthode d'interpollation à utiliser.
  const Kernel& K;

  // Nombre maximal de pixels sources pris en compte pour une interpolation en x.
  int Kx;
  
  // Nombre maximal de pixels sources pris en compte pour une interpolation en y.
  int Ky;

  double ratio_x, ratio_y;

  Grid* grid;

  float*  __buffer;

  int*    src_line_index;
  float** src_line_buffer;
  
  int dst_line_index;
  float* dst_line_buffer[4];
  float* mux_dst_line_buffer;

  float* X[4];
  float* Y[4];
  float* Wx[1024];
  float* Wy[1024];
  

  float* WWx;
  float* WWy;
  int xmin[1024];
  int ymin[1024];

  float* TMP1;
  float* TMP2;


  float* compute_dst_line(int line);

  public:
  ReprojectedImage(Image *image,  BoundingBox<double> bbox, Grid* grid,  Kernel::KernelType KT = Kernel::LANCZOS_3);

  int getline(float* buffer, int line);

  int getline(uint8_t* buffer, int line);

  ~ReprojectedImage();

};

#endif
