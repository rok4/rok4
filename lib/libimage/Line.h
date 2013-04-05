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
 * \file Line.h
 * \~french
 * \brief Définition de la classe Line, permettant la manipulation et la conversion de lignes d'image.
 * \~english
 * \brief Define the Line class , to manipulate and convert image's lines.
 */

#ifndef LINE_H
#define LINE_H

#include <string.h>
#include <stdio.h>
#include <Logger.h>

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Représentation d'une ligne entière ou flottante
 * \details Cette classe stocke une ligne d'image sur 4 canaux, 3 pour la couleur et un canal alpha associé (prémultiplié aux couleurs). Ce fonctionnement est toujours le même, que les canaux sources soient entiers ou flottants. En stockant toujours les informations dans ce format de travail, on va faciliter les calculs de fusion de plusieurs lignes, dont les caractéristiques étaient différentes. Le formattage final sera également facilité.
 *
 * Cette classe gère les données, que ce soit comme sources ou comme sortie, sur :
 * \li 1 canal : niveau de gris
 * \li 2 canaux : niveau de gris + alpha associé
 * \li 3 canaux : vraie couleur
 * \li 4 canaux : vraie couleur + alpha associé
 *
 * Les données peuvent possédé un masque associé, auquel cas il sera stocké en parallèle des données. Quelque soit le mode de fusion utilisé par la suite, on tiendra toujours compte de ce masque.
 *
 * Les modes de fusion gérés sont :
 * \li par multiplication : les canaux sont multipliés un à un (valable uniquement pour les canaux entier sur 8 bits) -> #multiply
 * \li par transparence : on applique une formule d'alpha-blending -> #alphaBlending
 * \li par masque : seul le masques sont considérés, la donnée du dessus écrase celle du dessous -> #useMask
 *
 * \todo Les modes de fusion DARKEN et LIGHTEN ne sont pas implémentés.
 ** \~french
 * \brief Represent an image line, with integer or float
 */
template<typename T>
class Line {

public:
    /**
     * \~french \brief Canaux de couleur, prémultiplié par alpha
     * \~english \brief Color's samples, premultiplied with alpha
     */
    T* samples;
    /**
     * \~french \brief Valeurs d'alpha, entre 0 et 1
     * \~english \brief Alpha values, between 0 and 1
     */
    float* alpha;
    /**
     * \~french \brief Masque associé aux données
     * \details \li 0 = non-donnée
     * \li 1 -> 255 = donnée
     * \~english \brief Associated mask
     * \details \li 0 = nodata
     * \li 1 -> 255 = data
     */
    uint8_t* mask;

    /**
     * \~french \brief Largeur de la ligne (nombre de pixels)
     * \~english \brief Line's width (pixels' number)
     */
    int width;

    /** \~french
     * \brief Crée un objet Line à partir de la largeur
     * \details Il n'y a pas de stockage de données, juste une allocation de la mémoire nécessaire.
     * \param[in] width largeur de la ligne en pixel
     ** \~english
     * \brief Create a Line, from the width
     * \details No data storage, just a memory allocation.
     * \param[in] width line's width, in pixel
     */
    Line ( int width ) : width ( width ) {
        samples = new T[3*width];
        alpha = new float[width];
        mask = new uint8_t[width];
    }

    /** \~french
     * \brief Crée un objet Line à partir de données, et précision d'une valeur de transparence
     * \details Les données sont sockées et les pixels dont la valeur est celle transparente sont stocké avec un alpha nul
     * \param[in] imageIn données en entrée
     * \param[in] maskIn masque associé aux données en entrée
     * \param[in] srcSpp nombre de canaux dans les données sources
     * \param[in] width largeur de la ligne en pixel
     * \param[in] transparent valeur des pixels à considéré comme transparent
     ** \~english
     * \brief Create a Line, from the data and transparent value
     * \details Data are stored and pixel containing the transparent value are stored with a null alpha.
     * \param[in] imageIn data to store
     * \param[in] maskIn associated mask
     * \param[in] srcSpp number of samples per pixel in input data
     * \param[in] width line's width, in pixel
     * \param[in] transparent pixel's value to consider as transparent
     */
    Line ( T* imageIn, uint8_t* maskIn, int srcSpp, int width, T* transparent ) : width ( width ) {
        samples = new T[3*width];
        alpha = new float[width];
        mask = new uint8_t[width];
        store ( imageIn, mask, srcSpp, transparent );
    }

