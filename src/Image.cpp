#include "Image.h"


/**
 * A Optimiser
 *
int Image::getline(uint8_t* buffer, int line) {
  float* _buffer = new float[width*channels];
  getline(_buffer, line);
  for(int i = 0; i < width*channels; i++) buffer[i] = (uint8_t) _buffer[i];
  delete[] _buffer;
  return 1;
}

/**
 * A Optimiser
 *
int Image::getline(float* buffer, int line) {
  float* _buffer = new float[width*channels];
  getline(_buffer, line);
  for(int i = 0; i < width*channels; i++) buffer[i] = (float) _buffer[i];
  delete[] _buffer;
  return 1;
}
*/

