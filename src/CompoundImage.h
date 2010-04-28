#ifndef COMPOUND_IMAGE_H
#define COMPOUND_IMAGE_H

#include "Image.h"
#include "Tile.h"
#include <vector>

/**
 * Interface de base des classes Image.
 * Les implémentations définiront des images avec différents strcuture de pixel
 * (type et nombre de canaux). Cette interface basique permet de définir des
 * fonctions prenant en paramètre tout type d'image.
 */
class CompoundImage : public Image {
  private:
  std::vector<std::vector<Image*> > images;


  public:

  /** D */
  int getline(uint8_t* buffer, int line);

  /** D */
  int getline(float* buffer, int line);


  /** D */
  CompoundImage(std::vector<std::vector<Image*> >& images);
  
  /** D */
  ~CompoundImage();
};

#endif
