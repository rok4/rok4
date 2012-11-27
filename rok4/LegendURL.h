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
 * \file LegendURL.h
 * \~french
 * \brief Définition de la classe LegendURL gérant les éléments de légendes des documents de capacités
 * \~english
 * \brief Define the LegendURL Class handling capabilities legends elements
 */

#ifndef LEGENDURL_H
#define LEGENDURL_H
#include "ResourceLocator.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance LegendURL représente un élément LegendURL dans les différents documents de capacités. 
 * \brief Gestion des éléments de légendes des documents de capacités
 * \~english
 * A LegendURL represent a LegendURL element in the differents capabilities documents.
 * \brief Legends handler for the capabilities documents
 */
class LegendURL : public ResourceLocator {
private:
    /**
     * \~french \brief Largeur de l'image
     * \~english \brief Image width
     */
    int width;
    /**
     * \~french \brief Hauteur de l'image
     * \~english \brief Image height
     */
    int height;
    /**
     * \~french \brief Échelle minimum à laquelle s'applique la légende
     * \~english \brief Minimum scale at which the legend is applicable
     */
    double minScaleDenominator;
    /**
     * \~french \brief Échelle maximum à laquelle s'applique la légende
     * \~english \brief Maximum scale at which the legend is applicable
     */
    double maxScaleDenominator;
public:
    /**
     * \~french
     * \brief Crée un LegendURL à partir des ses éléments constitutifs
     * \param[in] format format de la légende (cf ResourceLocator)
     * \param[in] href lien vers la légende
     * \param[in] width largeur de l'image
     * \param[in] height hauteur de l'image
     * \param[in] minScaleDenominator échelle minimum à laquelle s'applique la légende
     * \param[in] maxScaleDenominator échelle maximum à laquelle s'applique la légende
     * \~english
     * \brief Create a LegendURL
     * \param[in] format image format (see ResourceLocator)
     * \param[in] href link to the legend image (see ResourceLocator)
     * \param[in] width image width
     * \param[in] height image height
     * \param[in] minScaleDenominator minimum scale at which the legend is applicable
     * \param[in] maxScaleDenominator maximum scale at which the legend is applicable
     */
    LegendURL ( std::string format, std::string href,int width, int height, double minScaleDenominator, double maxScaleDenominator );
    /**
     * \~french
     * Crée un LegendURL à partir d'un autre 
     * \brief Constructeur de copie
     * \param[in] origLUrl LegendURL à copié
     * \~english
     * Create a LegendURL from another
     * \brief Copy Constructor
     * \param[in] origLUrl LegendURL to copy
     */
    LegendURL ( const LegendURL &origLUrl );
    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    LegendURL& operator= ( LegendURL const & other);
    /**
     * \~french
     * \brief Test d'egalite de 2 LegendURLs
     * \return true si tous les attributs sont identiques, false sinon
     * \~english
     * \brief Test whether 2 LegendURLs are equals
     * \return true if all their attributes are identical
     */
    bool operator== ( const LegendURL& other ) const;
/**
     * \~french
     * \brief Test d'inégalite de 2 LegendURLs
     * \return true s'ils ont un attribut différent, false sinon
     * \~english
     * \brief Test whether 2 LegendURLs are different
     * \return true if one of their attributes is different
     */
    bool operator!= ( const LegendURL& other ) const;
    
    /**
     * \~french
     * \brief Retourne la largeur de l'image
     * \return largeur
     * \~english
     * \brief Return the image width
     * \return width
     */
    inline int getWidth() {
        return width;
    }
    
    /**
     * \~french
     * \brief Retourne la hauteur de l'image
     * \return hauteur
     * \~english
     * \brief Return the image height
     * \return height
     */
    inline int getHeight() {
        return height;
    }
    
    /**
     * \~french
     * \brief Retourne l'échelle minimum
     * \return échelle minimum
     * \~english
     * \brief Return the minimum scale
     * \return minimum scale
     */
    inline double getMinScaleDenominator() {
        return minScaleDenominator;
    }
    
    /**
     * \~french
     * \brief Retourne l'échelle maximum
     * \return échelle maximum
     * \~english
     * \brief Return the maximum scale
     * \return maximum scale
     */
    inline double getMaxScaleDenominator() {
        return maxScaleDenominator;
    }
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~LegendURL();
};

#endif // LEGENDURL_H
