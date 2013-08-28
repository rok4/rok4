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
 * \file Kernel.cpp
 * \~french
 * \brief Implémentation de la classe Kernel, classe mère des différents noyaux d'interpolation
 * \~english
 * \brief Implement the Kernel class, super class for different interpolation kernels
 */

#include <cmath>
#include "Kernel.h"
#include "Logger.h"

int Kernel::weight ( float* W, int& length, double x, int max ) const {

    double rayon = ( double ) length/2.;
    int xmin = ceil ( x - rayon - 1E-7 );

    // On ne sort pas de l'image à interpoler à gauche ...
    if ( xmin < 0 ) {
        xmin = 0;
        rayon = std::abs(x);
        length = ceil(rayon * 2 - 1E-7);
    }
    // ... comme à droite, quitte à diminuer le nombre de poids
    if ( xmin + length > max ) {
        rayon = std::abs(double(max - 1) - x);
        xmin = ceil ( x - rayon - 1E-7 );
        length = ceil(rayon * 2 - 1E-7);
    }

    if (length == 1) {
        W[0] = 1.;
        return xmin;
    }

    double step = 1024. / rayon;
    double sum = 0;              // somme des poids pour normaliser en fin de calcul.
    double indf = ( x - xmin ) * step;  // index flottant dans le tableau coeff
    int i = 0;

    for ( ; indf >= 0; indf -= step ) {
        int ind = ( int ) indf;
        // le coefficient est interpolé linéairement par rapport au deux coefficients qui l'entourent
        sum += W[i++] = coeff[ind] + ( coeff[ind+1] - coeff[ind] ) * ( indf - ind );
    }

    for ( indf = -indf; indf < 1024. && i < length; indf += step ) {
        int ind = ( int ) indf;
        sum += W[i++] = coeff[ind] + ( coeff[ind+1] - coeff[ind] ) * ( indf - ind );
    }

    length = i;

    while (i--) {
        W[i] /= sum;   // On normalise pour que la somme des poids fasse 1.
    }

    return xmin;
}


/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Noyau d'interpolation de type Lanczos
 * \details Il existe plusieurs noyaux lanczos : lanczos R avec R e [2,4]
 * Les caractéristiques sont :
 * \li taille de base du noyau : R
 * \li taille du noyau constant : non
 * \li fonction caractéristique : sinusoïdale
 *
 * \image html lanczos.png
 * \li 2 -> vert
 * \li 3 -> rouge
 * \li 4 -> bleu
 */
template<int s>
class Lanczos : public Kernel {
    friend const Kernel& Kernel::getInstance ( Interpolation::KernelType T );

private:
    double kernel_function ( double d ) {
        if ( d > s ) return 0.;
        else if ( d == 0. ) return 1.;
        else {
            d *= 3.14159265358979323846;
            return ( sin ( d ) / d ) * ( sin ( d/s ) / d * s );
        }
    }

    Lanczos() : Kernel ( s ) {
        init();
    }
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Noyau d'interpolation de type plus proche voisin
 * \details Les caractéristiques sont :
 * \li taille de base du noyau : 0.5
 * \li taille du noyau constant : oui
 * \li fonction caractéristique : escalier
 *
 * \image html ppv.png
 */
class NearestNeighbour : public Kernel {
    friend const Kernel& Kernel::getInstance ( Interpolation::KernelType T );

private:
    double kernel_function ( double d ) {
        if ( d > 0.5 ) return 0.;
        else return 1.;
    }
    NearestNeighbour() : Kernel ( 0.5, true ) {
        init();
    }
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Noyau d'interpolation de type linéaire
 * \details Les caractéristiques sont :
 * \li taille de base du noyau : 1
 * \li taille du noyau constant : non
 * \li fonction caractéristique : affine
 *
 * \image html linear.png
 */
class Linear : public Kernel {
    friend const Kernel& Kernel::getInstance ( Interpolation::KernelType T );

private:
    double kernel_function ( double d ) {
        if ( d > 1 ) return 0.;
        else return 1.-d;
    }
    Linear() : Kernel ( 1. ) {
        init();
    }
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Noyau d'interpolation de type bicubique
 * \details Les caractéristiques sont :
 * \li taille de base du noyau : 2
 * \li taille du noyau constant : non
 * \li fonction caractéristique : polynomiale
 *
 * \image html cubic.png
 */
class CatRom : public Kernel {

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

    friend const Kernel& Kernel::getInstance ( Interpolation::KernelType T );
private:
    double kernel_function ( double d ) {
        if ( d > 2 ) return 0.;
        else if ( d > 1 ) return 2. + d* ( -4. + d* ( 2.5 - 0.5*d ) );
        else return 1. + d*d* ( 1.5*d - 2.5 );
    }
    CatRom() : Kernel ( 2. ) {
        init();
    }
};



const Kernel& Kernel::getInstance ( Interpolation::KernelType T ) {
    static NearestNeighbour nearest_neighbour;
    static Linear linear;
    static CatRom catrom;
    static Lanczos<2> lanczos_2;
    static Lanczos<3> lanczos_3;
    static Lanczos<4> lanczos_4;

    switch ( T ) {
    case Interpolation::NEAREST_NEIGHBOUR:
        return nearest_neighbour;
        break;
    case Interpolation::LINEAR:
        return linear;
        break;
    case Interpolation::CUBIC:
        return catrom;
        break;
    case Interpolation::LANCZOS_2:
        return lanczos_2;
        break;
    case Interpolation::LANCZOS_3:
        return lanczos_3;
        break;
    case Interpolation::LANCZOS_4:
        return lanczos_4;
        break;
    }
    return lanczos_3;
}


