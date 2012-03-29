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

#ifndef RESAMPLED_IMAGE_H
#define RESAMPLED_IMAGE_H

#include "Image.h"
#include "Kernel.h"
#include "Interpolation.h"


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
  ResampledImage(Image *image, int width, int height, double left, double top, double ratio_x, double ratio_y, Interpolation::KernelType KT = Interpolation::LANCZOS_3, BoundingBox<double> bbox = BoundingBox<double>(0.,0.,0.,0.));
  ~ResampledImage();
  int getline(float* buffer, int line);
  int getline(uint8_t* buffer, int line);

};

#endif




