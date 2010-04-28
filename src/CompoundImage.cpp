#include "CompoundImage.h"
#include "Logger.h"


namespace CI {
  int width(std::vector<std::vector<Image*> > &images) {
    int width = 0;
    for(int x = 0; x < images[0].size(); x++) width += images[0][x]->width;
    return width;
  }

  int height(std::vector<std::vector<Image*> > &images) {
    int height = 0;
    for(int y = 0; y < images.size(); y++) height += images[y][0]->height;
    return height;
  }
}



CompoundImage::CompoundImage(std::vector<std::vector<Image*> >& images) 
: Image(CI::width(images), CI::height(images), images[0][0]->channels), images(images) {}



int CompoundImage::getline(uint8_t *buffer, int line) {
  int y = 0; 
  for(; line >= images[y][0]->height; y++) // TODO : faire plus efficace que temps linéaire
    line -= images[y][0]->height;

  for(int x = 0; x < images[y].size(); x++) 
    buffer += images[y][x]->getline(buffer, line);

  return width*channels;
}

int CompoundImage::getline(float *buffer, int line) {
  int y = 0; 
  for(; line >= images[y][0]->height; y++) // TODO : faire plus efficace que temps linéaire
    line -= images[y][0]->height;

  for(int x = 0; x < images[y].size(); x++) 
    buffer += images[y][x]->getline(buffer, line);

  return width*channels;
}


CompoundImage::~CompoundImage() {
  for(int y = 0; y < images.size(); y++)
    for(int x = 0; x < images[y].size(); x++)
      delete images[y][x];

  LOGGER(DEBUG) << " Destructeur CompoundImage"  << std::endl;  
}

