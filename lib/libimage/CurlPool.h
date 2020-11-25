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
 * \file CurlPool.h
 ** \~french
 * \brief Définition de la classe CurlPool
 ** \~english
 * \brief Define class CurlPool
 */

#ifndef CURLPOOL_H
#define CURLPOOL_H

#include <stdint.h>// pour uint8_t
#include "Logger.h"
#include <map>
#include <string.h>
#include <sstream>
#include <curl/curl.h>


/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un pool
 * \details Cette classe est prévue pour être utilisée sans instance
 */
class CurlPool {  

private:

    /**
     * \~french \brief Annuaire des objet Curl
     * \details La clé est l'identifiant du thread
     * \~english \brief Curl object book
     * \details Key is the thread's ID
     */
    static std::map<pthread_t, CURL*> pool;

    /**
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    CurlPool(){};

public:

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~CurlPool(){};

    /**
     * \~french \brief Retourne un objet Curl propre au thread appelant
     * \details Si il n'existe pas encore d'objet curl pour ce tread, on le crée et on l'initialise
     * \~english \brief Get the curl object specific to the calling thread
     * \details If curl object doesn't exist for this thread, it is created and initialized
     */
    static CURL* getCurlEnv() {
        pthread_t i = pthread_self();

        std::map<pthread_t, CURL*>::iterator it = pool.find ( i );
        if ( it == pool.end() ) {
            CURL* c = curl_easy_init();
            pool.insert ( std::pair<pthread_t, CURL*>(i,c) );
            return c;
        } else {
            return it->second;
        }
    }

    /**
     * \~french \brief Affiche le nombre d'objet curl dans l'annuaire
     * \~english \brief Print the number of curl objects in the book
     */
    static void printNumCurls () {
        LOGGER_INFO("Nombre de contextes curl : " << pool.size());
    }

    /**
     * \~french \brief Nettoie tous les objets curl dans l'annuaire et le vide
     * \~english \brief Clean all curl objects in the book and empty it
     */
    static void cleanCurlPool () {
        std::map<pthread_t, CURL*>::iterator it;
        for (it = pool.begin(); it != pool.end(); ++it) {
            curl_easy_cleanup(it->second);
        }
        pool.clear();
    }

};

#endif
