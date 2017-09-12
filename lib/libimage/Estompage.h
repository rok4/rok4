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

#ifndef ESTOMPAGE_H
#define ESTOMPAGE_H

#include <string>
#include "Interpolation.h"

/**
 * \file Estompage.h
 * \~french
 * \brief Définition de la classe Estompage modélisant l'estompage
 * \~english
 * \brief Define the Estompage Class handling estompage definition
 */

class Estompage {
private :
    /**
     * \~french \brief Indique si le style est un estompage
     * \~english \brief Indicate if the style is an estompage
     */
    bool estompage;
    /**
     * \~french \brief Azimuth du soleil en degré
     * \~english \brief Sun's azimuth in degree
     */
    float azimuth;
    /**
     * \~french \brief Facteur d'éxagération de la pente
     * \~english \brief Slope exaggeration factor
     */
    float zFactor;
    /**
     * \~french \brief Zenith du soleil en degré
     * \~english \brief Sun's zenith in degree
     */
    float zenith;
    /**
     * \~french \brief Interpolation pour l'estompage
     * \~english \brief Estompage interpolation
     */
    std::string interpolation;

public:
    /**
     * \~french \brief Construteur
     * \~english \brief Constructor
     */
    Estompage();
    /**
     * \~french \brief Construteur
     * \~english \brief Constructor
     */
    Estompage(bool est, float azi, float zf, float zen, std::string inter);
    /**
     * \~french \brief Construteur
     * \~english \brief Constructor
     */
    Estompage(const Estompage &obj);
    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    ~Estompage();

    /**
     * \~french
     * \brief Détermine si le style décrit un estompage
     * \return true si oui
     * \~english
     * \brief Determine if the style describe a relief shadows
     * \return true if it does
     */
    inline bool isEstompage() {
        return estompage;
    }

    /**
     * \~french
     * \brief Retourne le zenith du soleil
     * \return zenith
     * \~english
     * \brief Return the sun zenith
     * \return zenith
     */
    inline float getZenith() {
        return zenith;
    }

    /**
     * \~french
     * \brief Retourne l'azimuth du soleil
     * \return azimuth
     * \~english
     * \brief Return the sun azimuth
     * \return azimuth
     */
    inline float getAzimuth() {
        return azimuth;
    }

    /**
     * \~french
     * \brief Retourne le facteur d'exageration de la pente
     * \return zFactor
     * \~english
     * \brief Return the exageration factor of the slope
     * \return zFactor
     */
    inline float getZFactor() {
        return zFactor;
    }
    /**
    * \~french
    * \brief Retourne l'interpolation de l'estompage
    * \return interpolation de l'estompage
    * \~english
    * \brief Return the estompage interpolation
    * \return the estompage interpolation
    */
    inline Interpolation::KernelType getInterpolation() {
      return Interpolation::fromString(interpolation);
    }

    /**
     * \~french
     * \brief Modifie estompage
     * \~english
     * \brief Set relief shadows
     */
    void setEstompage(bool est) {
        estompage = est;
    }

    /**
     * \~french
     * \brief Modifie le zenith du soleil
     * \~english
     * \brief Set the sun zenith
     */
    void setZenith( float zen) {
        zenith = zen;
    }

    /**
     * \~french
     * \brief Modifie l'azimuth du soleil
     * \~english
     * \brief Set the sun azimuth
     */
    void setAzimuth(float azi) {
        azimuth = azi;
    }

    /**
     * \~french
     * \brief Modifie l'éxagération de la pente
     * \~english
     * \brief Set the slope exaggeration
     */
    void setZFactor(float zf) {
        zFactor = zf;
    }
    /**
    * \~french
    * \brief Modifie l'interpolation de l'estompage
    * \~english
    * \brief Set the estompage interpolation
    */
    void setInterpolation(std::string inter) {
      interpolation = inter;
    }
};

#endif // ESTOMPAGE_H
