#ifndef EMPTY_IMAGE_H
#define EMPTY_IMAGE_H

#include "Image.h"

/**
 * Interface de base des classes Image.
 * Les implémentations définiront des images avec différentes structures de pixel
 * (type et nombre de canaux). Cette interface basique permet de définir des
 * fonctions prenant en paramètre tout type d'image.
 */
class EmptyImage : public Image {

  uint8_t *color;

  public:

  /** Constructeur */
  EmptyImage(int width, int height, int channels, uint8_t* _color) : Image(width, height, channels) {
    color = new uint8_t[channels];
    for(int c = 0; c < channels; c++) color[c] = _color[c];   
  }

  virtual int getline(uint8_t *buffer, int line) {
    for(int i = 0; i < width; i++) 
      for(int c = 0; c < channels; c++) 
        buffer[channels*i + c] = color[c];
  };
  virtual int getline(float *buffer, int line) {
    for(int i = 0; i < width; i++) 
      for(int c = 0; c < channels; c++) 
        buffer[channels*i + c] = color[c];
  };

  virtual ~EmptyImage() {
    delete[] color;
  };
};

#endif
