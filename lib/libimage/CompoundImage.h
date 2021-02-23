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

#ifndef COMPOUND_IMAGE_H
#define COMPOUND_IMAGE_H

#include "Image.h"
#include <vector>

class CompoundImage : public Image {
private:

    static int computeWidth ( std::vector<std::vector<Image*> > &images );
    static int computeHeight ( std::vector<std::vector<Image*> > &images );
    static BoundingBox<double> computeBbox ( std::vector<std::vector<Image*> > &images );

    std::vector<std::vector<Image*> > images;

    /** Indice y des tuiles courantes */
    int y;

    /** ligne correspondant au haut des tuiles courantes*/
    int top;

    template<typename T>
    inline int _getline ( T* buffer, int line );

public:

    /** D */
    int getline ( uint8_t* buffer, int line );

    /** D */
    int getline ( uint16_t* buffer, int line );
    
    /** D */
    int getline ( float* buffer, int line );

    /** D */
    CompoundImage ( std::vector< std::vector<Image*> >& images );

    /** D */
    ~CompoundImage() {
        
        if ( ! isMask ) {
            for ( int y = 0; y < images.size(); y++ )
                for ( int x = 0; x < images[y].size(); x++ )
                    delete images[y][x];
        }
    }

    /** \~french
     * \brief Sortie des informations sur l'image composée simple
     ** \~english
     * \brief Simple compounded image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "------ CompoundImage -------" );
        Image::print();
        LOGGER_INFO ( "\t- Number of images = " );
        LOGGER_INFO ( "\t\t- heightwise = " << images.size());
        LOGGER_INFO ( "\t\t- widthwise = " << images.at(0).size());
    }

};

#endif
