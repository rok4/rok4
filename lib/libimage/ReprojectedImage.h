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
 * \file ReprojectedImage.h
 ** \~french
 * \brief Définition de la classe ReprojectedImage, permettant la reprojection d'image
 ** \~english
 * \brief Define class ReprojectedImage, allowing image reprojecting
 */

#ifndef REPROJECT_H
#define REPROJECT_H

#include "Image.h"
#include "Grid.h"
#include "Kernel.h"
#include "Interpolation.h"
#include <mm_malloc.h>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'image reprojetée
 * \details Cette classe permet de reprojeter une image lorsque celle ci n'est pas dans le bon système spatial. Pour calculer la valeur d'un pixel de l'image reprojetée, on peut utiliser plusieurs interpolations, implémentées dans les différentes classes héritant de Kernel. La reprojection s'appuie principalement sur l'utilisation d'une grille de reprojection (objet de la classe Grid), qui permet de connaître le pixel source correspondant à chacun des pixels reprojetés.
 *
 * Les calculs étant lourds, on va optimiser le calcul :
 * \li en allouant en une seule fois tout l'espace nécessaire au calcul
 * \li en utilisant au maximum les instructions SSE pour les calculs vectoriels
 *
 * Du fait de la reprojection (aucun alignement), les calculs réalisés pour un pixel ne peuvent être réutilisés pour un autre pixel. Cette différence avec le simple réechnatillonnnage rend les calculs beaucoup plus lourds
 *
 * Pour augmenter les performances de la reprojection avec les instructions SSE, on est amené :
 * \li à calculer les lignes 4 par 4
 * \li à travailler sur des buffers dont la taille est un multiple de 4, aligné sur 128 bits
 * \li à travailler avec des données multiplexée : on regroupe les même canaux des différentes lignes
 *
 * On peut également tenir compte du masque associé à l'image source, pour limiter l'interpolation aux valeurs réelles. Cela ajoute non seulement de la complexité aux calculs, mais prend également plus de place. On va donc limiter cette utilisation aux cas vraiment nécessaires : si l'image source possède un masque (image pas pleine) et si l'utilisateur spécifie qu'il veut l'utiliser dans la reprojection.
 *
 * Enfin, lors de la reprojection, on ne tient pas compte du propre masque. C'est à dire qu'on remplit un pixel reprojeté avec de la donnée à partir du moment où un pixel de donnée source appartenait au noyau d'interpolation. Si on veut utiliser cette image sans avoir un "gonflement" artificiel des données, on devra la lire en parallèle de son masque (interpolé en plus proche voisin) pour la restreindre à l'étendue réelle des données (cela peut se faire avec ExtendedCompoundImage).
 */
class ReprojectedImage : public Image {
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
     * \~french \brief Grille de reprojection
     * \details La grille est utilisée pour connaître le pixel source correspondant à celui de l'image reprojetée. Celle-ci doit déjà avoir subi toutes les transformations nécessaires (reprojection et transformation affine. La grille ne sera pas modifiée lors des calculs de reprojection de l'image, c'est pourquoi elle peut être utilisée par plusieurs images, si elles ont les mêmes dimensions (largeur, hauteur et bbox). Typiquement, une image reprojetée et son masque associé peuvent utiliser la même grille.
     * \~english \brief Reprojection grid
     * \details The grid is used to know corresponding pixel in the source image.
     */
    Grid* grid;

    /**
     * \~french \brief Buffer général
     * \details Il regroupe :
     * \li les buffers de mémorisation des lignes sources (image et masque)
     * \li les buffers intermédiaires de calcul (image et masque)
     * \li les buffers des poids de l'interpolation
     * \li le buffer d'indexation des données
     * \~english \brief Global buffer
     * \details Contains:
     * \li widthwise source lines memorization buffers (image and mask)
     * \li intermediate calculation buffers (image and mask)
     * \li weights' buffer
     * \li index buffer
     */
    float*  __buffer;

