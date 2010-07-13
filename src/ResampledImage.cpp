#include "ResampledImage.h"
#include "Logger.h"
#include "Convert.h"
#include <cstring>



ResampledImage::ResampledImage(Image *image, int width, int height, double left, double top, double ratio_x, double ratio_y) :
  Image(width, height, image->channels), image(image) , left(left), top(top), ratio_x(ratio_x), ratio_y(ratio_y), K(Kernel::getInstance("lanczos_2")) {

  LOGGER_DEBUG( "ResampledImage => Constructeur ");

    Kx = ceil(2 * K.size(ratio_x));
    Ky = ceil(2 * K.size(ratio_y));

    int sz1 = 4*((image->width*channels + 3)/4);  // nombre d'éléments d'une ligne de l'image source arrondie au multiple de 4 supérieur.
    int sz2 = 4*((width*channels + 3)/4);         // nombre d'éléments d'une ligne de l'image calculée arrondie au multiple de 4 supérieur.

    int sz = sz1 * sizeof(float)             // place pour src_line_buffer;
           + sz2 * (Ky+5) * sizeof(float);   // place pour (Ky+4) lignes de resampled_src_line + dst_line_buffer


    resampled_line = new float*[Ky+4];
    resampled_line_index = new int[Ky+4];

    __buffer = (float*) _mm_malloc(sz, 16);  // Allocation allignée sur 16 octets pour SSE
    memset(__buffer, 0, sz);

    float* B = (float*) __buffer;     
    src_line_buffer = B; B += sz1;

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
  LOGGER_DEBUG( "ResampledImage Constructeur => ");

  }


float* ResampledImage::resample_src_line(int line) {

  int l = line % (Ky+4);
  if(resampled_line_index[l] == line) 
    return resampled_line[l];

  image->getline(src_line_buffer, line);
  memset(resampled_line[l], 0, width*channels*sizeof(float));

  for(int x = 0; x < width; x++) {    
    for(int c = 0; c < channels; c++) 
      for(int k = 0; k < Kx; k++) 
        resampled_line[l][channels*x + c] += Wx[x*Kx + k] * src_line_buffer[channels * (xmin[x] + k) + c];  
  }

  return resampled_line[l];
}





float* ResampledImage::compute_dst_line(int line) {
  float weights[Ky];
  int nb_weights = Ky;
  int ymin = K.weight(weights, nb_weights, top + line * ratio_y, ratio_y); // On calcule les coefficient d'interpollation

  int L = ((width*channels + 3)/4); 
  __m128 *DST = (__m128*) dst_line_buffer;
  __m128* SRC = (__m128*) resample_src_line(ymin);
  __m128 w = _mm_set1_ps(weights[0]);  
  for(int i = 0; i < L; i++) DST[i] = w * SRC[i];

  for(int y = 1; y < nb_weights; y++) {
    SRC = (__m128*) resample_src_line(ymin + y);
    w = _mm_set1_ps(weights[y]);
    for(int i = 0; i < L; i++) DST[i] += w * SRC[i];
  }
  return dst_line_buffer;
}


int ResampledImage::getline(float* buffer, int line) {
  LOGGER_DEBUG( "ResampledImage getline " );
  float* dst_line = compute_dst_line(line);
  memcpy(buffer, dst_line, width*channels*sizeof(float)); //TODO: éviter la copie intermédiaire.
  return width*channels;
}




ResampledImage::~ResampledImage() {
  LOGGER_DEBUG( "Destructeur ResampledImage" );
  _mm_free(__buffer);
  delete[] resampled_line;
  delete[] resampled_line_index;
  delete[] Wx;
  delete[] xmin;
  delete image; 
}
