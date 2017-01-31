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
 * \file AliasManager.h
 ** \~french
 * \brief Définition de la classe AliasManager
 * \details
 * \li AliasManager : classe d'abstraction du gestionnaire d'alias (redis)
 ** \~english
 * \brief Define classe AliasManager
 * \details
 * \li AliasManager : alias manager abstraction
 */

#ifndef ALIASMANAGER_H
#define ALIASMANAGER_H

#include <stdint.h>// pour uint8_t
#include "Logger.h"
#include <string.h>
#include <sstream>

/**
 * \~french \brief Énumération des types de gestionnaires d'alias
 * \~english \brief Available alias manager type
 */
enum eAliasManagerType {
    REDISDATABASE
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un gestionnaire d'alias abstrait 
 */
class AliasManager {  

protected:

    bool ok;

    /**
     * \~french \brief Crée un objet AliasManager
     * \~english \brief Create a AliasManager object
     */
    AliasManager () {}

public:

    /**
     * \~french \brief Retourne la nom correspondant à l'alias si il existe
     * \param[in] alias Nom de l'alias
     * \param[out] exists L'alias existe-t-il ?
     * \~english \brief Return name referenced by the alias, if exists
     * \param[in] alias Alias' name
     * \param[out] exists Does alias exist ?
     */
    virtual std::string getAliasedName(std::string alias, bool* exists) = 0;

    /**
     * \~french \brief Retourne le type du gestionnaire d'alias
     * \~english \brief Return the alias manager's type
     */
    virtual eAliasManagerType getType() = 0;
    /**
     * \~french \brief Retourne le type du gestionnaire d'alias, en string
     * \~english \brief Return the alias manager's type, as string
     */
    virtual std::string getTypeStr() = 0;


    bool isOk() {return ok;}

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    virtual ~AliasManager() {}
};

#endif
