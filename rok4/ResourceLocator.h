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
 * \file ResourceLocator.h
 * \~french
 * \brief Définition de la classe RessourceLocator gérant les liens vers les ressources externe dans les documents de capacités
 * \~english
 * \brief Define the LegendURL Class handling capabilities external link elements
 */

#ifndef RESOURCELOCATOR_H
#define RESOURCELOCATOR_H

#include <string>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance ResourceLocator représente un lien externe dans les différents documents de capacités. 
 * \brief Gestion des éléments de légendes des documents de capacités
 * \~english
 * A ResourceLocator represent an external link element in the differents capabilities documents.
 * \brief Legends handler for the capabilities documents
 */
class ResourceLocator {
private:
    /**
     * \~french \brief Type mime du fichier référencé 
     * \~english \brief Linked file mime type
     */
    std::string format;
    /**
     * \~french \brief Lien vers le fichier sous la forme d'une URL
     * \~english \brief File link as a URL
     */    
    std::string href;
public:
    /**
     * \~french
     * \brief Crée un ResourceLocator à partir des ses éléments constitutifs
     * \param[in] format type mime du fichier référencé
     * \param[in] href lien vers le fichier sous la forme d'une URL
     * \~english
     * \brief Create a ResourceLocator
     * \param[in] format linked file mime type
     * \param[in] href file link as a URL
     */
    ResourceLocator ( std::string format, std::string href );
    /**
     * \~french
     * Crée un ResourceLocator à partir d'un autre 
     * \brief Constructeur de copie
     * \param[in] origRL ResourceLocator à copié
     * \~english
     * Create a ResourceLocator from another
     * \brief Copy Constructor
     * \param[in] origRL ResourceLocator to copy
     */
    ResourceLocator ( const ResourceLocator &origRL );
    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    ResourceLocator& operator= (ResourceLocator const& other);
    /**
     * \~french
     * \brief Test d'egalite de 2 ResourceLocators
     * \return true si tous les attributs sont identiques, false sinon
     * \~english
     * \brief Test whether 2 ResourceLocators are equals
     * \return true if all their attributes are identical
     */
    bool operator== ( const ResourceLocator& other ) const;
    /**
     * \~french
     * \brief Test d'inégalite de 2 ResourceLocators
     * \return true s'ils ont un attribut différent, false sinon
     * \~english
     * \brief Test whether 2 ResourceLocators are different
     * \return true if one of their attributes is different
     */
    bool operator!= ( const ResourceLocator& other ) const;
    
    /**
     * \~french
     * \brief Retourne le type mime du fichier référencé
     * \return hauteur
     * \~english
     * \brief Return the linked file mime type
     * \return height
     */
    inline const std::string getFormat() const {
        return format;
    }
    /**
     * \~french
     * \brief Retourne le lien vers le fichier sous la forme d'une URL
     * \return hauteur
     * \~english
     * \brief Return the file link in URL format
     * \return height
     */
    
    inline const std::string getHRef() const {
        return href;
    }
   
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~ResourceLocator();

};

#endif // RESOURCELOCATOR_H
