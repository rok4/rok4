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
 * @file removeWhite.cpp
 * @brief On ne veut pas qu'il y ait tous les canaux à 255 lorsque ceux ci sont des entiers sur 8 bits.
 * Dans les cas du rgb, on ne veut donc pas de blanc dans l'image. Cette couleur doit être réservée au nodata,
 * même si ce n'est pas celle donnée dans la configuration de be4.
 * La raison est la suivante : le blanc pourra être rendu transparent, et dans le cas du jpeg, le blanc de nodata
 * doit être pur, c'est pourquoi on remplit de blanc les blocs (16*16 pixels) qui contiennent au moins un pixel blanc.
 * Pour éviter de "trouer" les données, on remplace ce blanc légitime par du gris très clair (FEFEFE).
 * @author IGN
*
*/

#include "TiffWhiteManager.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "../be4version.h"

using namespace std;

void usage() {
    cerr << "removeWhite version "<< BE4_VERSION << endl;
    cerr << endl << "Usage: removeWhite <input_file> <output_file>" << endl << endl;
    cerr << "remove white pixels in the image but not pixels which touch edges (nodata)" << endl;
}

void error(string message) {
    cerr << message << endl;
    usage();
    exit(1);
}

int main(int argc, char* argv[]) {
    char *input_file = 0, *output_file = 0;

    for(int i = 1; i < argc; i++) {
        if(!input_file) input_file = argv[i];
        else if(!output_file) output_file = argv[i];
        else {
            error("Error : argument must specify exactly one input file and one output file");
        }
    }
    if(!output_file || !input_file) error("Error : argument must specify exactly one input file and one output file");
    
    TiffWhiteManager TWM(input_file,output_file,true,true);
    if (! TWM.treatWhite()) {
        error("Error : unable to treat white for this file : " + string(input_file));
    }
    
    return 0;
}

