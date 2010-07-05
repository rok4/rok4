#ifndef KERNEL_H
#define KERNEL_H

class NearestNeighbour;

#include <cmath>
#include <string>
#include "Logger.h"

 /**
  * Classe 
  *
  * Les classe filles implémentant un noyau particulier de rééchantillonnage auront 
  * à définir la taille du noyau (kernel_size), à déterminer si celle-ci dépend du ratio 
  * de rééchantillonage (const_ratio) et la fonction de poids (kernel_function)
  */
class Kernel {
  private:

  float coeff[1025];

  /**
   * Taille du noyau en nombre de pixels pour un ratio de 1.
   * Pour calculer la valeur d'un pixel rééchantillonné x les pixels
   * sources entre x - kernel_sie et x + kernel_size seront utilisés.
   */
  const double kernel_size;

  /**
   * Détermnie si le ratio de rééchantillonnage influe sur la taille du noyau.
   *
   * Si const_ratio = true, la taille du noyau est kernel_size
   * Si const_ratio = false la taille du noyeau est 
   *    - kernel_size pour ratio <= 1
   *    - kenel_size * ratio pour ratio > 1
   */
  const bool const_ratio;

  /**
   * Fonction du noyau définissant le poid d'un pixel en fonction de sa distance d au
   * pixel rééchantillonné.
   */
  virtual double kernel_function(double d) = 0;

  protected:

  /**
   * Initialise le tableau des coefficient avec en échantillonnant la fonction kernel_function.
   */
  void init() {
    for(int i = 0; i <= 1024; i++) 
      coeff[i] = kernel_function(i * kernel_size / 1024.);
  }

  Kernel(double kernel_size, bool const_ratio = false) : kernel_size(kernel_size), const_ratio(const_ratio) {}

  public:

  static const Kernel& getInstance(std::string method);

  inline double size(double ratio = 1.) const {
      if(ratio <= 1 || const_ratio) return kernel_size;
      else return kernel_size * ratio;
  }


 /*
   * W     : tableau de coefficients d'interpollation à calculer.
   * length  : taille max du tableau. Valeur modifiée en retour pour fixer le nombre de coefficient remplis dans W.
   * x     : valeur à interpoler
   * ratio : ratio d'interpollation. >1 sous-échantillonnage. <1 sur échantillonage.
   *
   * retour (xmin) : première valeur entière avec coefficient non nul.
   * W est rempli avec size coefficients le premier correspond à xmin, le dernier à xmin+size-1.
   */
  int weight(float* W, int &length, double x, double ratio) const {
//    LOGGER(DEBUG) << size << std::endl;
//    if(ratio < 1) ratio = 1;
    double Ks = size(ratio);                  // Taille du noyeau prenant compte le ratio du réchantillonnage.
    int xmin = ceil(x - Ks + 1e-9);          // Premier x avec coeff non nul.    

    double sum = 0;
    double step = 1024. / Ks;
    double indf = (x - xmin) * step;

    int i = 0;
    for(;indf >= 0 && i < length; indf -= step) {
      int ind = (int) indf;
      sum += W[i++] = coeff[ind] + (coeff[ind+1] - coeff[ind]) * (indf - ind);
    }
    for(indf = -indf; indf < 1024. && i < length; indf += step) {
      int ind = (int) indf;
      sum += W[i++] = coeff[ind] + (coeff[ind+1] - coeff[ind]) * (indf - ind);
    }
    length = i;
    while(i--) W[i] /= sum;
   // LOGGER_DEBUG(" ");
    return xmin;
  }

};




#endif
