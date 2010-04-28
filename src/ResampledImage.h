#ifndef RESAMPLED_IMAGE_H
#define RESAMPLED_IMAGE_H

#include "Image.h"
#include "Kernel.h"
#include "Convert.h"

template<class Kernel>
class ResampledImage : public Image {
  private:
  Image* image;
  
  int Kx, Ky;
  double top, left, ratio_x, ratio_y;

  float* src_line;
  float* resampled_src_line;
  float* resample_src_line(int line);

  float* float_buffer;
  float* Wx;
  int* xmin;



  public:
  ResampledImage(Image *image, int width, int height, double left, double top, double ratio_x, double ratio_y);
  ~ResampledImage();
  int getline(float* buffer, int line);
  int getline(uint8_t* buffer, int line) {
    return convert(buffer, float_buffer, getline(float_buffer, line));
  }

};


/** D */
template<>
class ResampledImage<NearestNeighbour> : public Image {
  private:
  Image* image;     // image source  
  double top, left, ratio_x, ratio_y;

  template<typename T> 
  int _getline(T* buffer, int line);

  public:
/** D */
  ResampledImage(Image *image, int width, int height, double left, double top, double ratio_x, double ratio_y);
/** D */
  ~ResampledImage();

/** D */
  int getline(uint8_t *buffer, int line) {return _getline(buffer, line);}
/** D */
  int getline(float *buffer, int line) {return _getline(buffer, line);}

};



#endif