    /**
     * \~french \brief Nombre de lignes source que l'on va mémoriser, pour l'image et le masque
     * \details On ne veut pas charger l'intégralité de l'image source en mémoire vive, c'est pourquoi on ne va en stocker qu'un certain nombre :
     * \li ni trop faible car on ne doit pas être amené à demander une même ligne source plusieurs fois, pour des raisons de performances.
     * \li ni trop élevé, pour ne pas surcharger le mémoire.
     *
     * Du fait de la reprojection, 2 pixels sur la même ligne reprojetée vont correspondre à deux lignes potentiellement différentes dans l'image source. On va donc quantifier cet écart (Grid#deltaY) et l'utiliser pour définir le nombre de lignes sources mémorisée dans #src_image_buffer (et #src_mask_buffer) : #memorizedLines = Grid#deltaY + 2 x #Ky
     * \~english \brief Number of memorized source lines, for image and mask
     */
    int memorizedLines;

    /**
     * \~french \brief Indexation des lignes sources mémorisées
     * \details Elle permet de convertir un indice de la ligne de l'image source en indice dans le tableau des lignes mémorisées (dans #src_image_buffer et #src_mask_buffer).
     * \~english \brief Memorized source lines indexing
     * \details To convert source image line (widthwise resampled) indice to memorized line indice (in #resampled_image and #resampled_mask).
     */
    int* src_line_index;

    /**
     * \~french \brief Buffer de stockage des lignes de l'image source
     * \details On stocke #memorizedLines lignes
     * \~english \brief Image source lines storage buffer
     * \details We store #memorizedLines lines
     */
    float** src_image_buffer;
    /**
     * \~french \brief Buffer de stockage des lignes du masque source
     * \details On stocke #memorizedLines lignes
     * \~english \brief Mask source lines storage buffer
     * \details We store #memorizedLines lines
     */
    float** src_mask_buffer;

    /**
     * \~french \brief Index des lignes reprojetées
     * \details Les lignes de l'image reprojetée sont calculées 4 par 4. On veut donc mémoriser les lignes qui sont déjà calculées et présentes dans le buffer #dst_image_buffer (et le masque dans #dst_mask_buffer) pour pouvoir les retourner directement.
     *
     * #dst_line_index contient le résultat de la division entière de l'indice de la ligne reprojetée par 4, et la place de la ligne dans le buffer sera l'indice de ligne modulo 4.
     * \~english \brief Reprojected lines indexing
     */
    int dst_line_index;

    /**
     * \~french \brief 4 lignes d'image entièrement reprojetées
     * \~english \brief 4 completly reprojected image lines
     */
    float* dst_image_buffer[4];

    /**
     * \~french \brief 4 lignes d'image entièrement reprojetées, multiplexées
     * \~english \brief 4 completly reprojected image lines, multiplexed
     */
    float* mux_dst_image_buffer;

    /**
     * \~french \brief 4 lignes de masque entièrement reprojetées
     * \~english \brief 4 completly reprojected mask lines
     */
    float* dst_mask_buffer[4];

    /**
     * \~french \brief 4 lignes de masque entièrement reprojetées, multiplexées
     * \~english \brief 4 completly reprojected mask lines, multiplexed
     */
    float* mux_dst_mask_buffer;

    /**
     * \~french \brief Coordonnées X en pixel source des pixels de la ligne à reprojeter
     * \~english \brief X coordinate, in source pixel, of the line to reproject
     */
    float* X[4];

    /**
     * \~french \brief Coordonnées Y en pixel source des pixels de la ligne à reprojeter
     * \~english \brief Y coordinate, in source pixel, of the line to reproject
     */
    float* Y[4];

    /**
     * \~french \brief Poids pré-calculé, dans le sens des X
     * \details Du fait de la reprojection, tous les pixels à reprojeter sont décalés en X par rapport aux pixels sources d'une manière différente. Pour des raisons de performance, on ne peut pas calculer pour chaque pixel le tableau des poids correspondant. On va donc préalablement calculer 1024 possibilités de poids, qui seront utilisés pour l'ensemble de l'image reprojetée.
     * \~english \brief Pre-calculated weights, X wise
     */
    float* Wx[1024];

