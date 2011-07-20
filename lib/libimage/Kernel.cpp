#include <cmath>
#include "Kernel.h"

int Kernel::weight(float* W, int &length, double x, double ratio) const {
    double Ks = size(ratio);                  // Taille du noyau prenant compte le ratio du réchantillonnage.
    double step = 1024. / Ks;         
    int xmin = ceil(x - Ks + 1e-7); 
    if(length < 2*Ks) xmin = ceil(x - length*0.5 + 1e-9);
    double sum = 0;                           // somme des poids pour normaliser en fin de calcul.
    double indf = (x - xmin) * step;          // index flottant dans le tableau coeff 

    int i = 0;
    for(;indf >= 0; indf -= step) {
      int ind = (int) indf;
      sum += W[i++] = coeff[ind] + (coeff[ind+1] - coeff[ind]) * (indf - ind);
    }
    for(indf = -indf; indf < 1024. && i < length; indf += step) {
      int ind = (int) indf;
      sum += W[i++] = coeff[ind] + (coeff[ind+1] - coeff[ind]) * (indf - ind);
    }
    length = i;
    while(i--) W[i] /= sum;     // On normalise pour que la somme des poids fasse 1.
    return xmin;
  }



template<int s>
class Lanczos : public Kernel {
  friend const Kernel& Kernel::getInstance(KernelType T);

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
  friend const Kernel& Kernel::getInstance(KernelType T);
  private:
   double kernel_function(double d) {
     if(d > 0.5) return 0.;
     else return 1.;
  }
  NearestNeighbour() : Kernel(0.5, true) {init();}
};


class Linear : public Kernel {
  friend const Kernel& Kernel::getInstance(KernelType T);
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
  friend const Kernel& Kernel::getInstance(KernelType T);
  private:
   double kernel_function(double d) {
     if(d > 2) return 0.;
     else if(d > 1) return 2. + d*(-4. + d*(2.5 - 0.5*d));
     else return 1. + d*d*(1.5*d - 2.5);
  }
  CatRom() : Kernel(2.) {init();}
};



const Kernel& Kernel::getInstance(KernelType T) {
  static NearestNeighbour nearest_neighbour;
  static Linear linear;
  static CatRom catrom;
  static Lanczos<2> lanczos_2;
  static Lanczos<3> lanczos_3;
  static Lanczos<4> lanczos_4;

  switch(T) {
    case NEAREST_NEIGHBOUR: return nearest_neighbour; break;
    case LINEAR: return linear; break;
    case CUBIC: return catrom; break;
    case LANCZOS_2: return lanczos_2; break;
    case LANCZOS_3: return lanczos_3; break;
    case LANCZOS_4: return lanczos_4; break;
  }
  return lanczos_3;
}


