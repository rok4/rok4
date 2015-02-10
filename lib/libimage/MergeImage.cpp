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
 * \file MergeImage.cpp
 ** \~french
 * \brief Implémentation des classes MergeImage, MergeImageFactory et MergeMask et du namespace Merge
 * \details
 * \li MergeImage : image résultant de la fusion d'images semblables, selon différents modes de composition
 * \li MergeImageFactory : usine de création d'objet MergeImage
 * \li MergeMask : masque fusionné, associé à une image fusionnée
 * \li Merge : énumère et manipule les différentes méthodes de fusion
 ** \~english
 * \brief Implement classes MergeImage, MergeImageFactory and MergeMask and the namespace Merge
 * \details
 * \li MergeImage : image merged with similar images, with different merge methods
 * \li MergeImageFactory : factory to create MergeImage object
 * \li MergeMask : merged mask, associated with a merged image
 * \li Merge : enumerate and managed different merge methods
 */

#include "MergeImage.h"

#include "Utils.h"
#include "Logger.h"
#include <cstring>


/* Implementation de getline pour les uint8_t */
int MergeMask::getline ( uint8_t* buffer, int line ) {
    memset ( buffer,0,width );

    uint8_t* buffer_m = new uint8_t[width];

    for ( uint i = 0; i < MI->getImages()->size(); i++ ) {

        if ( MI->getMask ( i ) == NULL ) {
            /* L'image n'a pas de masque, on la considère comme pleine. Ca ne sert à rien d'aller voir plus loin,
             * cette ligne du masque est déjà pleine */
            memset ( buffer, 255, width );
            delete [] buffer_m;
            return width;
        } else {
            // Récupération du masque de l'image courante de l'MI.
            MI->getMask ( i )->getline ( buffer_m,line );
            // On ajoute au masque actuel (on écrase si la valeur est différente de 0)
            for ( int j = 0; j < width; j++ ) {
                if ( buffer_m[j] ) {
                    memcpy ( &buffer[j],&buffer_m[j],1 );
                }
            }
        }
    }

    delete [] buffer_m;
    return width;
}

/* Implementation de getline pour les float */
int MergeMask::getline ( float* buffer, int line ) {
    uint8_t* buffer_t = new uint8_t[width*channels];
    int retour = getline ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return retour;
}

namespace Merge {

const char *mergeType_name[] = {
    "UNKNOWN",
    "NORMAL",
    "LIGHTEN",
    "DARKEN",
    "MULTIPLY",
    "ALPHATOP",
    "TOP"
};

eMergeType fromString ( std::string strMergeMethod ) {
    int i;
    for ( i = mergeType_size; i ; --i ) {
        if ( strMergeMethod.compare ( mergeType_name[i] ) == 0 )
            break;
    }
    return static_cast<eMergeType> ( i );
}

std::string toString ( eMergeType mergeMethod ) {
    return std::string ( mergeType_name[mergeMethod] );
}
}
