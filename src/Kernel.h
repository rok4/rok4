#ifndef KERNEL_H
#define KERNEL_H

class NearestNeighbour;

#include <cmath>
#include "Logger.h"


template<int s>
class Lanczos {
  static double weight(double d) {
 //   LOGGER(DEBUG) << d << std::endl;
    if(d == 0.) return 1.;
    if(d < -s || d > s) return 0;
    d *= 3.14159265358979323846;
    return (sin(d) / d) * (sin(d/s) / d * s);
  }

  public:
  static double size(double ratio) {
    if(ratio < 1) ratio = 1;
    return s * ratio;
  }

  /*
   * W     : tableau de coefficients d'interpollation à calculer.
   * size  : taille max du tableau. Valeur modifiée en retour pour fixer le nombre de coefficient remplis dans W.
   * x     : valeur à interpoler
   * ratio : ratio d'interpollation. >1 sous-échantillonnage. <1 sur échantillonage.
   *
   * retour (xmin) : première valeur entière avec coefficient non nul.
   * W est rempli avec size coefficients le premier correspond à xmin, le dernier à xmin+size-1.
   */
  static int weight(float* W, int &size, double x, double ratio) {
//    LOGGER(DEBUG) << size << std::endl;
    if(ratio < 1) ratio = 1;
    double Ks = s*ratio;                // Taille du noyeau prenant compte du réchantillonnage.
    int xmin = ceil(x - Ks);            // Premier x aveca coeff non nul.    
    int xmax = floor(x + Ks);           // Dernier x avec coeff non nul.

//    double d = (xmin - x)/ratio;
//    double step = 1/ratio;
    while((W[0] = weight((double(xmin) - x)/ratio)) == 0) xmin++; // On incrémente xmin tant que son coeff est nul.
    for(int i = xmin+1; i <= xmax; i++) W[i-xmin] = weight((i - x)/ratio);   
    while(W[xmax - xmin] == 0) xmax--;         // On retire tous les coefficients nul en fin.    

//    LOGGER(DEBUG) << xmin << " " << x << " " << ratio << " " << xmax << " " << size << std::endl;
//    assert(xmin >= 0);
 //   assert(xmax - xmin < size);         // On vérifie que la tableau est assez grand.
    size = xmax - xmin + 1;             // On met à jour la taille de retour du tableau.
    double S = 0;
    for(int i = 0; i < size; i++) S += W[i];
    for(int i = 0; i < size; i++) W[i] /= S; // TODO: traiter les cas pathologiques (S = 0)

//    for(int i = 0; i <size; i++) LOGGER(DEBUG) << W[i] << std::endl;

    return xmin;
  }

};

#endif
