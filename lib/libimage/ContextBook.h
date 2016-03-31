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
 * \file ContextBook.h
 ** \~french
 * \brief Définition de la classe ContextBook
 * \details
 * \li ContextBook : annuaire de contextes
 ** \~english
 * \brief Define classe ContextBook
 * \details
 * \li ContextBook : Directory of contexts
 */

#ifndef CONTEXTBOOK_H
#define CONTEXTBOOK_H

#include <rados/librados.h>
#include <map>
#include "Logger.h"
#include "Context.h"
#include "CephPoolContext.h"
#include "SwiftContext.h"

class ContextBook {

private:

    /**
     * \~french \brief Contexte de base
     * Issu de la lecture du server.conf
     * Contient les infrmations utile pour se connecter à un cluster de données
     * \~english \brief Base Context
     */
    Context* baseContext;

    /**
     * \~french \brief Annuaire de contextes utiles
     * \~english \brief Directory of contexts
     */
    std::map<std::string, Context*> book;

public:

    /**
     * \~french
     * \brief Constructeur pour un contexte Ceph
     * \~english
     * \brief Constructor for CephContext
     */
    ContextBook(std::string name, std::string user, std::string conf, std::string pool);

    /**
     * \~french
     * \brief Retourne le baseContext
     * \~english
     * \brief Return baseContext
     */
    Context* getBaseContext() {
        return baseContext;
    }

    bool connectContexts() {
        std::map<std::string, Context*>::iterator it = book.begin();
        while (it != book.end()) {
            if (! it->second->connection()) {
                LOGGER_ERROR("Impossible de se connecter le contexte Ceph de pool " << it->first);
                return false;
            }
            it++;
        }
        return true;
    }

    /**
     * \~french
     * \brief Retourne le baseContext si c'est du Ceph
     * \~english
     * \brief Return baseContext if CEPHCONTEXT
     */
    CephPoolContext* getCephBaseContext() {
        if (baseContext->getType() == CEPHCONTEXT) {
            return reinterpret_cast<CephPoolContext*> (baseContext);
        } else {
            return NULL;
        }
    }

    virtual std::string toString() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ Context book -------" << std::endl;
        oss << "\t- context number = " << book.size() << std::endl;

        std::map<std::string, Context*>::iterator it = book.begin();
        while (it != book.end()) {
            oss << "\t\t- pool = " << it->first << std::endl;
            oss << it->second->toString() << std::endl;
            it++;
        }

        return oss.str() ;
    }

    /**
     * \~french
     * \brief Retourne le context correspondant au pool demandé
     * \~english
     * \brief Return context of this pool
     */
    Context* getContext(std::string pool);

    /**
     * \~french
     * \brief Verifie si un contexte existe et l'ajoute si non
     * \~english
     * \brief Check if a context exists and add it if it is not exist
     */
    Context * addContext(std::string pool);


    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~ContextBook();


};

#endif // CONTEXTBOOK_H