    /**
     * \~french \brief Poids pré-calculé, dans le sens des Y
     * \details Du fait de la reprojection, tous les pixels à reprojeter sont décalés en Y par rapport aux pixels sources d'une manière différente. Pour des raisons de performance, on ne peut pas calculer pour chaque pixel le tableau des poids correspondant. On va donc préalablement calculer 1024 possibilités de poids, qui seront utilisés pour l'ensemble de l'image reprojetée.
     * \~english \brief Pre-calculated weights, Y wise
     */
    float* Wy[1024];

    /**
     * \~french \brief Poids dans le sens des X utilisés pour le calcul des 4 pixels en cours, multiplexés
     * \~english \brief X wise weights use to calculate the 4 pixels in progress, multiplexed
     */
    float* WWx;
    /**
     * \~french \brief Poids dans le sens des Y utilisés pour le calcul des 4 pixels en cours, multiplexés
     * \~english \brief Y wise weights use to calculate the 4 pixels in progress, multiplexed
     */
    float* WWy;

    /**
     * \~french \brief Premiers pixels sources à utiliser, dans le sens des X, pré-calculé
     * \details Du fait de la reprojection, tous les pixels à reprojeter sont décalés en X par rapport aux pixels sources d'une manière différente. Pour des raisons de performance, on ne peut pas calculer pour chaque pixel le premier pixel source à utiliser. On va donc préalablement calculer 1024 possibilités de premier pixel, qui seront utilisés pour l'ensemble de l'image reprojetée.
     * \~english \brief First usefull source pixels, X wise, pre-calculated
     */
    int xmin[1024];
    /**
     * \~french \brief Premiers pixels sources à utiliser, dans le sens des Y, pré-calculé
     * \details Du fait de la reprojection, tous les pixels à reprojeter sont décalés en Y par rapport aux pixels sources d'une manière différente. Pour des raisons de performance, on ne peut pas calculer pour chaque pixel le premier pixel source à utiliser. On va donc préalablement calculer 1024 possibilités de premier pixel, qui seront utilisés pour l'ensemble de l'image reprojetée.
     * \~english \brief First usefull source pixels, Y wise, pre-calculated
     */
    int ymin[1024];

    /**
     * \~french \brief Pixels sources à utiliser pour les 4 pixels en cours, multiplexés
     * \~english \brief Source pixels to use to calculate the 4 pixels in progress, multiplexed
     */
    float* tmp1Img;

    /**
     * \~french \brief 4 pixels interpolés dans le sens des X , multiplexés
     * \~english \brief 4 pixels, X wise interpolated, multiplexed
     */
    float* tmp2Img;

    /**
     * \~french \brief Pixels du masque source à utiliser pour les 4 pixels en cours, multiplexés
     * \~english \brief Source mask's pixels to use to calculate the 4 pixels in progress, multiplexed
     */
    float* tmp1Msk;

    /**
     * \~french \brief 4 pixels du masque, interpolé dans le sens des X , multiplexés
     * \~english \brief 4 mask pixels, X wise interpolated, multiplexed
     */
    float* tmp2Msk;

    /** \~french
     * \brief Retourne une ligne entièrement reprojetée, flottante
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return Tableau contenant la ligne reprojetée
     */
    float* computeDestLine ( int line );

    /** \~french
     * \brief Retourne l'index dans le buffer #src_image_buffer (et #src_mask_buffer) de la ligne source voulue
     * \details On ne mémorise que #memorizedLines lignes sources. Lorsque l'on a besoin d'une ligne source, on en demande l'index. Si cette ligne est déjà chargée dans le buffer, on retourne directement l'index. Sinon, on récupère la ligne de #sourceImage, on la stocke, on met à jour la table des index #src_line_index, et on retourne l'index de la ligne voulue.
     * \param[in] line Indice de la ligne source dont on veut l'indice
     * \return Indice de la ligne voulue dans le buffer des sources
     */
    int getSourceLineIndex ( int line );

public:

