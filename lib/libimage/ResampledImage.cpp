/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière 
 * 
 * Géoportail SAV <geop_services@geoportail.fr>
 * 
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 * 
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * 
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

#include "ResampledImage.h"
#include "Logger.h"
#include "Utils.h"
#include <tiff.h>
#include <cmath>
#include <cstring>

ResampledImage::ResampledImage(Image *image, int width, int height, double resx, double resy, double lefttmp, double toptmp, double ratio_x, double ratio_y,  Interpolation::KernelType KT, BoundingBox<double> bbox) :
Image(width, height,resx,resy, image->channels, bbox), image(image) , left(lefttmp), top(toptmp), ratio_x(ratio_x), ratio_y(ratio_y), K(Kernel::getInstance(KT))
{
    left += 0.5*ratio_x - 0.5; // Pour prendre en compte que les échantillons 
    top  += 0.5*ratio_y - 0.5; // sont positionnés aux centres des pixels

    Kx = ceil(2 * K.size(ratio_x));
    Ky = ceil(2 * K.size(ratio_y));

    // nombre d'éléments d'une ligne de l'image source arrondie au multiple de 4 supérieur.
    int sz1 = 4*((image->width*channels + 3)/4);
    int sz1mask = 4*((image->width + 3)/4);
    
    // nombre d'éléments d'une ligne de l'image calculée arrondie au multiple de 4 supérieur.
    int sz2 = 4*((width*channels + 3)/4);
    int sz2mask = 4*((width + 3)/4);

    // nombre de poids dans Wx
    int sz3 = 4*width*Kx;
    int sz4 = 4*((width+3)/4);

    /* On veut mémoriser un certain nombre de lignes pour ne pas refaire un travail déjà fait.
     * On va travailler les lignes 4 par 4 (pour l'utilisation des instructions SSE). On va donc mémoriser
     * un multiple de 4 lignes.
     * Une ligne va intervenir au maximum dans l'interpolation de Ky lignes (diamètre du noyau d'interpolation)
     * Conclusion : on mémorise "Ky arrondi au multiple de 4 supérieur" lignes
     */
    memorize_line = 4*((Ky+3)/4);

    int sz = 8 * sz1 * sizeof(float)        // place pour src_line_buffer et mux_src_line_buffer;
      + 8 * sz1mask * sizeof(float)        // place pour src_mask_buffer et mux_src_mask_buffer;
      + sz2 * (memorize_line + 4 + 1) * sizeof(float)    // place pour memorize_line lignes de resampled_line + dst_line_buffer
      + sz2mask * (memorize_line + 4 + 1) * sizeof(float)    // place pour memorize_line lignes de resampled_mask + dst_weight_buffer
      + sz3 * sizeof(float)                 // place pour Wx
      + sz4 * sizeof(int);                  // place pour le tableau xmin


    /* Allocation de la mémoire pour tous les buffers en une seule fois :
     *  - gain de temps (l'allocation est une action qui prend du temps
     *  - tous les buffers sont côtes à côtes dans la mémoire, gain de temps lors des lectures/écritures
     */
    __buffer = (float*) _mm_malloc(sz, 16);  // Allocation allignée sur 16 octets pour SSE
    memset(__buffer, 0, sz);
    
    float* B = (float*) __buffer;

    // Lignes source d'image
    for(int i = 0; i < 4; i++) {
        src_line_buffer[i] = B; B += sz1;
    }
    mux_src_line_buffer = B; B += 4*sz1;

    // Lignes source de masque
    for(int i = 0; i < 4; i++) {
        src_mask_buffer[i] = B; B += sz1mask;
    }
    mux_src_mask_buffer = B; B += 4*sz1mask;

    // Ligne d'image rééchantillonnée
    resampled_line = new float*[Ky+4];
    resampled_line_index = new int[memorize_line];
    
    mux_resampled_line = B; B += 4*sz2;
    for(int i = 0; i < memorize_line; i++) {
            resampled_line[i] = B; B += sz2;
            resampled_line_index[i] = -1;
    }
    dst_line_buffer = B; B += sz2;
    weight_buffer = B; B += sz2mask;

    // Ligne de masque rééchantillonnée
    resampled_mask = new float*[Ky+4];
    mux_resampled_mask = B; B += 4*sz2mask;
    
    for(int i = 0; i < memorize_line; i++) {
            resampled_mask[i] = B; B += sz2mask;
    }

    Wx = B; B += sz3;
    xmin = (int*) B; B += sz4;

    memset(Wx, 0, sz3 * sizeof(float));
    float* W = Wx;
    for(int x = 0; x < width; x++) {
        int nb = Kx;
        xmin[x] = K.weight(W, nb, left + x * ratio_x, ratio_x, image->width);
        // On copie chaque poids en 4 exemplaires.
        for(int i = nb-1; i >= 0; i--) for(int j = 0; j < 4; j++) W[4*i + j] = W[i];
        W += 4*Kx;
    }
}



