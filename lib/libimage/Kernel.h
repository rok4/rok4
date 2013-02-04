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

#ifndef KERNEL_H
#define KERNEL_H

#include "Interpolation.h"

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
    friend class NearestNeighbour;
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
        for (int i = 0; i <= 1024; i++) coeff[i] = kernel_function(i * kernel_size / 1024.);
    }

    /**
     * Constructeur de la classe mère
     * Ne peux être appelé que par les constructeurs des classes filles.
     */
     Kernel(double kernel_size, bool const_ratio = false) : kernel_size(kernel_size), const_ratio(const_ratio) {}

public:

    /**
     * Factory permettant d'obtenir une instance d'un type de noyau donné.
     */
    static const Kernel& getInstance(Interpolation::KernelType T = Interpolation::LANCZOS_3);

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
        if (ratio <= 1 || const_ratio) return kernel_size;
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
    virtual double weight(float* W, int &length, double x, double ratio) const;

};




#endif
