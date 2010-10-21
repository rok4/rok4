#ifndef COMPOUND_IMAGE_H
#define COMPOUND_IMAGE_H

#include "Image.h"
#include <vector>

class CompoundImage : public Image { 
  private:

  static int compute_width(std::vector<std::vector<Image*> > &images) {
    int width = 0;
    for(int x = 0; x < images[0].size(); x++) width += images[0][x]->width;
    return width;
  }

  static int compute_height(std::vector<std::vector<Image*> > &images) {
    int height = 0;
    for(int y = 0; y < images.size(); y++) height += images[y][0]->height;
    return height;
  }


  std::vector<std::vector<Image*> > images;

  /** Indice y des tuiles courrantes */
  int y;

  /** ligne correspondnat au haut des tuiles courrantes*/
  int top;

  template<typename T> 
  inline int _getline(T* buffer, int line) {
    // doit-on changer de tuile ?
    while(top + images[y][0]->height <= line) top += images[y++][0]->height;
    while(top > line) top -= images[--y][0]->height;
    // on calcule l'indice de la ligne dans la sous tuile
    line -= top; 
    for(int x = 0; x < images[y].size(); x++)
	buffer += images[y][x]->getline(buffer, line);
    return width*channels;
  }

  public:

  /** D */
  int getline(uint8_t* buffer, int line) { return _getline(buffer, line); }

  /** D */
  int getline(float* buffer, int line) { return _getline(buffer, line); }


  /** D */
  CompoundImage(std::vector< std::vector<Image*> >& images) :
        Image(compute_width(images), compute_height(images), images[0][0]->channels), 
        images(images),
        top(0),
        y(0) {}

  /** D */
  virtual ~CompoundImage() {
    for(int y = 0; y < images.size(); y++)
      for(int x = 0; x < images[y].size(); x++)
        delete images[y][x];
  }

};

#endif
