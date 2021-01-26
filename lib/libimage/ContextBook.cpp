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


ContextBook::ContextBook(){}

Context * ContextBook::addContext(ContextType::eContextType type,std::string tray)
{
    Context* ctx;
    std::pair<ContextType::eContextType,std::string> key = make_pair(type,tray);
    LOGGER_DEBUG("On essaye d'ajouter la clé " << ContextType::toString(key.first) <<" / " << key.second );

    std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = book.find (key);
    if ( it != book.end() ) {
        //le contenant est déjà existant et donc connecté
        return it->second;

    } else {
        //ce contenant n'est pas encore connecté, on va créer la connexion
        //
        //on créé le context selon le type de stockage
        switch(type){
            case ContextType::SWIFTCONTEXT:
                ctx = new SwiftContext(tray);
                break;
            case ContextType::CEPHCONTEXT:
                ctx = new CephPoolContext(tray);
                break;
            case ContextType::S3CONTEXT:
                ctx = new S3Context(tray);
                break;
            case ContextType::FILECONTEXT:
                ctx = new FileContext(tray);
                break;
            default:
                //ERREUR
                LOGGER_ERROR("Ce type de contexte n'est pas géré.");
                return NULL;
        }

        // on connecte pour vérifier que ce contexte est valide
        if (!(ctx->connection())) {
            LOGGER_ERROR("Impossible de connecter au contexte de type " << ContextType::toString(type) << ", contenant " << tray);
            delete ctx;
            return NULL;
        }


        //LOGGER_DEBUG("On insère ce contexte " << ctx->toString() );
        book.insert(make_pair(key,ctx));

        return ctx;
    }

}

Context * ContextBook::getContext(ContextType::eContextType type,std::string tray)
{
    std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = book.find (make_pair(type,tray));
    if ( it == book.end() ) {
        LOGGER_ERROR("Le contenant demandé n'a pas été trouvé dans l'annuaire.");
        return NULL;
    } else {
        //le contenant est déjà existant et donc connecté
        return it->second;
    }

}

ContextBook::~ContextBook()
{
    std::map<std::pair<ContextType::eContextType,std::string>,Context*>::iterator it;
    for (it=book.begin(); it!=book.end(); ++it) {
        delete it->second;
        it->second = NULL;
    }
}

int ContextBook::size(){
  return book.size();
}

