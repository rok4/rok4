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
 * \file ContextBook.h
 ** \~french
 * \brief Définition de la classe ContextBook
 * \details
 * \li ContextBook : annuaire de contextes
 ** \~english
 * \brief Define classe ContextBook
 * \details
 * \li ContextBook : book of contexts
 */

#ifndef CONTEXTBOOK_H
#define CONTEXTBOOK_H

#include <map>
#include <utility>
#include "Logger.h"
#include "Context.h"
#include "FileContext.h"

#if BUILD_OBJECT
#include "CephPoolContext.h"
#include "SwiftContext.h"
#include "S3Context.h"
#endif

class ContextBook {

private:

    /**
     * \~french \brief Annuaire de contextes
     * \details La clé est une paire composée du type de stockage et du contenant du contexte
     * \~english \brief Book of contexts
     * \details Key is a pair composed of type of storage and the context's bucket
     */
    //std::map<std::string, Context*> book;
    std::map<std::pair<ContextType::eContextType,std::string>,Context*> book;


public:

    /**
     * \~french
     * \brief Constructeur pour un annuaire de contextes
     */

    ContextBook();

    /**
     * \~french \brief Retourne une chaîne de caracère décrivant l'annuaire
     * \~english \brief Return a string describing the book
     */
    virtual std::string toString() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ Context book -------" << std::endl;
        oss << "\t- context number = " << book.size() << std::endl;

        std::map<std::pair<ContextType::eContextType,std::string>, Context*>::iterator it = book.begin();
        while (it != book.end()) {
            std::pair<ContextType::eContextType,std::string> key = it->first;
            oss << "\t\t- bucket = " << key.first << "/" << key.second << std::endl;
            oss << it->second->toString() << std::endl;
            it++;
        }

        return oss.str() ;
    }

    /**
     * \~french
     * \brief Retourne le context correspondant au contenant demandé
     * \details Si il n'existe pas, une erreur s'affiche et on retourne NULL
     * \param[in] type Type de stockage du contexte rechercé
     * \param[in] tray Nom du contenant pour lequel on veut le contexte
     * \~english
     * \brief Return context of this tray
     * \details If context dosn't exist for this tray, an error is print and NULL is returned
     * \param[in] type storage type of looked for's context 
     * \param[in] tray Tray's name for which context is wanted
     */
    Context* getContext(ContextType::eContextType type,std::string tray);

    /**
     * \~french
     * \brief Ajoute un nouveau contexte
     * \details Si un contexte existe déjà pour ce nom de contenant, on ne crée pas de nouveau contexte et on retourne celui déjà existant. Le nouveau contexte n'est pas connecté.
     * \param[in] type type de stockage pour lequel on veut créer un contexte
     * \param[in] tray Nom du contenant pour lequel on veut créer un contexte
     * \param[in] ctx* contexte à ajouter

     * \brief Add a new context
     * \details If a context already exists for this tray's name, we don't create a new one and the existing is returned. New context is not connected.
     * \param[in] type Storage Type for which context is created
     * \param[in] tray Tray's name for which context is created
     * \param[in] ctx* Context to add
     
     */
    Context * addContext(ContextType::eContextType type,std::string tray);


    /**
     * \~french
     * \brief Nombre de contextes de l'annuaire
     * \~english
     * \brief contexts number in book
     */
    int size();

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~ContextBook();

};

#endif // CONTEXTBOOK_H
