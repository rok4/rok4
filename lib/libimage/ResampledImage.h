#ifndef RESAMPLED_IMAGE_H
#define RESAMPLED_IMAGE_H

#include "Image.h"
#include "Kernel.h"


class ResampledImage : public Image {
  private:
  // Image source à rééchantilloner.
  Image* image;

  // Noyau de la méthode d'interpolation à utiliser.
  const Kernel& K;

  // Nombre maximal de pixels sources pris en compte pour une interpolation en x.
  int Kx;
  
  // Nombre maximal de pixels sources pris en compte pour une interpolation en y.
  int Ky;

  // Ratio de rééchantillonage en x = résolution x source / résolution x cible
  double ratio_x;

  // Ratio de rééchantillonage en y = résolution y source / résolution y cible
  double ratio_y;

  // Offset en y du haut de l'image rééchantillonée par rapport à l'image source (en nombre de pixels source)
  double top;

  // Offset en x de bord gauche l'image rééchantillonée par rapport à l'image source (en nombre de pixels source)
  double left;


  float* __buffer;  
  
  // Buffers de travail en float
  float* src_line_buffer[4];
  float* mux_src_line_buffer;
  float* mux_resampled_line;


  float** resampled_line;
  float *dst_line_buffer;
  int* resampled_line_index;



/**
 * Rééchantillone une ligne source selon les x.
 *
 * @param line Indice de la ligne de l'image source
 * @return pointeur vers la ligne rééchantillonnée de width pixels.
 */
  float* resample_src_line(int line);


  float* Wx;
  int* xmin;

  public:
  ResampledImage(Image *image, int width, int height, double left, double top, double ratio_x, double ratio_y, Kernel::KernelType KT = Kernel::LANCZOS_3, BoundingBox<double> bbox = BoundingBox<double>(0.,0.,0.,0.));
  ~ResampledImage();
  int getline(float* buffer, int line);
  int getline(uint8_t* buffer, int line);

};

#endif




