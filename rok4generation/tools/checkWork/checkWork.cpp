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

/**
 * \file checkWork.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Contrôle la validité d'une image
 * \~english \brief Control image validity
 */

#include <cstdlib>
#include <iostream>
#include <string.h>
#include "Logger.h"
#include "FileImage.h"
#include "../../../rok4version.h"

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande checkWork
 */
void usage() {

    LOGGER_INFO ( "\ncheckWork version " << ROK4_VERSION << "\n\n" <<

                  "Control TIFF, JPEG, JPEG2000 or PNG image validity\n\n" << 

                  "Usage: checkWork <INPUT FILE>\n"
                );
}

/**
 * \~french
 * \brief Affiche un message d'erreur, l'utilisation de la commande et sort en erreur
 * \param[in] message message d'erreur
 * \param[in] errorCode code de retour
 */
void error ( std::string message, int errorCode ) {
    LOGGER_ERROR ( message );
    usage();
    sleep ( 1 );
    exit ( errorCode );
}

/**
 ** \~french
 * \brief Fonction principale de l'outil createNodata
 * \details Tout est contenu dans cette fonction.
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 en cas de succès, -1 sinon
 ** \~english
 * \brief Main function for tool createNodata
 * \details All instructions are in this function.
 * \param[in] argc parameters number
 * \param[in] argv parameters array
 * \return return code, 0 if success, -1 otherwise
 */
int main ( int argc, char **argv )
{
    char* input = 0;

    /* Initialisation des Loggers */
    Logger::setOutput ( STANDARD_OUTPUT_STREAM_FOR_ERRORS );

    Accumulator* acc = new StreamAccumulator();
    Logger::setAccumulator ( INFO , acc );
    Logger::setAccumulator ( WARN , acc );
    Logger::setAccumulator ( ERROR, acc );
    Logger::setAccumulator ( FATAL, acc );

    std::ostream &logw = LOGGER ( WARN );
    logw.precision ( 16 );
    logw.setf ( std::ios::fixed,std::ios::floatfield );

    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'h': // help
                usage();
                exit ( 0 );
                break;
            default:
                error ( "Unknown option : -" + argv[i][1] ,-1 );
            }
        } else {
            if ( input == 0 ) input = argv[i];
            else {
                error ("Argument must specify ONE input file", -1);
            }
        }
    }

    if ( input == 0) {
        error ("Argument must specify one input file", -1);
    }

    FileImageFactory FIF;

    FileImage* image = FIF.createImageToRead(input);
    int ret;
    if (image == NULL) {
        LOGGER_ERROR("Image NOK");
        ret = -1;
    } else {
        LOGGER_INFO("Image OK");
        ret = 0;
    }

    // Nettoyage
    delete image;
    // Suppression du nettoyage du logger jusqu'à sa refonte
    // Logger::stopLogger();
    // if ( acc ) {
    //     delete acc;
    // }

    return ret;
}
