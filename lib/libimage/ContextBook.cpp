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
 * \file ContextBook.cpp
 ** \~french
 * \brief Définition de la classe ContextBook
 * \details
 * \li ContextBook : annuaire de contextes
 ** \~english
 * \brief Define classe ContextBook
 * \details
 * \li ContextBook : Directory of contexts
 */

#include "ContextBook.h"

ContextBook::ContextBook(std::string name, std::string user, std::string conf)
{

    bCeph = true;
    bSwift = false;
    ceph_name = name;
    ceph_user = user;
    ceph_conf = conf;

}

ContextBook::ContextBook(std::string auth, std::string account, std::string user, std::string passwd)
{

    bSwift = true;
    bCeph = false;
    swift_auth = auth;
    swift_account = account;
    swift_user = user;
    swift_passwd = passwd;

}

Context * ContextBook::addContext(std::string pool)
{
    Context* ctx;
    std::map<std::string, Context*>::iterator it = book.find ( pool );
    if ( it != book.end() ) {
        //le pool est déjà existant et donc connecté
        return it->second;

    } else {
        //ce pool n'est pas encore connecté, on va créer la connexion
        if (bCeph) {
            ctx = new CephPoolContext(ceph_name, ceph_user, ceph_conf, pool);
        } else if (bSwift) {
            ctx = new SwiftContext(swift_auth,swift_account,swift_user,swift_passwd,pool);
        } else {
            return NULL;
        }

        //on se connecte
//        if (!ctx->connection()) {
//            LOGGER_ERROR("Impossible de se connecter aux donnees.");
//            return NULL;
//        }
        //on ajoute au book
        book.insert ( std::pair<std::string,Context*>(pool,ctx) );

        return ctx;
    }

}

Context * ContextBook::getContext(std::string pool)
{

    std::map<std::string, Context*>::iterator it = book.find ( pool );
    if ( it == book.end() ) {
        LOGGER_ERROR("Le pool demandé n'a pas été trouvé dans l'annuaire.");
        return NULL;
    } else {
        //le pool est déjà existant et donc connecté
        return it->second;
    }

}

ContextBook::~ContextBook()
{
    std::map<std::string,Context*>::iterator it;
    for (it=book.begin(); it!=book.end(); ++it) {
        delete it->second;
        it->second = NULL;
    }

}

bool ContextBook::connectAllContext()
{
    std::map<std::string,Context*>::iterator it;
    for (it=book.begin(); it!=book.end(); ++it) {
        if (!(it->second->connection())) {
            return false;
        }
    }
    return true;
}

void ContextBook::disconnectAllContext()
{
    std::map<std::string,Context*>::iterator it;
    for (it=book.begin(); it!=book.end(); ++it) {
        it->second->closeConnection();
    }
}

