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
 * \file StoreDataSource.h
 ** \~french
 * \brief Définition des classes StoreDataSource et StoreDataSourceFactory
 * \details
 * \li StoreDataSource : Permet de lire de la donnée quelque soit le type de stockage
 * \li StoreDataSourceFactory : usine de création d'objet StoreDataSource
 ** \~english
 * \brief Define classes StoreDataSource and StoreDataSourceFactory
 * \details
 * \li StoreDataSource : To read data, whatever the storage type
 * \li StoreDataSourceFactory : factory to create StoreDataSource object
 */

#ifndef STOREDATASOURCE_H
#define STOREDATASOURCE_H

#include "Data.h"
#include "Context.h"
#include <stdlib.h>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Lecture d'une source de données
 * \details Cette classe abstraite permet de lire de la donnée quelque soit le contexte de stockage :
 * \li Fichier -> FileContext
 * \li Ceph -> CepĥPoolContext
 * \li Swift -> SwiftContext
 */
class StoreDataSource : public DataSource {

friend class StoreDataSourceFactory;

protected:
    /**
     * \~french \brief Nom de la source de donnée
     * \~english \brief Data source name
     */
    std::string name;

    /**
     * \~french \brief Précise si la source de donnée doit être lue en entier, ou seulement en partie
     * \~english \brief Precise if data source have to be read entirely or just a part
     */
    bool readFull;


    /**
     * \~french \brief Précise si l'index et la taille de la tuile doivent êtr elus ou is nous les avons directement
     * \~english \brief Precise if tile's offset and size have to be read or if we have thme directly
     */
    bool readIndex;

    /**
     * \~french \brief Taille maximale lue dans la source de donnée
     * \~english \brief Max size read in the datasource
     */
    const uint32_t maxsize;

    /**
     * \~french \brief (Position dans la source de) l'offset de la tuile voulue
     * \~english \brief (Position in the data source of) the tile's offset
     */
    const uint32_t posoff;
    /**
     * \~french \brief (Position dans la source de) la taille de la tuile voulue
     * \~english \brief (Position in the data source of) the tile's size
     */
    const uint32_t possize;
    /**
     * \~french \brief Donnée (tuile) voulue
     * \details Si elle est demandée plusieurs fois, on ne va la lire dans la source qu'une seule fois
     * \~english \brief Wanted data (a tile)
     * \details If asked serveral times, data source is read only once
     */
    uint8_t* data;
    /**
     * \~french \brief A-t-on déjà essayé de lire la donnée
     * \~english \brief Have we already tried to read data
     */
    bool alreadyTried;
    /**
     * \~french \brief Taille utile dans #data
     * \~english \brief Real size in #data
     */
    size_t size;
    /**
     * \~french \brief Encodage de la donnée
     * \~english \brief Data encoding
     */
    std::string encoding;
    /**
     * \~french \brief Mime-type de la donnée
     * \~english \brief Data mime-type
     */
    std::string type;

    /**
     * \~french \brief Contexte de stockage de la donnée
     * \~english \brief Data storage context
     */
    Context* context;

    /** \~french
     * \brief Crée un objet StoreDataSource à lecture partielle
     * \details La donnée (tuile) n'est qu'une partie de la source de donnée qui sera lue. On doit passer par l'usine StoreDataSourceFactory.
     * \param[in] name Nom de la source de donnée
     * \param[in] posoff Position de l'offset de la tuile
     * \param[in] possize Position de la taille de la tuile dans la source de donnée
     * \param[in] type Mime-type de la donnée
     * \param[in] c Contexte de stockage de la donnée
     * \param[in] encoding Encodage de la source de donnée
     ** \~english
     * \brief Create a StoreDataSource object, for partially reading.
     * \param[in] name Data source name
     * \param[in] posoff Position of tile's offset
     * \param[in] possize Position of tile's size
     * \param[in] type Data mime-type
     * \param[in] c Data storage context
     * \param[in] encoding Data encoding
     */
    StoreDataSource ( const char* name, bool indexToRead, const uint32_t o, const uint32_t s, std::string type, Context* c, std::string encoding);

    /** \~french
     * \brief Crée un objet StoreDataSource à lecture complète
     * \details La donnée (tuile) est la donnée en entier. On doit passer par l'usine StoreDataSourceFactory.
     * \param[in] name Nom de la source de donnée
     * \param[in] maxsize Position de l'offset de la tuile
     * \param[in] type Mime-type de la donnée
     * \param[in] c Contexte de stockage de la donnée
     * \param[in] encoding Encodage de la source de donnée
     ** \~english
     * \brief Create a StoreDataSource object, for full reading
     * \param[in] name Data source name
     * \param[in] maxsize Max size read in the data source
     * \param[in] type Data mime-type
     * \param[in] c Data storage context
     * \param[in] encoding Data encoding
     */
    StoreDataSource ( const char* name, const uint32_t maxsize, std::string type, Context* c, std::string encoding);

public:


    /** \~french
     * \brief Récupère la donnée depuis la source
     * \details Si la donnée a déjà été lue (#data est déjà instancié), on la retourne directement.
     * 
     * Dans le cas d'une lecture complète, on essaie de lire #maxsize dans la source, et on mémorise la taille effectivement lue
     * 
     * Dans le cas d'une lecture partielle, on lit la position de la donnée (#posoff), la taille de la donnée (#possize), et on lit la tuile.
     * \param[out] tile_size Taille utile dans le buffer pointé en sortie
     * \return Un pointeur vers la donnée
     ** \~english
     * \brief Get the data from the source
     * \details If data is already read (#data is not null), it's returned without re-reading.
     * 
     * For full reading, we try to read #maxsize in data source, and we memorize the real read size.
     * 
     * For partially reading, We read tile's position (#posoff), tile's size (#possize), then we read the data.
     * \param[out] tile_size Real size of data in the returned pointed buffer
     * \return Data pointer
     */
    virtual const uint8_t* getData ( size_t &tile_size );

    /** \~french
     * \brief Récupère le bout de donnée depuis la source
     * \details On dit où et combien de la donnée on veut lire. Celle ci n'est pas mémorisée. Cette fonction instancie le buffer mais à charge de l'appelant de le supprimer
     * \param[in] offset À partir d'où on veut lire la donnée
     * \param[in] size La taille de la donnée que l'on veut lire
     * \return Un pointeur vers la donnée
     ** \~english
     * \brief Get the part of data from the source
     * \details We precise from where and how many bytes we want to read. Data is not memorized. This function instanciate the buffer but the caller have to delete it.
     * \param[in] offset From where we want to read
     * \param[in] size Data size to read
     * \return Data pointer
     */
    virtual uint8_t* getThisData ( const uint32_t offset, const uint32_t size );


    /**
     * \~french \brief Supprime la donnée mémorisée (#data)
     * \~english \brief Delete memorized data (#data)
     */
    bool releaseData() {
        if (data) {
            delete[] data;
        }
        data = 0;
        return true;
    }

    /**
     * \~french \brief Destructeur
     * \details Appelle #releaseData
     * \~english \brief Destructor
     * \details Call #releaseData
     */
    ~StoreDataSource(){
        releaseData();
    }

    /**
     * \~french \brief Retourne 200
     * \~english \brief Return 200
     */
    int getHttpStatus() {
        return 200;
    }

    /**
     * \~french \brief Retourne l'encodage #encoding
     * \~english \brief Return #encoding
     */
    std::string getEncoding() {
        return encoding;
    }

    /**
     * \~french \brief Retourne le mime-type #type
     * \~english \brief Return the mime-type #type
     */
    std::string getType() {
        return type;
    }

};

class StoreDataSourceFactory {

public:

    /** \~french
     * \brief Crée un objet StoreDataSource à lecture partielle
     * \details La donnée (tuile) n'est qu'une partie de la source de donnée qui sera lue.
     * \param[in] name Nom de la source de donnée
     * \param[in] indexToRead Précise si on doit lire la taille et l'offset dans la source ou si ils sont fournis directement
     * \param[in] o (Position de) l'offset de la tuile
     * \param[in] s (Position de) la taille de la tuile dans la source de donnée
     * \param[in] type Mime-type de la donnée
     * \param[in] c Contexte de stockage de la donnée
     * \param[in] encoding Encodage de la source de donnée
     ** \~english
     * \brief Create a StoreDataSource object, for partially reading.
     * \param[in] name Data source name
     * \param[in] indexToRead Precise if offset and size have to be read in source or if it's directly provided
     * \param[in] o (Position of) tile's offset
     * \param[in] s (Position of) tile's size
     * \param[in] type Data mime-type
     * \param[in] c Data storage context
     * \param[in] encoding Data encoding
     */
    StoreDataSource * createStoreDataSource (
        const char* name, bool indexToRead, const uint32_t o, const uint32_t s, std::string type ,
        Context* c, std::string encoding = ""
    );

    /** \~french
     * \brief Crée un objet StoreDataSource à lecture complète
     * \details La donnée (tuile) est la donnée en entier.
     * \param[in] name Nom de la source de donnée
     * \param[in] maxsize Position de l'offset de la tuile
     * \param[in] type Mime-type de la donnée
     * \param[in] c Contexte de stockage de la donnée
     * \param[in] encoding Encodage de la source de donnée
     ** \~english
     * \brief Create a StoreDataSource object, for full reading
     * \param[in] name Data source name
     * \param[in] maxsize Max size read in the data source
     * \param[in] type Data mime-type
     * \param[in] c Data storage context
     * \param[in] encoding Data encoding
     */
    StoreDataSource * createStoreDataSource (
        const char* name, const uint32_t maxsize, std::string type , Context* c, std::string encoding = ""
    );
};

#endif // STOREDATASOURCE_H
