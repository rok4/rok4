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
 * \li ContextBook : book of contexts
 */

#ifndef CONTEXTBOOK_H
#define CONTEXTBOOK_H

#include <map>
#include "Logger.h"
#include "Context.h"
#include "CephPoolContext.h"
#include "SwiftContext.h"
#include "S3Context.h"

class ContextBook {

private:

    /**
     * \~french \brief Annuaire de contextes
     * \details La clé est le contenant du contexte
     * \~english \brief Book of contexts
     * \details Key is the context's bucket
     */
    std::map<std::string, Context*> book;

    /**
     * \~french \brief Précise le type des contextes de l'annuaire
     * \~english \brief Précise book type
     */
    eContextType contextType;

    /**
     * \~french \brief Nom par défaut du cluster ceph pour les nouveaux contextes
     * \~english \brief Default name of ceph cluster for new contexts
     */
    std::string ceph_name;
    /**
     * \~french \brief Nom par défaut de l'utilisateur ceph pour les nouveaux contextes
     * \~english \brief Default name of ceph user for new contexts
     */
    std::string ceph_user;
    /**
     * \~french \brief Configuration ceph par défaut pour les nouveaux contextes
     * \~english \brief Default ceph configuration file for new contexts
     */
    std::string ceph_conf;

    /**
     * \~french \brief Url par défaut pour les nouveaux contextes s3
     * \~english \brief Default url for new s3 contexts
     */
    std::string s3_url;
    /**
     * \~french \brief Clé par défaut pour les nouveaux contextes s3
     * \~english \brief Default key for new s3 contexts
     */
    std::string s3_key;
    /**
     * \~french \brief Clé secrète par défaut pour les nouveaux contextes s3
     * \~english \brief Default secret key for new s3 contexts
     */
    std::string s3_secret_key;

    /**
     * \~french \brief Url d'authehtification par défaut pour les nouveaux contextes swift
     * \~english \brief Default authentication url for new swift contexts
     */
    std::string swift_auth;
    /**
     * \~french \brief Utilisateur par défaut pour les nouveaux contextes swift
     * \~english \brief Default user for new swift contexts
     */
    std::string swift_user;
    /**
     * \~french \brief Mot de passe par défaut pour les nouveaux contextes swift
     * \~english \brief Default password for new swift contexts
     */
    std::string swift_passwd;

    /**
     * \~french \brief Gestionnaire d'alias, pour convertir les noms de fichier/objet
     * \~english \brief Alias manager to convert file/object name
     */
    AliasManager* am;

public:

    /**
     * \~french
     * \brief Constructeur pour un annuaire de contextes Ceph S3 ou SWIFT
     * \param[in] type Type des contextes de l'annuaire
     * \param[in] s1 
     * \param[in] s2 
     * \param[in] s3 
     * \~english
     * \brief Constructor for a ceph, s3 or swift context book
     * \param[in] type Book type
     * \param[in] s1
     * \param[in] s2
     * \param[in] s3
     */
    ContextBook(eContextType type, std::string s1, std::string s2, std::string s3);


    /**
     * \~french \brief Retourne une chaîne de caracère décrivant l'annuaire
     * \~english \brief Return a string describing the book
     */
    virtual std::string toString() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ Context book -------" << std::endl;
        oss << "\t- context number = " << book.size() << std::endl;

        std::map<std::string, Context*>::iterator it = book.begin();
        while (it != book.end()) {
            oss << "\t\t- bucket = " << it->first << std::endl;
            oss << it->second->toString() << std::endl;
            it++;
        }

        return oss.str() ;
    }

    /**
     * \~french
     * \brief Retourne le context correspondant au contenant demandé
     * \details Si il n'existe pas, une erreur s'affiche et on retourne NULL
     * \param[in] bucket Nom du contenant pour lequel on veut le contexte
     * \~english
     * \brief Return context of this bucket
     * \details If context dosn't exist for this bucket, an error is print and NULL is returned
     * \param[in] bucket Bucket's name for which context is wanted
     */
    Context* getContext(std::string bucket);

    /**
     * \~french
     * \brief Ajoute un nouveau contexte
     * \details Si un contexte existe déjà pour ce nom de contenant, on ne crée pas de nouveau contexte et on retourne celui déjà existant. Le nouveau contexte n'est pas connecté.
     * \param[in] bucket Nom du contenant pour lequel on veut créer un contexte
     * \~english
     * \brief Add a new context
     * \details If a context already exists for this bucket's name, we don't create a new one and the existing is returned. New context is not connected.
     * \param[in] bucket Bucket's name for which context is created
     */
    Context * addContext(std::string bucket, bool keystone = false);

    /**
     * \~french
     * \brief Précise le gestionnaire d'alias
     * \~english
     * \brief Set the alias manager
     */
    void setAliasManager(AliasManager* a) {
        am = a;
    }

    /**
     * \~french
     * \brief Connecte l'ensemble des contextes de l'annuaire
     * \~english
     * \brief Connect all contexts
     */
    bool connectAllContext();

    /**
     * \~french
     * \brief Deconnecte l'ensemble des contextes de l'annuaire
     * \~english
     * \brief Disconnect all contexts
     */
    void disconnectAllContext();


    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~ContextBook();

};

#endif // CONTEXTBOOK_H