    /** \~french
     * \brief Crée un objet ReprojectedImage à partir de tous ses éléments constitutifs, sauf les résolutions (calculées)
     * \param[in] image image source
     * \param[in] bbox emprise rectangulaire de l'image reprojetée
     * \param[in] grid grille de reprojection à utiliser
     * \param[in] KT noyau d'interpolation à utiliser pour la reprojection
     * \param[in] bUseMask précise si la reprojection doit tenir compte des masques
     ** \~english
     * \brief Create a ReprojectedImage object, from all attributes, except resolutions (calculated)
     * \param[in] image source image
     * \param[in] bbox reprojected image bounding box
     * \param[in] grid reprojecting grid to use
     * \param[in] KT interpolation kernel to use for reprojecting
     * \param[in] bUseMask precise if reprojecting use masks
     */
    ReprojectedImage ( Image *image, BoundingBox<double> bbox, Grid* grid, Interpolation::KernelType KT = Interpolation::LANCZOS_2, bool bMask = false ) : Image ( grid->width, grid->height,image->getChannels(), bbox ),sourceImage ( image ), grid ( grid ), K ( Kernel::getInstance ( KT ) ), useMask ( bMask ) {
        initialize();
    }

    /** \~french
     * \brief Crée un objet ReprojectedImage à partir de tous ses éléments constitutifs
     * \param[in] image image source
     * \param[in] bbox emprise rectangulaire de l'image reprojetée
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] grid grille de reprojection à utiliser
     * \param[in] KT noyau d'interpolation à utiliser pour la reprojection
     * \param[in] bUseMask précise si la reprojection doit tenir compte des masques
     ** \~english
     * \brief Create a ReprojectedImage object, from all attributes
     * \param[in] image source image
     * \param[in] bbox reprojected image bounding box
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] grid reprojecting grid to use
     * \param[in] KT interpolation kernel to use for reprojecting
     * \param[in] bUseMask precise if reprojecting use masks
     */
    ReprojectedImage ( Image *image, BoundingBox<double> bbox, double resx, double resy, Grid* grid, Interpolation::KernelType KT = Interpolation::LANCZOS_2, bool bMask = false ) : Image ( grid->width, grid->height,image->getChannels(), resx, resy, bbox ),sourceImage ( image ), grid ( grid ), K ( Kernel::getInstance ( KT ) ), useMask ( bMask ) {
        initialize();
    }

    /** \~french
     * \brief Initialise les buffers de calcul
     ** \~english
     * \brief Initialize calculation's buffers
     */
    void initialize();

    int getline ( float* buffer, int line );
    int getline ( uint8_t* buffer, int line );
    int getline ( uint16_t* buffer, int line );

    /**
     * \~french \brief Destructeur par défaut
     * \details Désallocation de la mémoire :
     * \li du buffer général #__buffer
     * \li du buffer d'index #src_line_index
     * \li des buffers #src_image_buffer et #src_mask_buffer
     *
     * Et suppression de #sourceImage.
     *
     * \~english \brief Default destructor
     * \details Desallocate global :
     * \li buffer #__buffer
     * \li index buffer #src_line_index
     * \li buffers #src_image_buffer and #src_mask_buffer
     *
     * And remove #sourceImage
     */
    ~ReprojectedImage() {
        _mm_free ( __buffer );

        delete[] src_image_buffer;
        delete[] src_line_index;

        if ( useMask ) {
            delete[] src_mask_buffer;
        }

        if ( ! isMask ) {
            // Le masque utilise la même grille, c'est pourquoi seule l'image de données supprime la grille.
            delete grid;
            delete sourceImage;
        }
    }

    /** \~french
     * \brief Sortie des informations sur l'image reprojetée
     ** \~english
     * \brief Reprojected image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "--------- ReprojectedImage -----------" );
        Image::print();
        LOGGER_INFO ( "\t- Kernel size, x wise = " << Kx << ", y wise = " << Ky );
        LOGGER_INFO ( "\t- Ratio, x wise = " << ratioX << ", y wise = " << ratioY );
        LOGGER_INFO ( "\t- Source lines buffer size = " << memorizedLines );
        if ( useMask ) {
            LOGGER_INFO ( "\t- Use mask in interpolation" );
        } else {
            LOGGER_INFO ( "\t- Doesn't use mask in interpolation" );
        }
        grid->print();
        LOGGER_INFO ( "" );
    }

};

#endif

