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

#ifndef EMPTYDATASOURCE_H
#define EMPTYDATASOURCE_H

#include <stdint.h>// pour uint8_t
#include <cstddef> // pour size_t
#include <string>  // pour std::string
#include <cstring> // pour memcpy

#include "Logger.h"
#include "Data.h"
#include "EmptyImage.h"
#include "TiffEncoder.h"
#include "PNGEncoder.h"
#include "JPEGEncoder.h"
#include "BilEncoder.h"
#include "Format.h"

/**
 * Classe d'une donnée noDataTile formatée.
 * Utile pour renvoyée une tuile de noData non lue dans un fichier
 * mais déjà pré-calculée en mémoire.
 * Elle est dans un format donné et stockée dans un buffer.
 */
class EmptyDataSource : public DataSource {
private:
    //nécessaire pour construire la donnée
    int channels;
    int* color;
    int width;
    int height;
    Rok4Format::eformat_data format;
    //nécessaire pour renvoyer la donnée
    size_t dataSize;
    uint8_t* data;
public:
    /**
     * Constructeur.
     */
    EmptyDataSource ( int channels, std::vector<int> _color, int width, int height, Rok4Format::eformat_data format);

    /** Destructeur **/
    virtual ~EmptyDataSource() {
        delete[] data;
        data = 0;
        delete[] color;
    }

    /** Implémentation de l'interface DataSource **/
    const uint8_t* getData ( size_t &size ) {
        size = dataSize;
        return data;
    }

    /**
     * Le buffer ne peut pas être libéré car on n'a pas de moyen de le reremplir pour un éventuel futur getData
     * @return false
     */
    bool releaseData() {
        if (data)
          delete[] data;
        data = 0;
        return true;
    }

    /** @return le type du dataSource */
    std::string getType() {
        return Rok4Format::toMimeType(format);
    }

    /** @return le status du dataSource */
    int getHttpStatus() {
        return 200;
    }

     /** @return l'encodage du dataSource */
    std::string getEncoding() {
        return "";
    }

    /** @return la taille du buffer */
   size_t getSize() {
       return dataSize;
   }
};

#endif // EMPTYDATASOURCE_H
