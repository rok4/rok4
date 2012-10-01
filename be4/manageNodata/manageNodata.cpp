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
 * \file manageNodata.cpp
 * \brief Remplacement de la valeur de nodata par une autre
 * \author IGN
 *
 * Ce programme est destine à modifier les pixels qui peuvent être considérés comme du nodata. Pour cela, on définit la valeur que le nodata a dans l'image et la valeur que l'on veut lui donner. Tous les pixels qui touchent le bord et qui portent la valeur donnée seront modifiés. Dans l'exemple du dessus, tous les pixel noirs opaque qui touchent le bord deviendront blanc transparent.
 *
 * Parametres d'entree :
 * 1. Le nombre de canaux sur lequel on travaille
 * 2. La valeur du nodata dans l'image en entrée
 * 3. La valeur que le nodata aura dans l'image de sortie
 * 4. Un fichier en entrée
 * 5. Un fichier de sortie
 *
 * Le nombre de canaux du fichier en entrée et les valeurs de nodata renseignée doivent être cohérents.
 */

using namespace std;

#include "TiffNodataManager.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "../be4version.h"

void usage() {
    cerr << "manageNodata version "<< BE4_VERSION << endl;
    cerr << endl << "Usage: manageNodata -channels 4 -oldValue 0,0,0,255 -newValue 255,255,255,0 input.tif output.tif" << endl << endl;
}


void error(string message) {
    cerr << message << endl;
    usage();
    exit(1);
}

int main(int argc, char* argv[]) {
    char* input = 0;
    char* output = 0;

    char* strOldValue = 0;
    char* strNewValue = 0;

    int channels = 0;

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i],"-oldValue")) {
            if(i++ >= argc) error("Error with option -oldValue");
            strOldValue = argv[i];
            continue;
        }
        else if(!strcmp(argv[i],"-channels")) {
            if(i++ >= argc) error("Error with option -channels");
            channels = atoi(argv[i]);
            continue;
        }
        else if(!strcmp(argv[i],"-newValue")) {
            if(i++ >= argc) error("Error with option -newValue");
            strNewValue = argv[i];
            continue;
        }
        else if(!input) {
            input = argv[i];
        }
        else if(!output) {
            output = argv[i];
        }
        else {
            error("Error : unknown option : "+string(argv[i]));
        }
    }

    if(!output || !input) error("Error : input file or output file is missing.");
    if(!channels) error("Error : samples number have to be precise with option -channels");
    if(!strOldValue || !strNewValue) error("Error : old and new values for nodata are required.");

    // Old nodata interpretation
    uint8_t oldValue[channels];
    char* charValue = strtok(strOldValue,",");
    if(charValue == NULL) {
        error("Error with option -oldValue : integer values (between 0 and 255) seperated by comma");
    }
    int value = atoi(charValue);
    if(value < 0 || value > 255) {
        error("Error with option -oldValue : integer values (between 0 and 255) seperated by comma");
    }
    oldValue[0] = value;
    for(int i = 1; i < channels; i++) {
        charValue = strtok (NULL, ",");
        if(charValue == NULL) {
            error("Error with option -oldValue : integer values (between 0 and 255) seperated by comma");
        }
        value = atoi(charValue);
        if(value < 0 || value > 255) {
            error("Error with option -oldValue : integer values (between 0 and 255) seperated by comma");
        }
        oldValue[i] = value;
    }

    // Old nodata interpretation
    uint8_t newValue[channels];
    charValue = strtok(strNewValue,",");
    if(charValue == NULL) {
        error("Error with option -newValue : integer values (between 0 and 255) seperated by comma");
    }
    value = atoi(charValue);
    if(value < 0 || value > 255) {
        error("Error with option -newValue : integer values (between 0 and 255) seperated by comma");
    }
    newValue[0] = value;
    for(int i = 1; i < channels; i++) {
        charValue = strtok (NULL, ",");
        if(charValue == NULL) {
            error("Error with option -newValue : integer values (between 0 and 255) seperated by comma");
        }
        value = atoi(charValue);
        if(value < 0 || value > 255) {
            error("Error with option -newValue : integer values (between 0 and 255) seperated by comma");
        }
        newValue[i] = value;
    }

    TiffNodataManager TNM(channels,oldValue,oldValue,newValue,false,true);
    
    if (! TNM.treatNodata(input,output)) {
        error("Error : unable to treat white for this file : " + string(input));
    }

    return 0;
}
