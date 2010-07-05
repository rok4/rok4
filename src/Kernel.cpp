

#include "Kernel.h"




template<int s>
class Lanczos : public Kernel {
  friend const Kernel& Kernel::getInstance(std::string);

  private:
   double kernel_function(double d) {
    if(d > s) return 0.;
    else if(d == 0.) return 1.;
    else {
      d *= 3.14159265358979323846;
      return (sin(d) / d) * (sin(d/s) / d * s);
    }
  }

  Lanczos() : Kernel(s) {init();}
};


class NearestNeighbour : public Kernel {
  friend const Kernel& Kernel::getInstance(std::string);
  private:
   double kernel_function(double d) {
     if(d > 0.5) return 0.;
     else return 1.;
  }
  NearestNeighbour() : Kernel(0.5, true) {init();}
};


class Linear : public Kernel {
  friend const Kernel& Kernel::getInstance(std::string);
  private:
   double kernel_function(double d) {
     if(d > 1) return 0.;
     else return 1.-d;
  }
  Linear() : Kernel(1.) {init();}
};


 /*
  * Pris dans Image Magick
  *
    Cubic Filters using B,C determined values:

    Catmull-Rom         B= 0  C=1/2   Cublic Interpolation Function

    Coefficents are determined from B,C values
       P0 = (  6 - 2*B       )/6     = 1
       P1 =         0                = 0
       P2 = (-18 +12*B + 6*C )/6     = -5/2
       P3 = ( 12 - 9*B - 6*C )/6     = 3/2
       Q0 = (      8*B +24*C )/6     = 2
       Q1 = (    -12*B -48*C )/6     = -4
       Q2 = (      6*B +30*C )/6     = 5/2
       Q3 = (    - 1*B - 6*C )/6     = -1/2

    Which is used to define the filter...
       P0 + P1*x + P2*x^2 + P3*x^3      0 <= x < 1
       Q0 + Q1*x + Q2*x^2 + Q3*x^3      1 <= x <= 2

    Which ensures function is continuous in value and derivative (slope).
  */


class CatRom : public Kernel {
  friend const Kernel& Kernel::getInstance(std::string);
  private:
   double kernel_function(double d) {
     if(d > 2) return 0.;
     else if(d > 1) return 2. + d*(-4. + d*(2.5 - 0.5*d));
     else return 1. + d*d*(1.5*d - 2.5);
  }
  CatRom() : Kernel(2.) {init();}
};



const Kernel& Kernel::getInstance(std::string method) {
  static Lanczos<2> lanczos_2;
  static Lanczos<3> lanczos_3;
  static Lanczos<4> lanczos_4;
  static NearestNeighbour nearest_neighbour;
  static Linear linear;
  static CatRom catrom;
  return lanczos_4;
}


