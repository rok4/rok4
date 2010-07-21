#ifndef RESAMPLED_IMAGE_H
#define RESAMPLED_IMAGE_H

#include "Image.h"
#include "Kernel.h"
#include "Convert.h"

#include <xmmintrin.h>
#include <mm_malloc.h>

#include "Logger.h"

class ResampledImage : public Image {
  private:
  Image* image;

  const Kernel& K;


  /**
   * Taille du noyau en nombre de pixels
   */ 
  int Kx, Ky;

  /**
   * Fenêtre de l'image à rééchantillonner
   */
  double top, left, ratio_x, ratio_y;


  float* __buffer;  
  
  // Buffers de travail en float
  float* src_line_buffer;
  float** resampled_line;
  float *dst_line_buffer;
  int* resampled_line_index;


//  float* resampled_src_line;

  float* resample_src_line(int line);
  float* compute_dst_line(int line);



  float* Wx;
  int* xmin;

  public:
  ResampledImage(Image *image, int width, int height, double left, double top, double ratio_x, double ratio_y);
  ~ResampledImage();
  int getline(float* buffer, int line);
  int getline(uint8_t* buffer, int line) {
//    LOGGER_DEBUG( "ResampledImage getline uint8_t" );
    float* dst = compute_dst_line(line);
    convert(buffer, dst, width*channels);
    return width*channels;
  }

};

#endif




