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
 * \file StoreDataSource.cpp
 ** \~french
 * \brief Implémentation des classes StoreDataSource et StoreDataSourceFactory
 * \details
 * \li StoreDataSource : Permet de lire de la donnée quelque soit le type de stockage
 * \li StoreDataSourceFactory : usine de création d'objet StoreDataSource
 ** \~english
 * \brief Implements classes StoreDataSource and StoreDataSourceFactory
 * \details
 * \li StoreDataSource : To read data, whatever the storage type
 * \li StoreDataSourceFactory : factory to create StoreDataSource object
 */

#include "StoreDataSource.h"
#include <fcntl.h>
#include "Logger.h"
#include <cstdio>
#include <errno.h>
#include "Rok4Image.h"

// Taille maximum d'une tuile WMTS
#define MAX_TILE_SIZE 1048576

StoreDataSource::StoreDataSource (std::string n, const uint32_t o, const uint32_t s, std::string type, Context* c, std::string encoding ) :
    name ( n ), posoff(o), possize(s), maxsize(0), headerIndexSize(0), type (type), encoding( encoding ), context(c)
{
    data = NULL;
    size = 0;
    readFull = false;
    readIndex = false;
    alreadyTried = false;
}

StoreDataSource::StoreDataSource (std::string n, const uint32_t po, const uint32_t ps, const uint32_t hisize, std::string type, Context* c, std::string encoding ) :
    name ( n ), posoff(po), possize(ps), maxsize(0), headerIndexSize(hisize), type (type), encoding( encoding ), context(c)
{
    data = NULL;
    size = 0;
    readFull = false;
    readIndex = true;
    alreadyTried = false;
}

StoreDataSource::StoreDataSource (std::string n, const uint32_t maxsize, std::string type, Context* c, std::string encoding ) :
    name ( n ), posoff ( 0 ), possize ( 0 ), maxsize(maxsize), headerIndexSize(0), type (type), encoding( encoding ), context(c)
{
    data = NULL;
    size = 0;
    readFull = true;
    readIndex = false;
    alreadyTried = false;
}

/*
 * Fonction retournant les données de la tuile
 * Le fichier/objet ne doit etre lu qu une seule fois
 * Indique la taille de la tuile (inconnue a priori)
 */
const uint8_t* StoreDataSource::getData ( size_t &tile_size ) {
    if ( alreadyTried) {
        tile_size = size;
        return data;
    }

    alreadyTried = true;

    // il se peut que le contexte ne soit pas connecté, auquel cas on sort directement sans donnée
    if (! context->isConnected()) {
        data = NULL;
        return NULL;
    }

    if (readFull) {
        // On retourne tout l'objet
        data = new uint8_t[maxsize];
        int tileSize = context->read(data, 0, maxsize, name);
        if (tileSize < 0) {
            LOGGER_ERROR ( "Erreur lors de la lecture de la tuile = objet " << name );
            delete[] data;
            data = NULL;
            return NULL;
        }
        tile_size = tileSize;
        size = tileSize;
    }
    else if (! readIndex) {
        // On a directement la taille et l'offset
        data = new uint8_t[possize];
        int tileSize = context->read(data, posoff, possize, name);
        if (tileSize < 0) {
            LOGGER_ERROR ( "Erreur lors de la lecture de la tuile dans l'objet (sans passer par l'index) " << name );
            delete[] data;
            data = NULL;
            return NULL;
        }
        tile_size = tileSize;
        size = tileSize;
    }
    else {

        uint8_t* indexheader = new uint8_t[headerIndexSize];
        int realSize = context->read(indexheader, 0, headerIndexSize, name);

        if ( realSize < 0) {
            LOGGER_ERROR ( "Erreur lors de la lecture du header et de l'index dans l'objet/fichier " << name );
            delete[] indexheader;
            return NULL;
        }

        if ( realSize < ROK4_IMAGE_HEADER_SIZE ) {

            // Dans le cas d'un header de type objet lien, on verifie d'abord que la signature concernée est bien presente dans le header de l'objet
            if ( strncmp((char*) indexheader, ROK4_SYMLINK_SIGNATURE, ROK4_SYMLINK_SIGNATURE_SIZE) != 0 ) {
                LOGGER_ERROR ( "Erreur lors de la lecture du header, l'objet " << name << " ne correspond pas à un objet lien " );
                delete[] indexheader;
                return NULL;
            }

            // On est dans le cas d'un objet symbolique
            std::string originalName (name);
            char tmpName[realSize-ROK4_SYMLINK_SIGNATURE_SIZE+1];
            memcpy((uint8_t*) tmpName, indexheader+ROK4_SYMLINK_SIGNATURE_SIZE,realSize-ROK4_SYMLINK_SIGNATURE_SIZE);
            tmpName[realSize-ROK4_SYMLINK_SIGNATURE_SIZE] = '\0';
            name = std::string (tmpName);

            LOGGER_DEBUG ( "Dalle symbolique détectée : " << originalName << " référence une autre dalle symbolique " << name );

            int realSize = context->read(indexheader, 0, headerIndexSize, name);

            if ( realSize < 0) {
                LOGGER_ERROR ( "Erreur lors de la lecture du header et de l'index dans l'objet/fichier " << name );
                delete[] indexheader;
                return NULL;
            }
            if ( realSize < ROK4_IMAGE_HEADER_SIZE ) {
                LOGGER_ERROR ( "Erreur lors de la lecture : une dalle symbolique " << originalName << " référence une autre dalle symbolique " << name );
                delete[] indexheader;
                return NULL;
            }
        }

        // On est dans le cas d'une dalle
        uint32_t tileOffset = *((uint32_t*) (indexheader + posoff ));
        uint32_t tileSize = *((uint32_t*) (indexheader + possize ));

        // La taille de la tuile ne doit pas exceder un seuil
        // Objectif : gerer le cas de fichiers TIFF non conformes aux specs du cache
        // (et qui pourraient indiquer des tailles de tuiles excessives)

        if ( tileSize > MAX_TILE_SIZE ) {
            LOGGER_ERROR ( "Tuile trop volumineuse dans le fichier/objet " << name ) ;
            delete[] indexheader;
            return NULL;
        }

        // Lecture de la tuile
        data = new uint8_t[tileSize];
        if (context->read(data, tileOffset, tileSize, name) < 0) {
            delete[] data;
            data = NULL;
            LOGGER_ERROR ( "Erreur lors de la lecture de la tuile dans l'objet " << name );
            delete[] indexheader;
            return NULL;
        }
        delete[] indexheader;

        tile_size = tileSize;
        size = tileSize;
    }

    return data;

}
