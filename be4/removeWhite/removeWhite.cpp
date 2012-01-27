
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

#include "tiffio.h"
#include <cstdlib>
#include <iostream>
#include <string.h>

using namespace std;

void usage() {
    cerr << endl << "usage: removeWhite <input_file> <output_file>" << endl << endl;
    cerr << "remove white pixels in the image but not pixels which touch edges (nodata)" << endl;
}

void error(string message) {
    cerr << message << endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    char *input_file = 0, *output_file = 0;

    for(int i = 1; i < argc; i++) {
        if(!input_file) input_file = argv[i];
        else if(!output_file) output_file = argv[i];
        else error("Error : argument must specify exactly one input file and one output file");
    }
    if(!output_file) error("Error : argument must specify exactly one input file and one output file");
    
    char commandConvert[1000];
    strcpy(commandConvert,"convert -fill \"#FEFEFE\" -opaque \"#FFFFFF\" ");
    strcat(commandConvert,input_file);
    strcat(commandConvert," ");
    strcat(commandConvert,output_file);
    
    if (system(commandConvert)) {
        error("Convert failed");
    }
    
    char commandNodataIdentifier[1000];
    strcpy(commandNodataIdentifier,"nodataIdentifier -n1 FEFEFE -n2 FFFFFF ");
    strcat(commandNodataIdentifier,output_file);
    
    if (system(commandNodataIdentifier)) {
        error("NodataIdentifier failed");
    }
    
    return 0;
}

