#ifndef LIBTIFF_IMAGE_H
#define LIBTIFF_IMAGE_H

#include "GeoreferencedImage.h"
#include "tiffio.h"

class LibtiffImage : public GeoreferencedImage {

  friend class libtiffImageFactory;

  private:
  TIFF* tif;

  protected:
  /** D */
  LibtiffImage(int width, int height, int channels, double x0, double y0, double resx, double resy, TIFF* tif);

  public:

  /** D */
  int getline(uint8_t* buffer, int line);

  /** D */
  int getline(float* buffer, int line);

  /** D */
  ~LibtiffImage();
};

class libtiffImageFactory {
  public:
        LibtiffImage* createLibtiffImage(char* filename);
};


#endif

