#ifndef KERNEL_H
#define KERNEL_H

 /**
  * Classe mère définissant l'interface d'appel d'un noyau de rééchantillonnage en 1 dimension.
  *
  * Les classe filles implémentant un noyau particulier de rééchantillonnage auront 
  * à définir la taille du noyau (kernel_size), à déterminer si celle-ci dépend du ratio 
  * de rééchantillonage (const_ratio) et la fonction de poids (kernel_function)
  *
  * Les rééchantillonages en 2D sont effectués en échantillonnant en 1D selon chaque dimension.
  */
class Kernel {
  private:

  float coeff[1025];

  /**
   * Taille du noyau en nombre de pixels pour un ratio de 1.
   * Pour calculer la valeur d'un pixel rééchantillonné x les pixels
   * sources entre x - kernel_size et x + kernel_size seront utilisés.
   */
  const double kernel_size;

  /**
   * Détermine si le ratio de rééchantillonnage influe sur la taille du noyau.
   *
   * Si const_ratio = true, la taille du noyau est kernel_size
   * Si const_ratio = false la taille du noyeau est 
   *    - kernel_size pour ratio <= 1
   *    - kernel_size * ratio pour ratio > 1
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
   * Cette fonction doit typiquement être apellée à la fin du constructeur des classe filles 
   * (pour des raisons d'ordre d'initialisation des instances mère/fille).
   */
  void init() {
    for(int i = 0; i <= 1024; i++) coeff[i] = kernel_function(i * kernel_size / 1024.);
  }

  /**
   * Constructeur de la classe mère
   * Ne peux être appelé que par les constructeurs des classes filles.
   */
  Kernel(double kernel_size, bool const_ratio = false) : kernel_size(kernel_size), const_ratio(const_ratio) {}

  public:

  /**
   * Type définissant les différentes méthodes de d'interpolation.
   *
   *
   */
  typedef enum {UNKNOWN = 0, NEAREST_NEIGHBOUR = 1, LINEAR = 2, CUBIC = 3, LANCZOS_2 = 4, LANCZOS_3 = 5, LANCZOS_4 = 6} KernelType;
  
  /**
   * Factory permettant d'obtenir une instance d'un type de noyau donné.
   */
  static const Kernel& getInstance(KernelType T = LANCZOS_3);

  /**
   * Calcule la taille du noyau en nombre de pixels sources requis sur une dimension
   * en fonction du ratio de rééchantillonage.
   *
   *
   * @param ratio Ratio d'interpollation. >1 sous-échantillonnage. <1 sur échantillonage.
   *        ratio = resolution source / résolution cible.
   *
   * @return nombre de pixels (non forcément entiers) requis. Pour interpoler 
   *           la coordonnées x, les pixels compris entre x-size et x+size seront utlisés.
   */
  inline double size(double ratio = 1.) const {
      if(ratio <= 1 || const_ratio) return kernel_size;
      else return kernel_size * ratio;
  }

  /**
   * Fonction calculant les poids à appliquer aux pixels sources en fonction
   * du centre du pixel à calculer et du ratio de réchantillonage
   *
   * @param W Tableau de coefficients d'interpolation à calculer.
   * @param length Taille max du tableau. Valeur modifiée en retour pour fixer le nombre de coefficient remplis dans W.
   * @param x Valeur à interpoler
   * @param ratio Ratio d'interpollation. >1 sous-échantillonnage. <1 sur échantillonage.
   *
   * @return xmin : première valeur entière avec coefficient non nul. le paramètre length est modifié pour
   * indiquer le nombre réel de coefficients écrits dans W.
   */
  int weight(float* W, int &length, double x, double ratio) const;

};




#endif
