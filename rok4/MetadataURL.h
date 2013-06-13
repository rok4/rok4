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
 * \file MetadataURL.h
 * \~french
 * \brief Définition de la classe MetadataURL gérant les liens vers les méta-données dans les documents de capacités
 * \~english
 * \brief Define the MetadataURL Class handling capabilities metadata link elements
 */

#ifndef METADATAURL_H
#define METADATAURL_H
#include "ResourceLocator.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance MetadataURL représente un lien vers des méta-données dans les différents documents de capacités.
 * \brief Gestion des éléments de méta-données des documents de capacités
 * \~english
 * A MetadataURL represent a metadata link element in the differents capabilities documents.
 * \brief Metadata handler for the capabilities documents
 */
class MetadataURL : public ResourceLocator {
private:
    /**
     * \~french \brief Type de la méta-donnée
     * \~english \brief Metadata type
     */
    std::string type;

public:
    /**
     * \~french
     * \brief Crée un MetadataURL à partir des ses éléments constitutifs
     * \param[in] format type mime du fichier référencé (cf ResourceLocator::format)
     * \param[in] href lien vers le fichier sous la forme d'une URL (cf ResourceLocator::href)
     * \param[in] type type de la méta-donnée
     * \~english
     * \brief Create a MetadataURL
     * \param[in] format linked file mime type
     * \param[in] href file link as a URL
     * \param[in] type metadata type
     */
    MetadataURL ( std::string format, std::string href, std::string type );
    /**
     * \~french
     * Crée un MetadataURL à partir d'un autre
     * \brief Constructeur de copie
     * \param[in] origMtdUrl MetadataURL à copier
     * \~english
     * Create a MetadataURL from another
     * \brief Copy Constructor
     * \param[in] origMtdUrl MetadataURL to copy
     */
    MetadataURL ( const MetadataURL & origMtdUrl );
    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    MetadataURL& operator= ( MetadataURL const& other );
    /**
     * \~french
     * \brief Test d'egalite de 2 MetadataURLs
     * \return true si tous les attributs sont identiques, false sinon
     * \~english
     * \brief Test whether 2 MetadataURLs are equals
     * \return true if all their attributes are identical
     */
    bool operator== ( const MetadataURL& other ) const;
    /**
     * \~french
     * \brief Test d'inégalite de 2 MetadataURLs
     * \return true s'ils ont un attribut différent, false sinon
     * \~english
     * \brief Test whether 2 MetadataURLs are different
     * \return true if one of their attributes is different
     */
    bool operator!= ( const MetadataURL& other ) const;

    /**
     * \~french
     * \brief Retourne le type de la méta donnée
     * \return type
     * \~english
     * \brief Return the metadata type
     * \return type
     */
    inline std::string getType() {
        return type;
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~MetadataURL();
};

#endif // METADATAURL_H
