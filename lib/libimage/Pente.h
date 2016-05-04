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

public:

        /** \~french
     * \brief resolution : algo : choix de l'algorithme de calcul de pentes par l'utilisateur (vaut "Z" pour Zevenbergen&Thorne, "H" pour Horn)
     ** \~english
     * \brief resolution : algo : slope calculation algorithm chosen by the user (value = "Z" for Zevenbergen&Thorne OR "H" for Horn)
     */

    std::string algo;
    bool isPente;

public:

    Pente(): algo ("H"), isPente (false) {

    }

    Pente(std::string a,bool p = false): algo (a), isPente (p) {

    }

    ~Pente() {

    }

    bool empty(){
        if (isPente) {return false;}
        else {return true;}
        }

    void setAlgo(std::string n_algo){
        algo = n_algo;
        }

    void setPente(bool n_pente){
        isPente=n_pente;
        }

};
#endif

