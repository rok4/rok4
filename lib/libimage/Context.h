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
 * \file Context.h
 ** \~french
 * \brief Définition de la classe Context
 * \details Classe d'abstraction du contexte de stockage (fichier, ceph s3 ou swift)
 ** \~english
 * \brief Define classe Context
 * \details Storage context abstraction
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <map>
#include <stdint.h>// pour uint8_t
#include "Logger.h"
#include <string.h>
#include <sstream>


/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées au format de canal
 * \~english \brief Manage informations in connection with sample format
 */
namespace ContextType {
/**
 * \~french \brief Énumération des types de contextes
 * \~english \brief Available context type
 */
enum eContextType {
    FILECONTEXT,
    CEPHCONTEXT,
    SWIFTCONTEXT,
    S3CONTEXT
};

/**
 * \~french \brief Conversion d'un type de contexte vers une chaîne de caractères
 * \param[in] ct type de contexte à convertir
 * \return la chaîne de caractère nommant le type de contexte
 * \~english \brief Convert a context type to a string
 * \param[in] ct context type to convert
 * \return string namming the context type
 */
std::string toString ( eContextType ct );

}

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un contexte de stockage abstrait 
 */
class Context {  

protected:

    /**
     * \~french \brief Buffers pour les écritures différées
     * \~english \brief Postponed writings buffers
     */
    std::map<std::string, std::vector<char>*> writingBuffers;

    /**
     * \~french \brief Précise si le contexte est connecté
     * \~english \brief Precise if context is connected
     */
    bool connected;

    /**
     * \~french \brief Nombre de tentatives pour en lecture ou écriture
     * \~english \brief Attempts number to read or write
     */
    int attempts;

    /**
     * \~french \brief Crée un objet Context
     * \~english \brief Create a Context object
     */
    Context () : connected(false), attempts(1) {  }

public:


    /**
     * \~french \brief Modifie le nombre de tentative
     * \~english \brief Change attempts number
     */
    void setAttempts (int a) {
        if (a < 1) a = 1;
        attempts = a;
    }

    /**
     * \~french \brief Connecte le contexte
     * \~english \brief Connect the context
     */
    virtual bool connection() = 0;

    /**
     * \~french \brief Précise si l'objet demandé existe dans ce contexte
     * \param[in] name Nom de l'objet dont on veut savoir l'existence 
     * \~english \brief Precise if provided object exists in this context
     * \param[in] name Object's name whose existency is asked
     */
    bool exists(std::string name) {
        uint8_t test;
        return (read(&test, 0, 1, name) == 1);
    }

    /**
     * \~french \brief Précise si le contexte est connecté
     * \~english \brief Precise if context is connected
     */
    bool isConnected() {
        return connected;
    }

    /**
     * \~french \brief Récupère la donnée dans l'objet
     * \param[in,out] data Buffer où stocker la donnée lue. Doit être initialisé et assez grand
     * \param[in] offset À partir d'où on veut lire
     * \param[in] size Nombre d'octet que l'on veut lire
     * \param[in] name Nom de l'objet que l'on veut lire
     * \return Taille effectivement lue, un nombre négatif en cas d'erreur
     * \~english \brief Get the data in the named object
     * \param[in,out] data Buffer where to store read data. Have to be initialized
     * \param[in] offset From where we want to read
     * \param[in] size Number of bytes we want to read
     * \param[in] name Object's name we want to read
     * \return Real size of read data, negative integer if an error occured
     */
    virtual int read(uint8_t* data, int offset, int size, std::string name) = 0;

    /**
     * \~french \brief Écrit de la donnée dans l'objet
     * \param[in] data Buffer contenant la donnée à écrire
     * \param[in] offset À partir d'où on veut écrire
     * \param[in] size Nombre d'octet que l'on veut écrire
     * \param[in] name Nom de l'objet dans lequel on veut écrire
     * \~english \brief Write data in the named object
     * \param[in] data Buffer with data to write
     * \param[in] offset From where we want to write
     * \param[in] size Number of bytes we want to write
     * \param[in] name Object's name we want to write into
     */
    virtual bool write(uint8_t* data, int offset, int size, std::string name) = 0;

    /**
     * \~french \brief Écrit intégralement un objet
     * \param[in] data Buffer contenant la donnée à écrire
     * \param[in] size Nombre d'octet que l'on veut écrire
     * \param[in] name Nom de l'objet à écrire
     * \~english \brief Write an object full
     * \param[in] data Buffer with data to write
     * \param[in] size Number of bytes we want to write
     * \param[in] name Object's name to write
     */
    virtual bool writeFull(uint8_t* data, int size, std::string name) = 0;

    /**
     * \~french \brief Prépare l'objet en écriture
     * \param[in] name Nom de l'objet dans lequel on va vouloir écrire
     * \~english \brief Write data in the named object
     * \param[in] name Object's name we want to write into
     */
    virtual bool openToWrite(std::string name) = 0;
    /**
     * \~french \brief Termine l'écriture d'un objet
     * \param[in] name Nom de l'objet dans lequel on a voulu écrire
     * \~english \brief Stop the writing
     * \param[in] name Object's name we wanted to write into
     */
    virtual bool closeToWrite(std::string name) = 0;

    /**
     * \~french \brief Retourne le type du contexte
     * \~english \brief Return the context's type
     */
    virtual ContextType::eContextType getType() = 0;
    /**
     * \~french \brief Retourne le type du contexte, sous forme de texte
     * \~english \brief Return the context's type, as string
     */
    virtual std::string getTypeStr() = 0;

    /**
     * \~french \brief Retourne le contenant dans le contexte
     * \~english \brief Return the tray in the context
     */
    virtual std::string getTray() = 0;

    /**
     * \~french \brief Retourne le chemin pour une tuile X/Y relatif à ce contexte
     * \~english \brief Return the path for a tile (X/Y) in this context
     */
    virtual std::string getPath(std::string racine,int x,int y,int pathDepth = 2) = 0;

    /**
     * \~french \brief Sortie des informations sur le contexte
     * \~english \brief Context description output
     */
    virtual void print() = 0;

    /**
     * \~french \brief Retourne une chaîne de caracère décrivant le contexte
     * \~english \brief Return a string describing the context
     */
    virtual std::string toString() = 0;

    /**
     * \~french \brief Déconnecte le contexte
     * \~english \brief Disconnect the context
     */
    virtual void closeConnection() = 0;

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    virtual ~Context() {
        std::map<std::string,std::vector<char>*>::iterator it;
        for (it = writingBuffers.begin(); it != writingBuffers.end(); ++it) {
            delete it->second;
        }
    }
};

#endif
