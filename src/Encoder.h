#ifndef _ENCODER_
#define _ENCODER_

#include "Image.h"
#include "HttpResponse.h"
#include "Tile.h"
#include "pixel_type.h"
#include "Error.h"

class TIFF {
  public:
  template<class pixel_t> 
  static TiffEncoder<typename pixel_t::tiff_t>* encode(Image<pixel_t> *image) {
    return new TiffEncoder<typename pixel_t::tiff_t>(transform<pixel_t, typename pixel_t::tiff_t>(image));
  }
};

class JPEG {
  public:
  template<class pixel_t> 
  static JPEGEncoder<typename pixel_t::jpeg_t>* encode(Image<pixel_t> *image) {
    return new JPEGEncoder<typename pixel_t::jpeg_t>(transform<pixel_t, typename pixel_t::jpeg_t>(image));
  }
};

class PNG {
  public:
  template<class pixel_t> 
  static PNGEncoder<typename pixel_t::png_t>* encode(Image<pixel_t> *image) {
    return new PNGEncoder<typename pixel_t::png_t>(transform<pixel_t, typename pixel_t::png_t>(image));
  }
};

class BIL {
  public:
  template<class pixel_t>
  static BilEncoder<pixel_t>* encode(Image<pixel_t> *image) {
    return new BilEncoder<pixel_t>(image);
  }
};


/*
 * Pas de Style
 */
template<class pixel_t, class Encoder>
HttpResponse* NoStyle(Image<pixel_t> *image, bool transparent) {
  if(transparent) return Encoder::encode(transform_transparent(image));
  else            return Encoder::encode(transform_opaque(image));
}

template<>
HttpResponse* NoStyle<pixel_gray, PNG>(Image<pixel_gray> *image, bool transparent) {
  if(transparent) return new ColorizePNGEncoder(image, true);
  else            return PNG::encode(image);
}

template<>
HttpResponse* NoStyle<pixel_float, PNG>(Image<pixel_float> *image, bool transparent) {
             return BIL::encode(image);
}

/*
 * Style Gris
 */

  template<class pixel_t, class Encoder>
  HttpResponse* GrayStyle(Image<pixel_t> *image, bool transparent, const char* style) {
    assert(style);
    assert(strncmp(style, "gray", 4) == 0);
    
    float coeff[pixel_t::channels];             // valeur par défaut des coefficients
    memcpy(coeff, TransformConstants<pixel_t,pixel_gray>::coeff, sizeof(coeff));

    if(style[4] == '(') { // lecture des coeff en paramètre si présents
      for(int c = 0, pos = 5; style[pos] && style[pos] != ')' && c < pixel_t::channels; c++) {        
        coeff[c] = atof(style + pos);
        for(; style[pos] && style[pos] != ')' && style[pos] != ','; pos++); if(style[pos] == ',') pos++;
      }
    }
    return NoStyle<pixel_gray, Encoder>(transform<pixel_t, pixel_gray>(image, coeff), transparent);
  }

/*
 * Style RGB
 */

template<class pixel_t, class Encoder>
HttpResponse* RGBStyle(Image<pixel_t> *image, bool transparent, const uint8_t rgb[3]) {
  return Encoder::encode(transform<pixel_t, pixel_rgb>(image));
}

template<class pixel_t, class Encoder>
HttpResponse* RGBStyle(Image<pixel_t> *image, bool transparent, const char* style) {
  assert(style);
  assert(strncmp(style, "rgb(", 4) == 0);
  uint8_t rgb[3] = {0, 0, 0};
  for(int c = 0, pos = 4; style[pos] && style[pos] != ')' && c < 3; c++) {
    int value = atoi(style + pos);
    if(value >= 0 && value < 256) rgb[c] = value;
    for(; style[pos] && style[pos] != ')' && style[pos] != ','; pos++); if(style[pos] == ',') pos++;
  }
  return RGBStyle<pixel_t, Encoder>(image, transparent, rgb);
}


template<> HttpResponse* RGBStyle<pixel_rgb, PNG>(Image<pixel_rgb> *image, bool transparent, const uint8_t rgb[3]) {
  return new ColorizePNGEncoder(transform<pixel_rgb, pixel_gray>(image), transparent, rgb);
}

template<> HttpResponse* RGBStyle<pixel_gray, PNG>(Image<pixel_gray> *image, bool transparent, const uint8_t rgb[3]) {
  return new ColorizePNGEncoder(image, transparent, rgb); 
}


/*
 *
 *
 */

template<class pixel_t, class Encoder> 
HttpResponse* encodeImage(Image<pixel_t> *image, bool transparent, const char* style) {
  if(!style)                              return NoStyle  <pixel_t, Encoder>(image, transparent);
  else if(strncmp(style, "gray", 4) == 0) return GrayStyle<pixel_t, Encoder>(image, transparent, style);
  else if(strncmp(style, "rgb(", 4) == 0) return RGBStyle <pixel_t, Encoder>(image, transparent, style);
  else                                    return NoStyle  <pixel_t, Encoder>(image, transparent);
}

template<class pixel_t>
HttpResponse* encodeImage(Image<pixel_t> *image, const char* format = 0, bool transparent = true, const char* style = 0) {
  if(image == 0) return 0;
  if(!format)                                     return encodeImage<pixel_t, TIFF>(image, transparent, style);
  else if(strncmp(format, "image/tiff", 10) == 0) return encodeImage<pixel_t, TIFF>(image, transparent, style);
  else if(strncmp(format, "image/png" ,  9) == 0) return encodeImage<pixel_t, PNG >(image, transparent, style);
  else if(strncmp(format, "image/jpeg", 10) == 0) return encodeImage<pixel_t, JPEG>(image, false      , style);
  else if(strncmp(format, "image/bil",   9) == 0) return encodeImage<pixel_t, BIL >(image, false      , style);
  else {
    delete image;
    return new Error("Unsupported format");
  }
}




#endif
