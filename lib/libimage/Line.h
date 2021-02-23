/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
#include <Utils.h>

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Représentation d'une ligne flottante
 * \details Cette classe stocke une ligne d'image sur 4 canaux, 3 pour la couleur et un canal alpha non-associé (pas prémultiplié aux couleurs). Ce fonctionnement est toujours le même, que les canaux sources soient entiers ou flottants. En stockant toujours les informations dans ce format de travail, on va faciliter les calculs de fusion de plusieurs lignes, dont les caractéristiques étaient différentes. Le formattage final sera également facilité.
 *
 * Cette classe gère les données, que ce soit comme sources ou comme sortie, sur :
 * \li 1 canal : niveau de gris
 * \li 2 canaux : niveau de gris + alpha non-associé
 * \li 3 canaux : vraie couleur
 * \li 4 canaux : vraie couleur + alpha non-associé
 *
 * La classe travaille toujours sur des flottant, quel que soit le format de base des images lues. Cela permet de faire des calculs flottant, et de ne caster qu'au moment de l'écriture.
 *
 * Les données peuvent possédé un masque associé, auquel cas il sera stocké en parallèle des données. Quelque soit le mode de fusion utilisé par la suite, on tiendra toujours compte de ce masque.
 *
 * Les modes de fusion gérés sont :
 * \li par multiplication : les canaux sont multipliés un à un (valable uniquement pour les canaux entier sur 8 bits) -> #multiply
 * \li par transparence : on applique une formule d'alpha-blending -> #alphaBlending
 * \li par masque : seul le masques sont considérés, la donnée du dessus écrase celle du dessous -> #useMask
 *
 * \todo Travailler sur un nombre de canaux variable (pour l'instant, systématiquement 4, que ce soit en entier ou en flottant).
 * \todo Les modes de fusion DARKEN et LIGHTEN ne sont pas implémentés.
 * \todo Un pixel correspond toujours à 4 flottants (3 canaux + 1 alpha). Il doit être possible d'utiliser des fonctions SSE pour optimiser les calculs
 ** \~french
 * \brief Represent an image line, with float
 */
class Line {

public:
    /**
     * \~french \brief Canaux de couleur, sans tenir compte de l'alpha
     * \~english \brief Color's samples, ignoring alpha
     */
    float* samples;
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
     * \~french \brief Coefficient à appliquer, pour passer à un alpha flottant entre 0 et 1
     * \details \li Pour des données sur 8 bits -> 255.
     * \li Pour des données sur 32 bits (flottant) -> 1.
     * \~english \brief Coefficient to apply, to obtain an alpha between 0 and 1
     * \details \li 8-bit data -> 255.
     * \li 32-bit data (float) -> 1.
     */
    float coeff;

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
    Line ( int width, int samplesize ) : width ( width ) {
        if (samplesize == 1) coeff = 255.; //cas uint8_t
        else if (samplesize == 4) coeff = 1.; //cas float
        else LOGGER_ERROR("Sample size is unknown for the line");
        
        samples = new float[3*width];
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
    template<typename T>
    Line ( T* imageIn, uint8_t* maskIn, int srcSpp, int width, T* transparent ) : width ( width ) {
        if (sizeof(T) == 1) coeff = 255.; //cas uint8_t
        else coeff = 1.;
        samples = new float[3*width];
        alpha = new float[width];
        mask = new uint8_t[width];
        store ( imageIn, maskIn, srcSpp, transparent );
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
    template<typename T>
    Line ( T* imageIn, uint8_t* maskIn, int srcSpp, int width ) : width ( width ) {
        if (sizeof(T) == 1) coeff = 255.; //cas uint8_t
        else coeff = 1.;
        samples = new float[3*width];
        alpha = new float[width];
        mask = new uint8_t[width];
        store ( imageIn, maskIn, srcSpp );
    }

    /** \~french
     * \brief Stockage des données, avec précision d'une valeur de transparence
     * \details Les données sont sockées en convertissant le nombre de canaux si besoin est. L'alpha lui est potentiellement converti en flottant entre 0 et 1. Les pixels dont la couleur est celle précisée comme transparent sont "annulés" (alpha = 0). Cette fonction est un template mais n'est implémentée (spécifiée) que pour les entiers sur 8 bits et les flottant.
     * \param[in] imageIn données en entrée
     * \param[in] maskIn masque associé aux données en entrée
     * \param[in] srcSpp nombre de canaux dans les données sources
     * \param[in] width largeur de la ligne en pixel
     * \param[in] transparent valeur des pixels à considéré comme transparent
     ** \~english
     * \brief Data storage, with transparent value
     * \details Data are stored, converting number of samples per pixel if needed. Alpha is stored as a float between 0 and 1. Pixels whose value is the transparent one are "cancelled" (alpha = 0). This function is a template but is implemented (specified) only for 8-bit integers and floats.
     * \param[in] imageIn data to store
     * \param[in] maskIn associated mask
     * \param[in] srcSpp number of samples per pixel in input data
     * \param[in] width line's width, in pixel
     * \param[in] transparent pixel's value to consider as transparent
     */
    template<typename T>
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
    template<typename T>
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
    template<typename T>
    void write ( T* buffer, int outChannels );

    /** \~french
     * \brief Fusionne deux lignes par transparence (alpha blending)
     * \details La ligne courante est fusionnée avec une ligne par dessus, et le résultat est stocké dans la ligne courante.
     * Rappelons que les valeurs de transparence ne sont pas prémultipliée sur les canaux (en entrée et en sortie).
     * \image html merge_transparency.png
     * \param[in] above ligne du dessus, avec laquelle fusionner
     ** \~english
     * \brief Merge 2 lines with alpha blending
     * \details Current line is merged with an above line, and result is stored in the current line.
     * \image html merge_transparency.png
     * \param[in] above above line, to merge
     */
    void alphaBlending ( Line* above );

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
    void multiply ( Line* above );

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
    void useMask ( Line* above );

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

/* -------------------------------------FONCTIONS TEMPLATE ---------------------------------------- */

template<typename T>
void Line::write ( T* buffer, int outChannels ) {
    switch ( outChannels ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            buffer[i] = ( T ) ( 0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2] ) * alpha[i];
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            buffer[2*i] = ( T ) ( 0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2] );
            buffer[2*i+1] = ( T ) ( alpha[i]*coeff );
        }
        break;
    case 3:
        for ( int i = 0; i < width; i++ ) {
            buffer[3*i] =  ( T ) (alpha[i] * samples[3*i]);
            buffer[3*i+1] = ( T ) (alpha[i] * samples[3*i+1]);
            buffer[3*i+2] = ( T ) (alpha[i] * samples[3*i+2]);
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            buffer[4*i] = ( T ) (samples[3*i]);
            buffer[4*i+1] = ( T ) (samples[3*i+1]);
            buffer[4*i+2] = ( T ) (samples[3*i+2]);
            buffer[4*i+3] = ( T ) ( alpha[i]*coeff );
        }
        break;
    }
}

#endif