    /** \~french
     * \brief Crée un objet Line à partir de données
     * \param[in] imageIn données en entrée
     * \param[in] maskIn masque associé aux données en entrée
     * \param[in] srcSpp nombre de canux dans les données sources
     * \param[in] width largeur de la ligne en pixel
     ** \~english
     * \brief Create a Line from data
     * \param[in] imageIn data to store
     * \param[in] maskIn associated mask
     * \param[in] srcSpp number of samples per pixel in input data
     * \param[in] width line's width, in pixel
     */
    Line ( T* imageIn, uint8_t* maskIn, int srcSpp, int width ) : width ( width ) {
        samples = new T[3*width];
        alpha = new float[width];
        mask = new uint8_t[width];
        store ( imageIn, mask, srcSpp );
    }

    /** \~french
     * \brief Stockage des données, avec précision d'une valeur de transparence
     * \details Les données sont sockées en convertissant le nombre de canaux si besoin est. L'alpha lui est potentiellement converti en flottant entre 0 et 1. Les pixels dont la couleur est celle précisée comme transparent sont "annulés" (alpha = 0, et donc les canaux = 0). Cette fonction est un template mais n'est implémentée (spécifiée) que pour les entiers sur 8 bits et les flottant.
     * \param[in] imageIn données en entrée
     * \param[in] maskIn masque associé aux données en entrée
     * \param[in] srcSpp nombre de canux dans les données sources
     * \param[in] width largeur de la ligne en pixel
     * \param[in] transparent valeur des pixels à considéré comme transparent
     ** \~english
     * \brief Data storage, with transparent value
     * \details Data are stored, converting number of samples per pixel if needed. Alpha is stored as a float between 0 and 1. Pixels whose value is the transparent one are "cancelled" (alpha = 0, so samples = 0). This function is a template but is implemented (specified) only for 8-bit integers and floats.
     * \param[in] imageIn data to store
     * \param[in] maskIn associated mask
     * \param[in] srcSpp number of samples per pixel in input data
     * \param[in] width line's width, in pixel
     * \param[in] transparent pixel's value to consider as transparent
     */
    void store ( T* imageIn, uint8_t* maskIn, int srcSpp, T* transparent );

    /** \~french
     * \brief Stockage des données
     * \details Les données sont sockées en convertissant le nombre de canaux si besoin est. L'alpha lui est potentiellement converti en flottant entre 0 et 1. Cette fonction est un template mais n'est implémentée (spécifiée) que pour les entiers sur 8 bits et les flottant.
     * \param[in] imageIn données en entrée
     * \param[in] maskIn masque associé aux données en entrée
     * \param[in] srcSpp nombre de canux dans les données sources
     * \param[in] width largeur de la ligne en pixel
     ** \~english
     * \brief Data storage
     * \details Data are stored, converting number of samples per pixel if needed. Alpha is stored as a float between 0 and 1. This function is a template but is implemented (specified) only for 8-bit integers and floats.
     * \param[in] imageIn data to store
     * \param[in] maskIn associated mask
     * \param[in] srcSpp number of samples per pixel in input data
     * \param[in] width line's width, in pixel
     */
    void store ( T* imageIn, uint8_t* maskIn, int srcSpp );

