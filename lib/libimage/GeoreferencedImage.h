#ifndef GEOREFERENCEDIMAGE_IMAGE_H
#define GEOREFERENCEDIMAGE_IMAGE_H

#include "Image.h"
#include "tiffio.h"

class GeoreferencedImage : public Image {
  private:
  double x0;
  double y0;
  double resx;
  double resy;

  public:

  /** D */
  int getline(uint8_t* buffer, int line) {return 1;}

  /** D */
  int getline(float* buffer, int line) {return 1;}

  /** D */
  GeoreferencedImage(int width, int height, int channels, double x0, double y0, double resx, double resy)
  : Image(width,height,channels) , x0(x0), y0(y0), resx(resx), resy(resy)
  {}

  /** D */
  ~GeoreferencedImage() {}

  double inline getxmin() {return x0;}
  double inline getymax() {return y0;}
  double inline getxmax() {return x0+width*resx;}
  double inline getymin() {return y0-height*resy;}
  double inline getresx() {return resx;}
  double inline getresy() {return resy;}

  int inline x2c(double x) {return (int)((x-x0)/resx);}
  int inline y2l(double y) {return (int)((y0-y)/resy);}
  double inline c2x(int c) {return (x0+c*resx);}
  double inline l2y(int l) {return (y0-l*resy);}
};


#endif

