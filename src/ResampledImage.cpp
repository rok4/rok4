#include "ResampledImage.h"
#include "Logger.h"
#include "Convert.h"
#include <cstring>

ResampledImage<NearestNeighbour>::ResampledImage(Image *image, int width, int height, double left, double top, double ratio_x, double ratio_y) :
  Image(width, height, image->channels), image(image) , left(left), top(top), ratio_x(ratio_x), ratio_y(ratio_y) {
/*      assert(image);
      assert(left >= 0);
      assert(top >= 0);
      assert(right > left && right <= image->width);
      assert(bottom > top && bottom <= image->height);    
*/
    LOGGER_DEBUG( "Constructeur NearestNeighbourResampledImage" );
    }


ResampledImage<NearestNeighbour>::~ResampledImage() {
    LOGGER_DEBUG( "delete NearestNeighbourResampledImage");
//    assert(image);
    delete image;
  }


/*
template<class Kernel>
int Resample<Kernel>::getline(float* buffer, int line) {return 1;}
*/


template<class T>
int ResampledImage<NearestNeighbour>::_getline(T *buffer, int line) {    

//  LOGGER_DEBUG(" Resample: _getline " << line);  
 //   assert(line >= 0 && line < this->height);
    int l = (int) (top + line * ratio_y + 0.5);
 //   assert(l >= 0 && l < image->height);

    T * src_line = new T[image->width*channels];   
    image->getline(src_line, l);

    for(int x = 0; x < width; x++) {
      int src_x = (int) (left + x * ratio_x + 0.5);
   //   assert(src_x >= 0 && src_x < image->width);
      for(int c = 0; c < channels; c++)
        *(buffer++) = src_line[src_x*channels + c];
    }
    delete[] src_line;
    return width;
  }








template<class Kernel>
ResampledImage<Kernel>::ResampledImage(Image *image, int width, int height, double left, double top, double ratio_x, double ratio_y) :
  Image(width, height, image->channels), image(image) , left(left), top(top), ratio_x(ratio_x), ratio_y(ratio_y) {

    Kx = ceil(2 * Kernel::size(ratio_x));
    Ky = ceil(2 * Kernel::size(ratio_y));
    resampled_src_line = new float[width*channels];
    src_line = new float[image->width*image->channels];
    float_buffer = new float[width*channels];
    Wx = new float[Kx * width]; 
    xmin = new int[width];
    memset(Wx, 0, Kx * width * sizeof(float));
    for(int x = 0; x < width; x++) {
      int nb = Kx;
      xmin[x] = Kernel::weight(Wx + x*Kx, nb, left + x * ratio_x, ratio_x);
    }
}


template<class Kernel>
float* ResampledImage<Kernel>::resample_src_line(int line) {
  image->getline(src_line, line);
//  float weights[Ky];

  for(int i = 0; i < width*channels; i++) resampled_src_line[i] = 0.;

  for(int x = 0; x < width; x++) {
 //   int nb_weights = Kx;
 //   int xmn = Kernel::weight(weights, nb_weights, left + x * ratio_x, ratio_x);
    
    for(int c = 0; c < channels; c++) 
      for(int k = 0; k < Kx; k++) 
        resampled_src_line[channels*x + c] += Wx[x*Kx + k] * src_line[channels * (xmin[x] + k) + c];  
//    LOGGER(DEBUG) << x << " " << resampled_src_line[channels*x] << " " << weights[0] << " " <<  src_line[channels * xmin] << std::endl;
  }
  return resampled_src_line;
}


template<class Kernel>
int ResampledImage<Kernel>::getline(float* buffer, int line) {
//  LOGGER(DEBUG) << " ResampledImage: getline " << line << std::endl;  
  float weights[Ky];
  int nb_weights = Ky;
  int ymin = Kernel::weight(weights, nb_weights, top + line * ratio_y, ratio_y); // On calcule les coefficient d'interpollation

  memset(buffer, 0, width*channels*sizeof(float));

  for(int k = 0; k < nb_weights; k++) {
//    LOGGER(DEBUG) << " ResampledImage: k =  " << k << " " << height << " " << line << " " << ymin + k << " " << image->height << std::endl;  
    
    resample_src_line(ymin + k);
    for(int x = 0; x < channels*width; x++) buffer[x] += resampled_src_line[x] * weights[k];
  }
  return width*channels;
}

template<class Kernel>
ResampledImage<Kernel>::~ResampledImage() {
  delete[] resampled_src_line;
  delete[] src_line; 
  delete[] float_buffer;
}


template class ResampledImage<Lanczos<2> >;
