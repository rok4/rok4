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

#include <iostream>
#include <iomanip>
#include "AscEncoder.h"
#include "Logger.h"


size_t AscEncoder::read ( uint8_t *buffer, size_t size ) {
    //size_t offset=0;
    std::ostringstream tmp_stream;
    std::string tmp_str;

    nodata_value=-99999.00;

    // Définit l'affichage des nombres dans les chaines
    tmp_stream << std::fixed << std::setprecision(2);

    if(line==0 ){
        //On traite l'entete
        tmp_stream << std::setprecision(8)
                   << "ncols        " << image->getWidth() << std::endl
                   << "nrows        " << image->getHeight() << std::endl
                   << "xllcorner    " << image->getXmin() << std::endl
                   << "yllcorner    " << image->getYmin() << std::endl
                   << "cellsize     " << image->getResXmeter() << std::endl
                   << "NODATA_value " << std::setprecision(2) << nodata_value ;
    }

    // stockage d'une ligne de donnée (1 canal)
    float* buffer_line=new float[image->getWidth()];

    if( line < image->getHeight() ){

        image->getline(buffer_line,line++);

        tmp_stream << std::endl;
        for( int i=0;i<image->getWidth();i++){
        // formattage des données
            tmp_stream << " " << buffer_line[(i*image->channels)];
        }

        tmp_str = tmp_stream.str();

        if(tmp_str.size()<=size){
        //écriture des données texte dans le buffer flux
            for ( int i=0; i<tmp_str.size(); i++ ) {
                buffer[sizeof(uint8_t)*i]= (uint8_t) tmp_str.at(i);
            }
        }else{
          LOGGER_ERROR ( "Too much data to write on 2^21 buffer");
          return tmp_str.size();
        }

    }

    delete[] buffer_line;

    return tmp_str.size();
}

AscEncoder::~AscEncoder() {
    delete image;
}

bool AscEncoder::eof() {
    return line>=image->getHeight();
}