int ResampledImage::resample_src_line(int line)
{
    /* Comme on travaille les lignes 4 par 4, et qu'on en mémorise Ky+4 (une ligne étant utilisée pour
     * réechantillonner jusqu'à Ky lignes), on a potentiellement déjà calculé la ligne demandée
     * On vérifie dans le tableau des index si c'est le cas
     */
    if(resampled_line_index[line % memorize_line] == line) {
        return (line % memorize_line);
    }

    /* On va réechantillonner 4 lignes d'un coup. On commence par charger les 4 lignes de l'image source concernées
     * On vérifie bien que les 4 lignes existent bel et bien (qu'on dépasse pas la hauteur de l'image source)
     */
    for(int i = 0; i < 4; i++) {
        if(4*(line/4) + i < image->height) {
            image->getline(src_line_buffer[i], 4*(line/4) + i);
            if (image->getMask() == NULL) {
                for (int w = 0; w < image->width ; w++) {
                    src_mask_buffer[i][w] = 255.;
                }
                //memset(src_mask_buffer[i],254.,image->width*sizeof(float));
            } else {
                image->getMask()->getline(src_mask_buffer[i], 4*(line/4) + i);
            }
        }
    }

    /* Afin d'utiliser au mieux les instructions SSE, on "multiplexe" les 4 lignes, c'est-à dire que :
     *          - La colonne des 4 pixels sont à la suite dans le tableau
     *          - Les différents canaux ne sont plus entrelacés
     * 
     *    ligne 1      R1 G1 B1         R1 G1 B1        ...
     *    ligne 2      R2 G2 B2         R2 G2 B2        ...
     *    ligne 3      R3 G3 B3         R3 G3 B3        ...
     *    ligne 4      R4 G4 B4         R4 G4 B4        ...
     *                  | ---->
     *       on "lit" colonne par colonne
     *    Multiplexé = R1 R2 R3 R4 G1 G2 G3 G4 B1 B2 B3 B4 R1 R2 R3 R4 G1 G2 G3 G4 B1 B2 B3 B4 ...
     */
    multiplex(mux_src_line_buffer,
              src_line_buffer[0], src_line_buffer[1], src_line_buffer[2], src_line_buffer[3],
              image->width*image->channels);

    multiplex(mux_src_mask_buffer,
              src_mask_buffer[0], src_mask_buffer[1], src_mask_buffer[2], src_mask_buffer[3],
              image->width);

    for(int x = 0; x < width; x++) {
        dot_prod(channels, Kx,
                 mux_resampled_line + 4*x*channels,
                 mux_resampled_mask + 4*x,
                 mux_src_line_buffer + 4*xmin[x]*channels,
                 mux_src_mask_buffer + 4*xmin[x],
                 Wx + 4*Kx*x);

    }

    demultiplex(resampled_line[(4*(line/4)) % memorize_line],
                resampled_line[(4*(line/4)+1) % memorize_line],
                resampled_line[(4*(line/4)+2) % memorize_line],
                resampled_line[(4*(line/4)+3) % memorize_line],
                mux_resampled_line, width*channels);

    demultiplex(resampled_mask[(4*(line/4)) % memorize_line],
                resampled_mask[(4*(line/4)+1) % memorize_line],
                resampled_mask[(4*(line/4)+2) % memorize_line],
                resampled_mask[(4*(line/4)+3) % memorize_line],
                mux_resampled_mask, width);

    // Mise à jour des index des lignes mémorisées
    for(int i = 0; i < 4; i++) {
        resampled_line_index[(4*(line/4)+i) % memorize_line] = 4*(line/4)+i;
    }

    return (line % memorize_line);
}


int ResampledImage::getline(float* buffer, int line)
{
    float weights[Ky];
    float mask[width];
    int nb_weights = Ky;
    
    // On calcule les coefficient d'interpolation
    int ymin = K.weight(weights, nb_weights, top + line * ratio_y, ratio_y, image->height);

    int index = resample_src_line(ymin);
    //mult(buffer, weight_buffer, resampled_line[index], resampled_mask[index], weights[0], width, channels);
    mult(buffer, resampled_line[index], weights[0], width*channels);
    
    for(int y = 1; y < nb_weights; y++) {
        index = resample_src_line(ymin+y);
        //add_mult(buffer, weight_buffer, resampled_line[index], resampled_mask[index], weights[0], width, channels);
        add_mult(buffer, resampled_line[index], weights[y], width*channels);
    }
/*
    if (getMask()) {
        getMask()->getline(mask,line);
        for(int y = 0; y < width; y++) {
            if (mask[y] < 127.) weight_buffer[y] = 0.;
        }
        normalize(buffer, weight_buffer, width, channels);
    }*/

    
    return width*channels;
}

int ResampledImage::getline(uint8_t* buffer, int line) {
    int nb = getline(dst_line_buffer, line);
    convert(buffer, dst_line_buffer, nb);
    return nb;
}