    /** \~french
     * \brief Convertit et écrit la ligne de donnée
     * \details Cette fonction est un template mais n'est implémentée (spécifiée) que pour les entiers sur 8 bits et les flottant.
     * \param[in] buffer mémoire où écrire la ligne
     * \param[in] outChannels nombre de canaux des données écrites dans le buffer
     ** \~english
     * \brief Convert and write the data line
     * \details This function is a template but is implemented (specified) only for 8-bit integers and floats.
     * \param[in] buffer memory to write data
     * \param[in] outChannels number of samples per pixel in the buffer
     */
    void write ( T* buffer, int outChannels );

    /** \~french
     * \brief Fusionne deux lignes par transparence (alpha blending)
     * \details La ligne courante est fusionnée avec une ligne par dessus, et le résultat est stocké dans la ligne courante.
     * \image html merge_transparency.png
     * \param[in] above ligne du dessus, avec laquelle fusionner
     ** \~english
     * \brief Merge 2 lines with alpha blending
     * \details Current line is merged with an above line, and result is stored in the current line.
     * \image html merge_transparency.png
     * \param[in] above above line, to merge
     */
    void alphaBlending ( Line<T>* above );

    /** \~french
     * \brief Fusionne deux lignes par multiplication
     * \details La ligne courante est fusionnée avec une ligne par dessus, et le résultat est stocké dans la ligne courante.
     * \image html merge_multiply.png
     * \param[in] above ligne du dessus, avec laquelle fusionner
     ** \~english
     * \brief Merge 2 lines multiplying
     * \details Current line is merged with an above line, and result is stored in the current line.
     * \image html merge_multiply.png
     * \param[in] above above line, to merge
     */
    void multiply ( Line<T>* above );

    /** \~french
     * \brief Fusionne deux lignes par masque
     * \details La ligne courante est fusionnée avec une ligne par dessus, et le résultat est stocké dans la ligne courante.
     * \image html merge_mask.png
     * \param[in] above ligne du dessus, avec laquelle fusionner
     ** \~english
     * \brief Merge 2 lines using mask
     * \details Current line is merged with an above line, and result is stored in the current line.
     * \image html merge_mask.png
     * \param[in] above above line, to merge
     */
    void useMask ( Line<T>* above );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Libération de la mémoire occupée par les tableaux.
     * \~english
     * \brief Default destructor
     * \details Desallocate memory used by the Line object.
     */
    virtual ~Line() {
        delete[] alpha;
        delete[] samples;
        delete[] mask;
    }
};


/* ------------------------------------------------------------------------------------------------ */
/* --------------------------------- DÉFINITION DES FONCTIONS ------------------------------------- */
/* ------------------------------------------------------------------------------------------------ */

/* -------------------------------------FONCTIONS TEMPLATE ---------------------------------------- */

template<typename T>
void Line<T>::alphaBlending ( Line<T>* above ) {
    float finalAlpha;
    for ( int i = 0; i < width; i++ ) {
        if ( above->alpha[i] == 0. || ! above->mask[i] ) {
            // Le pixel de la ligne du dessus est transparent, ou n'est pas de la donnée, il ne change donc pas le pixel du dessous.
            continue;
        }

        alpha[i] = above->alpha[i] + alpha[i] * ( 1. - above->alpha[i] );

        samples[3*i] = ( T ) ( above->samples[3*i] + samples[3*i] * ( 1 - above->alpha[i] ) );
        samples[3*i+1] = ( T ) ( above->samples[3*i+1] + samples[3*i+1] * ( 1 - above->alpha[i] ) );
        samples[3*i+2] = ( T ) ( above->samples[3*i+2] + samples[3*i+2] * ( 1 - above->alpha[i] ) );
    }
}

template<typename T>
void Line<T>::useMask ( Line<T>* above ) {
    for ( int i = 0; i < width; i++ ) {
        if ( above->mask[i] ) {
            alpha[i] = above->alpha[i];
            samples[3*i] = above->samples[3*i];
            samples[3*i+1] = above->samples[3*i+1];
            samples[3*i+2] = above->samples[3*i+2];
        }
    }
}

