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

/**
 * \file ResampledImage.h
 ** \~french
 * \brief Définition de la classe ResampledImage, permettant le réechantillonnage d'image
 ** \~english
 * \brief Define class ResampledImage, allowing image resampling
 */

#ifndef RESAMPLED_IMAGE_H
#define RESAMPLED_IMAGE_H

#include "Image.h"
#include "Kernel.h"
#include "Interpolation.h"
#include <mm_malloc.h>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'image réechantillonnée
 * \details Cette classe permet de réechantillonner une image lorsque celle ci n'a pas la résolution ou la phase voulue. Pour calculer la valeur d'un pixel de l'image réechantillonnée, on peut utiliser plusieurs interpolations, implémentées dans les différentes classes héritant de Kernel. Le réechantillonnage est effectué séparemment dans chaque dimension (X et Y).
 *
 * \~ \image html ri_2D.png \~french
 *
 * Les calculs étant lourds, on va optimiser le calcul :
 * \li en mémorisant, pour éviter de calculer deux fois la même chose
 * \li en allouant en une seule fois tout l'espace nécessaire au calcul
 * \li en utilisant au maximum les instructions SSE pour les calculs vectoriels
 *
 * Pour augmenter les performances du réechantillonnage avec les instructions SSE, on est amené :
 * \li à calculer les lignes 4 par 4
 * \li à travailler sur des buffers dont la taille est un multiple de 4
 * \li à travailler avec des données multiplexée : on regroupe les même canaux des différentes lignes
 *
 * On peut également tenir compte du masque associé à l'image source, pour limiter l'interpolation aux valeurs réelles. Cela ajoute non seulement de la complexité aux calculs, mais prend également plus de place. On va donc limiter cette utilisation aux cas vraiment nécessaires : si l'image source possède un masque (image pas pleine) et si l'utilisateur spécifie qu'il veut l'utiliser dans le réechantillonnage.
 *
 * Enfin, lors du réechantillonnage, on ne tient pas compte du propre masque. C'est à dire qu'on remplit un pixel réechantillonné avec de la donnée à partir du moment où un pixel de donnée source appartenait au noyau d'interpolation. Si on veut utiliser cette image sans avoir un "gonflement" artificiel des données, on devra la lire en parallèle de son masque (interpolé en plus proche voisin) pour la restreindre à l'étendue réelle des données (cela peut se faire avec ExtendedCompoundImage).
 */
class ResampledImage : public Image {
private:
    /**
     * \~french \brief Image source, à réechantillonner
     * \~english \brief Source image, to resample
     */
    Image* sourceImage;

    /**
     * \~french \brief Précise si les masques doivent intervenir dans l'interpolation (lourd)
     * \~english \brief Precise if mask have to be used by interpolation (heavy)
     */
    bool useMask;

    /**
     * \~french \brief Noyau d'interpolation à utiliser
     * \~english \brief Interpolation kernel to use
     */
    const Kernel& K;

    /**
     * \~french \brief Nombre de pixels source intervenant dans l'interpolation, dans le sens des X
     * \~english \brief Number of source pixels used by interpolation, widthwise
     */
    int Kx;
    /**
     * \~french \brief Nombre de pixels source intervenant dans l'interpolation, dans le sens des Y
     * \~english \brief Number of source pixels used by interpolation, heightwise
     */
    int Ky;

    /**
     * \~french \brief Rapport des résolutions source et finale, dans le sens des X
     * \details Ratio de rééchantillonage en X = résolution X cible / résolution X source
     * \~english \brief Ratio between destination resolution and source resolution, widthwise
     * \details X ratio = X destination resolution / X source resolution
     */
    double ratioX;
    /**
     * \~french \brief Rapport des résolutions source et finale, dans le sens des Y
     * \details Ratio de rééchantillonage en Y = résolution Y cible / résolution Y source
     * \~english \brief Ratio between destination resolution and source resolution, heighthwise
     * \details Y ratio = Y destination resolution / Y source resolution
     */
    double ratioY;

    /**
     * \~french \brief Décalage entre le haut de l'image source et le haut de l'image réechantillonnée
     * \details La distance est exprimée en pixel source, et se mesure entre les centres des pixels.
     * \~english \brief Offset between source image's top and resmapled image's top
     * \details Distance is expressed as source pixel and between pixels' centers.
     */
    double top;
    /**
     * \~french \brief Décalage entre la gauche de l'image source et la gauche de l'image réechantillonnée
     * \details La distance est exprimée en pixel source, et se mesure entre les centres des pixels.
     * \~english \brief Offset between source image's left and resmapled image's left
     * \details Distance is expressed as source pixel and between pixels' centers.
     */
    double left;

