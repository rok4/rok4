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

#include "CompoundImage.h"

int CompoundImage::computeWidth ( std::vector<std::vector<Image*> > &images ) {
    int width = 0;
    for ( int x = 0; x < images[0].size(); x++ ) width += images[0][x]->getWidth();
    return width;
}

int CompoundImage::computeHeight ( std::vector<std::vector<Image*> > &images ) {
    int height = 0;
    for ( int y = 0; y < images.size(); y++ ) height += images[y][0]->getHeight();
    return height;
}

BoundingBox<double> CompoundImage::computeBbox ( std::vector<std::vector<Image*> > &images ) {
    double xmin = images[images.size()-1][0]->getBbox().xmin;
    double ymin = images[images.size()-1][0]->getBbox().ymin;

    double xmax = images[0][images[0].size()-1]->getBbox().xmax;
    double ymax = images[0][images[0].size()-1]->getBbox().ymax;

    return BoundingBox<double> ( xmin, ymin, xmax, ymax );
}

template<typename T>
inline int CompoundImage::_getline ( T* buffer, int line ) {
    // doit-on changer de tuile ?
    if (line >= height) {
        return 0;
    }
    while ( top + images[y][0]->getHeight() <= line ) top += images[y++][0]->getHeight();
    while ( top > line ) top -= images[--y][0]->getHeight();
    // on calcule l'indice de la ligne dans la sous tuile
    line -= top;
    for ( int x = 0; x < images[y].size(); x++ )
        buffer += images[y][x]->getline ( buffer, line );
    return width*channels;
}

/** D */
int CompoundImage::getline ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/** D */
int CompoundImage::getline ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/** D */
int CompoundImage::getline ( float* buffer, int line ) {
    return _getline ( buffer, line );
}

/** D */
CompoundImage::CompoundImage ( std::vector< std::vector<Image*> >& images ) :
    Image ( computeWidth ( images ), computeHeight ( images ), images[0][0]->getChannels(), images[0][0]->getResX(),images[0][0]->getResY(), computeBbox ( images ) ),
    images ( images ),
    top ( 0 ),
    y ( 0 ) {}

