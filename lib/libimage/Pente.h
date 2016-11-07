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
 * \file Pente.h
 ** \~french
 * \brief Définition de la classe Pente
 ** \~english
 * \brief Define class Pente
 */

#ifndef PENTE_H
#define PENTE_H

#include "Logger.h"

#include <string>


class Pente {

private:

     /** \~french
     * \brief algo : choix de l'algorithme de calcul de pentes par l'utilisateur ("H" pour Horn)
     ** \~english
     * \brief algo : slope calculation algorithm chosen by the user ("H" for Horn)
     */
    std::string algo;

    /** \~french
    * \brief isPente : indique que style est calcul de pente a partir de la donnée
    ** \~english
    * \brief isPente : indicate that the style is a slope computed from data
    */
    bool isPente;

    /** \~french
    * \brief unit : unité de la pente
    ** \~english
    * \brief unit : slope unit
    */
    std::string unit;

public:

    /**
     * \~french
     * \brief Constructeur sans arguments
     * \~english
     * \brief Constructor without arguments
     */
    Pente(): algo ("H"), isPente (false), unit ("degree") {

    }

    /**
     * \~french
     * \brief Constructeurs avec des arguments
     * \~english
     * \brief Constructor with arguments
     */
    Pente(std::string a,bool p = false,std::string u = "degree"): algo (a), isPente (p), unit (u) {

    }

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~Pente() {

    }

    /**
     * \~french
     * \brief Modifie l'algo
     * \~english
     * \brief Set algo
     */
    void setAlgo(std::string n_algo){
        algo = n_algo;
    }

    /**
     * \~french
     * \brief Modifie la présence de la pente
     * \~english
     * \brief Set pente
     */
    void setPente(bool n_pente){
        isPente=n_pente;
    }

    /**
     * \~french
     * \brief Renvoie l'algo
     * \~english
     * \brief Get algo
     */
    std::string getAlgo(){
        return algo;
    }

    /**
     * \~french
     * \brief Renvoie la présence de la pente
     * \~english
     * \brief Get pente
     */
    bool getPente(){
        return isPente;
    }

    /**
     * \~french
     * \brief Modifie unit
     * \~english
     * \brief Set unit
     */
    void setUnit(std::string u){
        unit = u;
    }

    /**
     * \~french
     * \brief Renvoie unit
     * \~english
     * \brief Get unit
     */
    std::string getUnit(){
        return unit;
    }

};
#endif

