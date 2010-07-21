#ifndef COMPOUND_IMAGE_H
#define COMPOUND_IMAGE_H

#include "GeoreferencedImage.h"
#include <vector>
#include <cstring>
#include "Logger.h"

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

class ExtendedCompoundImage : public GeoreferencedImage { 
  private:

  std::vector<GeoreferencedImage*> images;

  template<typename T>
  int _getline(T* buffer, int line) {

   double y=l2y(line);
   for (int i=0;i<images.size()-1;i++)
   {
        if (images[i]->getymin()>y||images[i]->getymax()<y)
                continue;
        else if (images[i]->getxmin()>getxmax()||images[i]->getxmax()<getxmin())
                continue;

        int c0=__max(0,x2c(images[i]->getxmin()));
        int c1=__min(width,x2c(images[i]->getxmax()));
        T* buffer_t = new T[images[i]->width*images[i]->channels];
        images[i]->getline(buffer_t,images[i]->y2l(y));
        memcpy(&buffer[c0],&buffer_t[__min(0,-x2c(images[i]->getxmin()))],(c1-c0)*channels);
        delete [] buffer_t;
   }
    return width*channels;
  }

  public:

  /** D */
  int getline(uint8_t* buffer, int line) { return _getline(buffer, line); }

  /** D */
  int getline(float* buffer, int line) { return _getline(buffer, line); }


  /** D */
  ExtendedCompoundImage(int width, int height, int channels, double x0, double y0, double resx, double resy, std::vector<GeoreferencedImage*>& images) :
        GeoreferencedImage(width, height, channels,x0,y0,resx,resy), 
        images(images) {}

  /** D */
  virtual ~ExtendedCompoundImage() {
	LOGGER_DEBUG("Destructeur ExtendedCompoundImage");
     /* for(int i = 0; i < images.size(); i++)
        delete images[i];*/
	LOGGER_DEBUG("Fin Destructeur ExtendedCompoundImage");
  }

};

#endif