template<typename T>
void Line<T>::store ( T* imageIn, uint8_t* maskIn, int srcSpp, T* transparent ) {
    memcpy ( mask, maskIn, width );
    switch ( srcSpp ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( T ) ) ) {
                memset ( samples+3*i, 0, 3*sizeof ( T ) );
                alpha[i] = 0.0;
            } else {
                samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
                alpha[i] = 1.0;
            }
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( T ) ) ) {
                memset ( samples+3*i, 0, 3*sizeof ( T ) );
                alpha[i] = 0.0;
            } else {
                samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
                alpha[i] = ( float ) imageIn[2*i+1];
            }
        }
        break;
    case 3:
        memcpy ( samples, imageIn, 3*width*sizeof ( T ) );
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( T ) ) ) {
                memset ( samples+3*i, 0, 3*sizeof ( T ) );
                alpha[i] = 0.0;
            } else {
                alpha[i] = 1.0;
            }
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( T ) ) ) {
                memset ( samples+3*i, 0, 3*sizeof ( T ) );
                alpha[i] = 0.0;
            } else {
                memcpy ( samples+i*3,imageIn+i*4,3*sizeof ( T ) );
                alpha[i] = ( float ) imageIn[i*4+3];
            }
        }
        break;
    }
}

/* --------------------------------- SPÉCIALISATION DE TEMPLATE ----------------------------------- */

// -------------- UINT8

template <>
void Line<uint8_t>::store ( uint8_t* imageIn, uint8_t* maskIn, int srcSpp, uint8_t* transparent ) {

    memcpy ( mask, maskIn, width );
    switch ( srcSpp ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3 ) ) {
                memset ( samples+3*i, 0, 3 );
                alpha[i] = 0.0;
            } else {
                samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
                alpha[i] = 1.0;
            }
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3 ) ) {
                memset ( samples+3*i, 0, 3 );
                alpha[i] = 0.0;
            } else {
                alpha[i] = ( float ) imageIn[2*i+1] / 255;
                samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
            }
        }
        break;
    case 3:
        memcpy ( samples, imageIn, 3*width );
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3 ) ) {
                memset ( samples+3*i, 0, 3 );
                alpha[i] = 0.0;
            } else {
                alpha[i] = 1.0;
            }
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3 ) ) {
                memset ( samples+3*i, 0, 3 );
                alpha[i] = 0.0;
            } else {
                memcpy ( samples+i*3,imageIn+i*4,3 );
                alpha[i] = ( float ) imageIn[i*4+3] / 255.;
            }
        }
        break;
    }

}

template <>
void Line<uint8_t>::store ( uint8_t* imageIn, uint8_t* maskIn, int srcSpp ) {
    mask = maskIn;
    switch ( srcSpp ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
            alpha[i] = 1.0;
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
            alpha[i] = ( float ) imageIn[2*i+1] / 255;
        }
        break;
    case 3:
        memcpy ( samples, imageIn, 3*width );
        for ( int i = 0; i < width; i++ ) {
            alpha[i] = 1.0;
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            memcpy ( samples+i*3,imageIn+i*4,3 );
            alpha[i] = ( float ) imageIn[i*4+3] / 255.;
        }
        break;
    }
}

template <>
void Line<uint8_t>::write ( uint8_t* buffer, int outChannels ) {
    switch ( outChannels ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            buffer[i] = ( uint8_t ) ( 0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2] );
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            buffer[2*i] = ( uint8_t ) ( 0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2] );
            buffer[2*i+1] = ( uint8_t ) ( alpha[i]*255 );
        }
        break;
    case 3:
        for ( int i = 0; i < width; i++ ) {
            buffer[3*i] = samples[3*i];
            buffer[3*i+1] = samples[3*i+1];
            buffer[3*i+2] = samples[3*i+2];
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            buffer[4*i] = samples[3*i];
            buffer[4*i+1] = samples[3*i+1];
            buffer[4*i+2] = samples[3*i+2];
            buffer[4*i+3] = ( uint8_t ) ( alpha[i]*255 );
        }
        break;
    }
}

