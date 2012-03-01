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
  ReprojectedImage(Image *image,  BoundingBox<double> bbox, Grid* grid,  Kernel::KernelType KT = Kernel::LANCZOS_2);

  int getline(float* buffer, int line);

  int getline(uint8_t* buffer, int line);

  ~ReprojectedImage();

};

#endif
