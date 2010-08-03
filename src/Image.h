#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

/**
 * Interface de base des classes Image.
 * Les implémentations définiront des images avec différentes structures de pixel
 * (type et nombre de canaux). Cette interface basique permet de définir des
 * fonctions prenant en paramètre tout type d'image.
 */
class Image {
  public:
  /** Largeur de l'image en pixels */
  const int width;
  
  /** Hauteur de l'image en pixels */
  const int height;

  /** Nombre de canaux de l'image */
  const int channels;

  /** Constructeur */
  Image(int width, int height, int channels) :
        width(width), height(height), channels(channels) {}
  /** 
   * Retourne une ligne en entier 8 bits. 
   * Les canaux sont entrelacés. Si les données ne sont pas intrinsèquement codées sur des entiers 8 bits
   * une conversion est effectuée.
   *
   * @param buffer Tableau contenant au moins width*channels octets
   * @param line Indice de la ligne à retourner (0 <= line < height)
   *
   * @return
   *   - Nombre d'éléments effectivement copiés dans buffer en cas de succès
   *   - 0 en cas d'échec
   */
  virtual int getline(uint8_t *buffer, int line) = 0;

  /** 
   * Retourne une ligne en float 32 bits.
   * Les canaux sont entrelacés. Si les données ne sont pas intrinsèquement codées sur des flotants 32 bits
   * une conversion est effectuée.
   *
   * @param buffer Tableau contenant au moins width*channels float
   * @param line Indice de la ligne à retourner (0 <= line < height)
   *
   * @return 
   *   - Nombre d'éléments effectivement copiés dans buffer en cas de succès
   *   - 0 en cas d'échec
   */
  virtual int getline(float *buffer, int line) = 0;

  /** 
   * Destructeur virtuel. Permet de lancer les destructeurs des classes filles
   * lors de la destruction d'un pointeur Image.
   */
  virtual ~Image() {};
};

#endif
