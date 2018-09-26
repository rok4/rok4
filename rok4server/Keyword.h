/*
 * Copyright © (2011-2013) Institut national de l'information
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
 * \file Keyword.h
 * \~french
 * \brief Définition de la classe Keyword gérant les mots-clés des documents de capacités
 * \~english
 * \brief Define the Keyword Class handling capabilities keywords
 */

#ifndef KEYWORD_H
#define KEYWORD_H
#include <string>
#include <map>

typedef std::pair<std::string,std::string> attribute;

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Un Keyword représente un élément Keyword dans les différents documents de capacités.
 * Une valeur textuelle le définit ainsi qu'une liste d'attributs.
 * \brief Gestion des mots-clés pour les documents de capacités
 * \~english
 * A Keyword represent a Keyword element in the differents capabilities documents.
 * A text value and an attributes list define it.
 * \brief Keywords handler for the capabilities documents
 */
class Keyword {
private:
    /**
     * \~french \brief Contenu de l'élément XML
     * \~english \brief XML element content
     */
    std::string content;
    /**
     * \~french \brief Liste d'attribut de l'élément XML
     * \~english \brief XML attributes list
     */
    std::map<std::string,std::string> attributes;
public:
    /**
     * \~french
     * \brief Crée un Keyword à partir des ses éléments constitutifs
     * \param[in] content Valeur du mot-clé
     * \param[in] attributes liste des attributs du mot-clé (clé-valeur)
     * \~english
     * \brief Create a Keyword
     * \param[in] content value
     * \param[in] attributes attributes list in KVP
     */
    Keyword ( std::string content, std::map<std::string,std::string> attributes );
    /**
     * \~french
     * Crée un Keyword à partir d'un autre Keyword
     * \brief Constructeur de copie
     * \param[in] origKW keyword à copier
     * \~english
     * Create a Keyword from another
     * \brief Copy Constructor
     * \param[in] origKW keyword to copy
     */
    Keyword ( const Keyword &origKW );

    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    Keyword& operator= ( Keyword const& other );
    /**
     * \~french
     * \brief Test d'egalite de 2 keywords
     * \return true s'ils ont la même valeur, false sinon
     * \~english
     * \brief Test whether 2 Keywords are equals
     * \return true if they share the value
     */
    bool operator== ( const Keyword& other ) const;
    /**
     * \~french
     * \brief Test d'inégalite de 2 keywords
     * \return true s'ils ont une valeur différente, false sinon
     * \~english
     * \brief Test whether 2 Keywords are different
     * \return true if their values are different
     */
    bool operator!= ( const Keyword& other ) const;

    /**
     * \~french
     * \brief Retourne le mot-clé
     * \return valeur du mot-clé
     * \~english
     * \brief Return the keyword text
     * \return keyword value
     */
    inline const std::string getContent() const {
        return content;
    }

    /**
     * \~french
     * \brief Retourne la liste des attributs du mot-clé
     * \return liste des attributs
     * \~english
     * \brief Return the attributes list
     * \return attributes list
     */
    inline const std::map<std::string,std::string>* getAttributes() const {
        return &attributes;
    }

    /**
     * \~french
     * \brief Teste si le mot-clé possède des attributs
     * \return true si il a au moins 1 attribut
     * \~english
     * \brief Test whether the keyword has attributes
     * \return true if it has at least one
     */
    inline bool hasAttributes() const {
        return ! ( attributes.empty() );
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Keyword();

};

#endif // KEYWORD_H