    /**
     * \~french \brief Buffer général
     * \details Il regroupe :
     * \li les buffers de mémorisation des lignes réechantillonnées en X (image et masque)
     * \li les buffers intermédiaires de calcul (image et masque)
     * \li les buffers des poids de l'interpolation
     * \li le buffer d'indexation des données
     * \~english \brief Global buffer
     * \details Contains:
     * \li widthwise resampled lines memorization buffers (image and mask)
     * \li intermediate calculation buffers (image and mask)
     * \li weights' buffer
     * \li index buffer
     */
    float* __buffer;

    /**
     * \~french \brief Buffer de stockage des lignes de l'image source
     * \details On stocke 4 lignes
     * \~english \brief Image source lines storage buffer
     * \details We store 4 lines
     */
    float* src_image_buffer[4];
    /**
     * \~french \brief Buffer de stockage des lignes de l'image source multiplexées
     * \details On stocke les 4 lignes sous forme multiplexée
     * \~english \brief \brief Multiplexed image source lines storage buffer
     * \details We store 4 lines, multiplexed
     */
    float* mux_src_image_buffer;

    /**
     * \~french \brief Buffer de stockage des lignes du masque source
     * \details On stocke 4 lignes
     * \~english \brief Mask source lines storage buffer
     * \details We store 4 lines
     */
    float* src_mask_buffer[4];
    /**
     * \~french \brief Buffer de stockage des lignes du masque source multiplexées
     * \details On stocke les 4 lignes sous forme multiplexée
     * \~english \brief Multiplexed mask source lines storage buffer
     * \details We store 4 lines, multiplexed
     */
    float* mux_src_mask_buffer;

    /**
     * \~french \brief Nombre de lignes réechantillonnées que l'on va mémoriser, pour l'image et le masque
     * \details On veut mémoriser un certain nombre de lignes (réechantillonnées dans le sens des X uniquement) pour ne pas refaire un travail déjà fait.
     * On va travailler les lignes 4 par 4 (pour l'utilisation des instructions SSE). On va donc mémoriser
     * un multiple de 4 lignes.
     * Une ligne va intervenir au maximum dans l'interpolation de #Ky lignes (diamètre du noyau d'interpolation)
     * Conclusion : on mémorise "Ky arrondi au multiple de 4 supérieur" lignes
     * \~english \brief Number of memorized resampled lines, for image and mask
     */
    int memorizedLines;
    /**
     * \~french \brief Indexation des lignes mémorisées
     * \details Elle permet de convertir un indice de la ligne de l'image source réechantillonnée en X en indice dans le tableau des lignes mémorisées (dans #resampled_image et #resampled_mask).
     * \~english \brief Memorized lines indexing
     * \details To convert source image line (widthwise resampled) indice to memorized line indice (in #resampled_image and #resampled_mask).
     */
    int* resampled_line_index;
    /**
     * \~french \brief Buffer de mémorisation des lignes de l'image déjà calculées
     * \details On mémorise memorize_line lignes de l'image source, réechantillonnées en X.
     * \~english \brief Buffer to memorize already calculated image's lines
     * \details We memorize memorize_line lines of source image, widthwise resampled.
     */
    float** resampled_image;
    /**
     * \~french \brief Buffer de mémorisation des lignes du masque déjà calculées
     * \details On mémorise memorize_line lignes du masque source, réechantillonnées en X.
     * \~english \brief Buffer to memorize already calculated mask's lines
     * \details We memorize memorize_line lines of source mask, widthwise resampled.
     */
    float** resampled_mask;

    /**
     * \~french \brief Ligne d'image, réechantillonnée en X, multiplexée
     * \~english \brief Image's line, widthwise resampled, multiplexed
     */
    float* mux_resampled_image;
    /**
     * \~french \brief Ligne de masque, réechantillonnée en X, multiplexée
     * \~english \brief Mask's line, widthwise resampled, multiplexed
     */
    float* mux_resampled_mask;

    /**
     * \~french \brief Ligne entièrement réechantillonnée
     * \~english \brief Completly resampled line
     */
    float *dst_image_buffer;

    /**
     * \~french \brief Tableau des poids temporaires
     * \details Quand on tient compte des masques dans l'interpolation, on va écarter certains pixels du calcul. La somme des poids des pixels utilisés n'est donc plus normalisée. On doit donc mémoriser la somme des poids au fur et à mesure pour normaliser la moyenne en fin de calcul.
     * \~english \brief Temporary weights' array
     * \details If masks are used, some pixels are not any more used by interpolation. Weight sum is not normalized. We have to memorize weight sum to normalize average in the end.
     */
    float *weight_buffer;