template<>
void Line<uint8_t>::multiply ( Line<uint8_t>* above ) {
    for ( int i = 0; i < width; i++ ) {
        if ( ! above->mask[i] ) {
            // Le pixel de la ligne du dessus n'est pas de la donnée, il ne change donc pas le pixel du dessous.
            continue;
        }

        alpha[i] *= above->alpha[i];
        samples[3*i] = samples[3*i] * above->samples[3*i] / 255;
        samples[3*i+1] = samples[3*i+1] * above->samples[3*i+1] / 255;
        samples[3*i+2] = samples[3*i+2] * above->samples[3*i+2] / 255;
    }
}

// -------------- FLOAT

template <>
void Line<float>::store ( float* imageIn, uint8_t* maskIn, int srcSpp, float* transparent ) {
    mask = maskIn;
    switch ( srcSpp ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( float ) ) ) {
                alpha[i] = 0.0;
                memset ( samples+3*i, 0, sizeof ( float ) *3 );
            } else {
                alpha[i] = 1.0;
                samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
            }
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( float ) ) ) {
                alpha[i] = 0.0;
                memset ( samples+3*i, 0, sizeof ( float ) *3 );
            } else {
                alpha[i] = imageIn[2*i+1];
                samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
            }
        }
        break;
    case 3:
        memcpy ( samples, imageIn, sizeof ( float ) *3*width );
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( float ) ) ) {
                alpha[i] = 0.0;
                memset ( samples+3*i, 0, sizeof ( float ) *3 );
            } else {
                alpha[i] = 1.0;
            }
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            memcpy ( samples+i*3, imageIn+i*4, sizeof ( float ) *3 );
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( float ) ) ) {
                memset ( samples+3*i, 0, sizeof ( float ) *3 );
                alpha[i] = 0.0;
            } else {
                memcpy ( samples+i*3, imageIn+i*4, sizeof ( float ) *3 );
                alpha[i] = imageIn[i*4+3];
            }
        }
        break;
    }
}

template <>
void Line<float>::store ( float* imageIn, uint8_t* maskIn, int srcSpp ) {
    mask = maskIn;
    switch ( srcSpp ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
            alpha[i] = 1.0;
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
            alpha[i] = imageIn[2*i+1];
        }
        break;
    case 3:
        memcpy ( samples, imageIn, sizeof ( float ) *3*width );
        for ( int i = 0; i < width; i++ ) {
            alpha[i] = 1.0;
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            memcpy ( samples+i*3,imageIn+i*4,sizeof ( float ) *3 );
            alpha[i] = imageIn[i*4+3];
        }
        break;
    }
}

template <>
void Line<float>::write ( float* buffer, int outChannels ) {
    switch ( outChannels ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            buffer[i] = 0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2];
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            buffer[2*i] = 0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2];
            buffer[2*i+1] = alpha[i];
        }
        break;
    case 3:
        for ( int i = 0; i < width; i++ ) {
            buffer[3*i] = samples[3*i];
            buffer[3*i+1] = samples[3*i+1];
            buffer[3*i+2] = samples[3*i+2];
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            buffer[4*i] = samples[3*i];
            buffer[4*i+1] = samples[3*i+1];
            buffer[4*i+2] = samples[3*i+2];
            buffer[4*i+3] = alpha[i];
        }
        break;
    }
}

template<>
void Line<float>::multiply ( Line<float>* above ) {
    for ( int i = 0; i < width; i++ ) {
        if ( ! above->mask[i] ) {
            // Le pixel de la ligne du dessus n'est pas de la donnée, il ne change donc pas le pixel du dessous.
            continue;
        }

        alpha[i] *= above->alpha[i];
        samples[3*i] = samples[3*i] * above->samples[3*i];
        samples[3*i+1] = samples[3*i+1] * above->samples[3*i+1];
        samples[3*i+2] = samples[3*i+2] * above->samples[3*i+2];
    }
}

#endif
