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
 * \file Kernel.h
 * \~french
 * \brief Définition de la classe Kernel, classe mère des différents noyaux d'interpolation
 * \~english
 * \brief Define the Kernel class, super class for different interpolation kernels
 */

#ifndef KERNEL_H
#define KERNEL_H

#include "Interpolation.h"
#include <iostream>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Gestion du noyau d'interpolation
 * \details Afin de réechantillonner une images qui n'a pas les bonnes résolutions, ou les bonnes phases, on est amené à utiliser un mode d'interpolation, c'est-à-dire une façon de calculer la valeur d'un pixel à partir d'un ou plusieurs pixels sources.
 *
 * Le calcul de réechantillonnage s'appuie sur un noyau d'interpolation, définissant les pixels sources à considérer dans le calcul et le \b poids qui leur est affecté (en fonction de la \b distance en pixel). En effet, une interpolation n'est rien d'autre qu'une moyenne pondérée. Ce sont ces deux caractéristiques qui vont varier d'un noyau à l'autre.
 *
 * Les noyaux sont gérés en 1 dimension. On y fera appel dans le sens des X puis dans le sens des Y pour obtenir une interpolation en deux dimensions.
 *
 * Les distances sont toujours exprimée en pixel source, et on considérera les centres des pixels.
 */
class Kernel {

    private:
        /**
         * \~french \brief Rayon de base du noyau d'interpolation
         * \details Chaque noyau a un rayon de base, qui va définir les pixels source de part et d'autre du pixel de destination qui vont compter dans le calcul (avoir un poids non nul). On peut imaginer un cercle autour du pixel final et considérer les pixels sources dans ce cercle.
         *
         * Le rayon est donc une distance exprimée en pixel source
         *
         * Ce rayon de base peut être modulé par le rapport des résolutions, c'est-à-dire le ratio :
         *
         * \b resolution destination / \b resolution source
         *
         * Et cela pour rendre possible les sur et sous échantillonnage.
         *
         * \~ \image html rayon_interpolation.png
         */
        const double kernel_size;

        /**
         * \~french \brief Tableau des poids
         * \details On ne veut pas calculer à la volée chaque poids à chaque utilisation du noyau (cela serait trop gourmand en calcul pour les noyaux dont la fonction de définition est trop complexe : sinus...). On va donc calculer 1025 poids, équitablement répartis pour des distances de 0 à kernel_size.
         */
        float coeff[1025];

        /**
         * \~french \brief Influence du rapport des résolutions sur la taille du noyau
         * \details
         * \li Si const_ratio est \b vrai, la taille du noyau kernel_size reste constante
         * \li Si const_ratio est \b false la taille du noyeau est dépendante du ratio est vaut
         *          - kernel_size pour un ratio inférieur 1
         *          - kernel_size * ratio pour un ratio supérieur à 1
         */
        const bool const_ratio;

        /**
         * \~french \brief Fonction caractéristique du noyau d'interpolation
         * \details C'est une fonction qui détermine le poids d'un pixel source en fonction de sa distance au pixel à calculer.
         * \param[in] d distance entre le pixel source et le pixel à calculer
         * \return poids affecté au pixel source
         */
        virtual double kernel_function(double d) = 0;

    protected:

        /**
         * \~french \brief Initialise le tableau des poids
         * \details On divise kernel_size en 1024 section et on calcule le poids pour chacun de ces points équitablement répartis.
         *
         * Cette fonction doit typiquement être apellée à la fin du constructeur des classes filles
         * (pour des raisons d'ordre d'initialisation des instances mère/fille).
         */
        void init() {
            for (int i = 0; i <= 1024; i++) {
                coeff[i] = kernel_function(i * kernel_size / 1024.);
            }
        }

        /**
         * \~french \brief Crée un Kernel à partir de tous ses éléments constitutifs
         * \details Ne peux être appelé que par les constructeurs des classes filles.
         * \param[in] kernel_size rayon de base du noyau d'interpolation
         * \param[in] const_ratio influence du rapport des résolutions sur le rayon du noyau
         */
         Kernel(double kernel_size, bool const_ratio = false) : kernel_size(kernel_size), const_ratio(const_ratio) {}

    public:

        /**
         * \~french \brief Usine d'instanciation d'un type de noyau
         * \param[in] T type de noyau d'interpolation
         * \return noyau d'interpolation
         */
        static const Kernel& getInstance(Interpolation::KernelType T = Interpolation::LANCZOS_3);

        /**
         * \~french \brief Calcule la taille effective du noyau en nombre de pixels sources
         * \details On tient compte du ratio et de const_ratio
         * \param[in] ratio rapport resolution destination / resolution source.
         * \li si supérieur à 1 : sous-échantillonnage
         * \li si inférieur à 1 : sur échantillonage
         * \return rayon effectif du noyau d'interpolation
         */
        inline double size(double ratio = 1.) const {
            if (const_ratio || ratio <= 1) return kernel_size;
            else return kernel_size * ratio;
        }

        /**
         * \~french \brief Calcule les poids pour chaque pixel source
         * \details On tient compte du ratio et de const_ratio
         * \param[out] W tableau des poids à affecter aux pixels sources
         * \param[in] length nombre de poids à calculer
         * \param[in] x coordonnée en pixel source du pixel à calculer (distance entre le centre du pixel source origine et le centre du pixel à calculer)
         * \param[in] max
         * \return indice du premier pixel source comptant dans le calcul (poids non nul)
         */
        virtual int weight( float* W, int length, double x, int max ) const;

};

#endif