    /**
     * \~french \brief Poids de réechantillonnage, pour le sens des X
     * \details Les poids sont quadruplé pour permettre le calcul sur 4 lignes en même temps.
     * \~english \brief Widthwise resampling weights
     * \details Weights are quadrupled, to calulate 4 lines in the same time.
     */
    float* Wx;
    /** \~french
     * \brief Tableau des coordonnées pixel inférieures
     * \details Pour chaque pixel de destination, on précise le premier pixel source (numéro de colonne) qui va intervenir dans le calcul d'interpolation (valeur de retour de la fonction Kernel#weight)
     ** \~english \brief Min pixels coordinates array
     * \details For each destination pixel, we precise the first source pixel (column indice) which be used by interpolation (return value of function Kernel#weight)
     */
    int* xMin;

    /** \~french
     * \brief Retourne une ligne source réechantillonnée en X, entière
     * \details Lorsqu'une demande une ligne de l'image réechantillonnée, le calcul va être divisé en deux parties :
     * \li on réechantillonne les lignes de l'image source dans le sens des X, avec la fonction
     * \li on moyenne (avec pondération) les #Ky lignes sources réechantillonnées en X, autrement dit on réechantillonne dans le sens des Y
     *
     * Avant de lancer les calculs, on vérifie que la ligne source demandée n'est pas déjà disponible dans le buffer mémoire #resampled_image en utilisant l'index #resampled_line_index. Si ce n'est pas le cas, on calcule le paquet de 4 lignes contenant celle voulue, on les stocke et on met à jour la table d'index.
     * \param[in] line Indice de la ligne source à réechantillonner (0 <= line < source_image.height)
     * \return position dans le buffer de mémorisation #resampled_image (et #resampled_mask) de la ligne voulue
     */
    int resampleSourceLine ( int line );

public:
    /** \~french
     * \brief Retourne une ligne entièrement réechantillonnée, flottante
     * \details Lorsqu'une demande une ligne de l'image réechantillonnée, le calcul va être divisé en deux parties :
     * \li on réechantillonne les lignes de l'image source dans le sens des X, avec la fonction #resampleSourceLine
     * \li on moyenne (avec pondération) les #Ky lignes sources réechantillonnées en X, autrement dit on réechantillonne dans le sens des Y
     *
     * \param[in,out] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    int getline ( float* buffer, int line );

    /** \~french
     * \brief Retourne une ligne entièrement réechantillonnée, entière
     * \details Elle ne fait que convertir le résultat du #getline flottant en entier. On ne travaille en effet que sur des flottants, même si les canaux des images sont des entiers, et cela car les poids de l'interpolation sont toujours flottants.
     * \param[in,out] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    int getline ( uint8_t* buffer, int line );

    /** \~french
     * \brief Crée un objet ResampledImage à partir de tous ses éléments constitutifs
     * \param[in] image image source
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] bUseMask précise si le réechantillonnage doit tenir compte des masques
     * \param[in] KT noyau d'interpolation à utiliser pour le réechantillonnage
     ** \~english
     * \brief Create a ResampledImage object, from all attributes
     * \param[in] image source image
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] bbox bounding box
     * \param[in] bUseMask precise if resampling use masks
     * \param[in] KT interpolation kernel to use for resampling
     */
    ResampledImage ( Image *image, int width, int height, double resx, double resy, BoundingBox<double> bbox,
                     Interpolation::KernelType KT = Interpolation::LANCZOS_3, bool bMask = false );

    /**
     * \~french \brief Destructeur par défaut
     * \details Désallocation de la mémoire :
     * \li du buffer général #__buffer
     * \li du buffer d'index #resampled_line_index
     * \li des buffers #resampled_image et #resampled_mask
     *
     * Et suppression de #sourceImage.
     *
     * \~english \brief Default destructor
     * \details Desallocate :
     * \li global buffer #__buffer
     * \li index buffer #resampled_line_index
     * \li buffers #resampled_image and #resampled_mask
     *
     * And remove #source_image
     */
    ~ResampledImage() {
        _mm_free ( __buffer );
        delete[] resampled_line_index;
        delete[] resampled_image;
        if ( useMask ) delete[] resampled_mask;
        if ( ! isMask ) {
            delete sourceImage;
        }
    }

    /** \~french
     * \brief Sortie des informations sur l'image réechantillonnée
     ** \~english
     * \brief Resampled image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "--------- ResampledImage -----------" );
        Image::print();
        LOGGER_INFO ( "\t- Kernel size, x wise = " << Kx << ", y wise = " << Ky );
        LOGGER_INFO ( "\t- Offsets, dx = " << left << ", dy = " << top );
        if ( useMask ) {
            LOGGER_INFO ( "\t- Use mask in interpolation" );
        } else {
            LOGGER_INFO ( "\t- Doesn't use mask in interpolation" );
        }
        LOGGER_INFO ( "" );
    }
};

#endif




