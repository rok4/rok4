#include "ResampledImage.h"
#include "Logger.h"
#include "Convert.h"
#include <cmath>
#include <cstring>

#include "SSE.h"

#include <mm_malloc.h>
#ifdef __SSE2__ 

#include <xmmintrin.h>
template<int C> 
inline void ResampledImage::interpolate() {
  __m128* S = (__m128*) mux_src_line_buffer;
  __m128* R = (__m128*) mux_resampled_line;

  for(int x = 0; x < width; x++) {
    __m128 W = _mm_set1_ps(Wx[x*Kx]);
    for(int c = 0; c < C; c++) R[x*C + c] = W * S[xmin[x]*C + c];
    for(int k = 1; k < Kx; k++) {
      W = _mm_set1_ps(Wx[x*Kx + k]);
      for(int c = 0; c < C; c++) R[x*C + c] += W * S[(xmin[x]+k)*C + c];
    }       
  }
}


#else // ifdef __SSE2__

 template<int C> 
 inline void ResampledImage::interpolate() {

  for(int x = 0; x < width; x++) {
    float w = Wx[x*Kx];    
    for(int c = 0; c < C; c++) for(int i = 0; i < 4; i++)
      mux_resampled_line[4*(x*C + c) + i] = w * mux_src_line_buffer[4*(xmin[x]*C + c) + i];
    for(int k = 1; k < Kx; k++) {
      w = Wx[x*Kx + k];      
      for(int c = 0; c < C; c++) for(int i = 0; i < 4; i++)
        mux_resampled_line[4*(x*C + c) + i] += w * mux_src_line_buffer[4*((xmin[x]+k)*C + c) + i];
    }       
  }
}


/*
float* ResampledImage::resample_src_line(int line) {

  int l = line % (Ky+4);
  if(resampled_line_index[l] == line) 
    return resampled_line[l];

  image->getline(src_line_buffer[0], line);
  memset(resampled_line[l], 0, width*channels*sizeof(float));

  for(int x = 0; x < width; x++) {    
    for(int c = 0; c < channels; c++) 
      for(int k = 0; k < Kx; k++) 
        resampled_line[l][channels*x + c] += Wx[x*Kx + k] * src_line_buffer[0][channels * (xmin[x] + k) + c];  
  }

  resampled_line_index[l] = line;
  return resampled_line[l];
}
*/



#endif // ifdef __SSE2__



ResampledImage::ResampledImage(Image *image, int width, int height, double left, double top, double ratio_x, double ratio_y,  Kernel::KernelType KT) :
  Image(width, height, image->channels), image(image) , left(left), top(top), ratio_x(ratio_x), ratio_y(ratio_y), K(Kernel::getInstance(KT)) {

    //  LOGGER_DEBUG( "ResampledImage => Constructeur ");

    Kx = ceil(2 * K.size(ratio_x));
    Ky = ceil(2 * K.size(ratio_y));

    int sz1 = 4*((image->width*channels + 3)/4);  // nombre d'éléments d'une ligne de l'image source arrondie au multiple de 4 supérieur.
    int sz2 = 4*((width*channels + 3)/4);         // nombre d'éléments d'une ligne de l'image calculée arrondie au multiple de 4 supérieur.


    int sz = 8 * sz1 * sizeof(float)             // place pour src_line_buffer;
           + sz2 * (Ky+4+4+1) * sizeof(float);   // place pour (Ky+4) lignes de resampled_src_line + dst_line_buffer


    resampled_line = new float*[Ky+4];
    resampled_line_index = new int[Ky+4];

    __buffer = (float*) _mm_malloc(sz, 16);  // Allocation allignée sur 16 octets pour SSE
    memset(__buffer, 0, sz);

    float* B = (float*) __buffer;     

    for(int i = 0; i < 4; i++) {
      src_line_buffer[i] = B; B += sz1;
    }
    mux_src_line_buffer = B; B += 4*sz1;
    mux_resampled_line = B; B += 4*sz2;


    for(int i = 0; i < Ky+4; i++) {
      resampled_line[i] = B; B += sz2;
      resampled_line_index[i] = -1;
    }
    dst_line_buffer = B; B += sz2;



    Wx = new float[Kx * width]; 
    xmin = new int[width];
    memset(Wx, 0, Kx * width * sizeof(float));
    for(int x = 0; x < width; x++) {
      int nb = Kx;
      xmin[x] = K.weight(Wx + x*Kx, nb, left + x * ratio_x, ratio_x);
    }
//  LOGGER_DEBUG( "ResampledImage Constructeur => ");

  }



float* ResampledImage::resample_src_line(int line) {
  if(resampled_line_index[line % (Ky+4)] == line) return resampled_line[line % (Ky+4)];

  for(int i = 0; i < 4; i++) if(4*(line/4) + i < image->height)
    image->getline(src_line_buffer[i], 4*(line/4) + i);

  multiplex(mux_src_line_buffer, src_line_buffer[0], src_line_buffer[1], src_line_buffer[2], src_line_buffer[3], image->width*image->channels);

  switch(channels) {
    case 1: interpolate<1>(); break;
    case 2: interpolate<2>(); break;
    case 3: interpolate<3>(); break;
    case 4: interpolate<4>(); break;
  }

  demultiplex(resampled_line[(4*(line/4))%(Ky+4)],
              resampled_line[(4*(line/4)+1)%(Ky+4)],
              resampled_line[(4*(line/4)+2)%(Ky+4)],
              resampled_line[(4*(line/4)+3)%(Ky+4)],
              mux_resampled_line, width*channels);

  for(int i = 0; i < 4; i++) resampled_line_index[(4*(line/4)+i)%(Ky+4)] = 4*(line/4)+i;
  return resampled_line[line % (Ky+4)];
}




float* ResampledImage::compute_dst_line(int line) {
  float weights[Ky];
  int nb_weights = Ky;
  int ymin = K.weight(weights, nb_weights, top + line * ratio_y, ratio_y); // On calcule les coefficient d'interpollation

  mult(dst_line_buffer, resample_src_line(ymin), weights[0], width*channels);
  for(int y = 1; y < nb_weights; y++) 
    add_mult(dst_line_buffer, resample_src_line(ymin + y), weights[y], width*channels);

  return dst_line_buffer;
}


int ResampledImage::getline(float* buffer, int line) {
//  LOGGER_DEBUG( "ResampledImage getline " );
  float* dst_line = compute_dst_line(line);
  memcpy(buffer, dst_line, width*channels*sizeof(float)); //TODO: éviter la copie intermédiaire.
  return width*channels;
}


int ResampledImage::getline(uint8_t* buffer, int line) {
    float* dst = compute_dst_line(line);
    convert(buffer, dst, width*channels);
    return width*channels;
  }


ResampledImage::~ResampledImage() {
//  LOGGER_DEBUG( "Destructeur ResampledImage" );
  _mm_free(__buffer);
  delete[] resampled_line;
  delete[] resampled_line_index;
  delete[] Wx;
  delete[] xmin;
  delete image; 
}
