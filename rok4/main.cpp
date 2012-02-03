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

#include "Rok4Server.h"
#include "ConfLoader.h"
#include <proj_api.h>
#include "Rok4Api.h"

/* Usage de la ligne de commande */

void usage() {
    std::cerr<<" Usage : rok4 [-f server_config_file]"<<std::endl;
}

/**
* @brief main
* @return -1 en cas d'erreur, 0 sinon
*/

int main ( int argc, char** argv ) {

    /* the following loop is for fcgi debugging purpose */
    int stopSleep = 0;
    while ( getenv ( "SLEEP" ) != NULL && stopSleep == 0 ) {
        sleep ( 2 );
    }

    // Lecture des arguments de la ligne de commande
    std::string serverConfigFile=DEFAULT_SERVER_CONF_PATH;
    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'f': // fichier de configuration du serveur
                if ( i++ >= argc ) {
                    std::cerr<<"Erreur sur l'option -f"<<std::endl;
                    usage();
                    return -1;
                }
                serverConfigFile.assign ( argv[i] );
                break;
            default:
                usage();
                return -1;
            }
        }
    }

    // Demarrage du serveur
    std::cout<< "Lancement du serveur rok4..."<<std::endl;
    Rok4Server* W=rok4InitServer ( serverConfigFile.c_str() );
    W->run();

    // Extinction du serveur
    LOGGER_INFO ( "Extinction du serveur ROK4" );
    //delete servicesConf;
    rok4KillServer ( W );

    return 0;
}

