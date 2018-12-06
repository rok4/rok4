/*
 * Copyright © (2011-2013) Institut national de l'information
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
 * \file main.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Le serveur ROk4 peut fonctionner dans 2 modes distinct :
 *  - autonome, en définissant l'adresse et le port d'écoute dans le fichier de configuration
 *  - controlé, les paramètres d'écoute sont données par un processus maitre
 *
 * Paramètre d'entrée :
 *  - le chemin vers le fichier de configuration du serveur
 *
 * Signaux écoutés :
 *  - \b SIGHUP réinitialise la configuration du serveur
 *  - \b SIGQUIT & \b SIGUSR1 éteint le serveur
 * \brief Exécutable du serveur ROK4
 * \~english
 * The ROK4 server can be started in two mods :
 *  - autonomous, by defining in the config files the adress and port to listen to
 *  - managed, by letting a master process define the adress and port to liste to
 *
 * Command line parameter :
 *  - path to the server configuration file
 *
 * Listened Signal :
 *  - \b SIGHUP reinitialise the server configuration
 *  - \b SIGQUIT & \b SIGUSR1 shut the server down
 * \brief ROK4 Server executable
 */

#include "Rok4Server.h"
#include "ConfLoader.h"
#include <proj_api.h>
#include "Rok4Api.h"
#include <csignal>
#include <bits/signum.h>
#include <sys/time.h>
#include <locale>
#include "intl.h"
#include <limits>
#include "config.h"
#include "curl/curl.h"
#include <time.h>
/* Usage de la ligne de commande */

Rok4Server* W;
Rok4Server* Wtmp;
bool reload;

std::string serverConfigFile;
time_t lastReload;

// Minimum time between two signal to be defered.
// Earlier signal would be ignored.
// in microseconds
static const double signal_defering_min_time = 1000000LL;

volatile sig_atomic_t signal_pending = 0;
volatile sig_atomic_t defer_signal;
volatile timeval signal_timestamp;

/**
 * \~french
 * \brief Affiche les paramètres de la ligne de commande
 * \~english
 * \brief Display the command line parameters
 */
void usage() {
    std::cerr<<_ ( "Usage : rok4 [-f server_config_file]" ) <<std::endl;
}

/**
 * \~french
 * \brief Force le rechargement de la configuration
 * \~english
 * \brief Force configuration reload
 */
void reloadConfig ( int signum ) {
    if ( defer_signal ) {
        timeval now;
        gettimeofday ( &now, NULL );
        double delta = ( now.tv_sec - signal_timestamp.tv_sec ) *1000000LL + ( now.tv_usec - signal_timestamp.tv_usec );
        if ( delta > signal_defering_min_time ) {
            signal_pending = signum;
        }
    } else {
        defer_signal++;
        signal_pending = 0;
        timeval begin;
        gettimeofday ( &begin, NULL );
        signal_timestamp.tv_sec = begin.tv_sec;
        signal_timestamp.tv_usec = begin.tv_usec;
        reload = true;
        std::cout<< _ ( "Rechargement du serveur rok4" ) << "["<< getpid() <<"]" <<std::endl;
        time_t tmpTime = time(NULL);
        Wtmp = rok4ReloadServer ( serverConfigFile.c_str(), W, lastReload );
        if ( !Wtmp ){
            std::cout<< _ ( "Erreur lors du rechargement du serveur rok4" ) << "["<< getpid() <<"]" <<std::endl;
            return;
        }
        lastReload = tmpTime;
        W->terminate();
    }
}
/**
 * \~french
 * \brief Force le serveur à s'éteindre
 * \~english
 * \brief Force server shutdown
 */
void shutdownServer ( int signum ) {
    if ( defer_signal ) {
        // Do nothing because rok4 is going to shutdown...
    } else {
        defer_signal++;
        reload = false;
        W->terminate();
    }
}

/**
 * \~french
 * \brief Retourne l'emplacement des fichier de traduction
 * \return repertoire des traductions
 * \~english
 * \brief Return the translation files path
 * \return translation directory
 */
std::string getlocalepath() {
    char result[ 4096 ];
    char procPath[20];
    sprintf ( procPath,"/proc/%u/exe",getpid() );
    ssize_t count = readlink ( procPath, result, 4096 );
    std::string exePath ( result, ( count > 0 ) ? count : 0 );
    std::string localePath ( exePath.substr ( 0,exePath.rfind ( "/" ) ) );
    localePath.append ( "/../share/locale" );
    return localePath;
}

/**
 * \~french
 * \brief Fonction principale
 * \return 1 en cas de problème, 0 sinon
 * \~english
 * \brief Main function
 * \return 1 if error, else 0
 */
int main ( int argc, char** argv ) {

    bool firstStart = true;
    int sock = 0;
    reload = true;
    defer_signal = 1;
    /* install Signal Handler for Conf Reloadind and Server Shutdown*/
    struct sigaction sa;
    sigemptyset ( &sa.sa_mask );
    sa.sa_flags = 0;
    sa.sa_handler = reloadConfig;
    sigaction ( SIGHUP, &sa,0 );

    sa.sa_handler = shutdownServer;
    sigaction ( SIGQUIT, &sa,0 );

    // Apache mod_fastcgi compatibility
    sa.sa_handler = shutdownServer;
    sigaction ( SIGUSR1, &sa,0 );

    setlocale ( LC_ALL,"" );
    //  textdomain("Rok4Server");
    bindtextdomain ( DOMAINNAME, getlocalepath().c_str() );

    //CURL initialization - one time for the whole program
    curl_global_init(CURL_GLOBAL_ALL);

    /* the following loop is for fcgi debugging purpose */
    int stopSleep = 0;
    while ( getenv ( "SLEEP" ) != NULL && stopSleep == 0 ) {
        sleep ( 2 );
    }

    // Lecture des arguments de la ligne de commande
    serverConfigFile=DEFAULT_SERVER_CONF_PATH;
    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'f': // fichier de configuration du serveur
                if ( i++ >= argc ) {
                    std::cerr<<_ ( "Erreur sur l'option -f" ) <<std::endl;
                    usage();
                    return 1;
                }
                serverConfigFile.assign ( argv[i] );
                break;
            default:
                usage();
                return 1;
            }
        }
    }

    // Demarrage du serveur
    while ( reload ) {
        reload = false;
        std::cout<< _ ( "Lancement du serveur rok4" ) << "["<< getpid() <<"]" <<std::endl;
       
        if ( firstStart ) {
            lastReload = time(NULL);
            W = rok4InitServer ( serverConfigFile.c_str() );
            if ( !W ) {
                return 1;
            }
            W->initFCGI();
            firstStart = false;
        } else {
            std::cout<< _ ( "Mise a jour de la configuration" ) << "["<< getpid() <<"]" <<std::endl;
            if ( Wtmp ) {
                std::cout<< _ ( "Bascule des serveurs" ) << "["<< getpid() <<"]" <<std::endl;
                W = Wtmp;
                Wtmp = 0;
            }
            W->setFCGISocket ( sock );
        }
#if BUILD_OBJECT
        rok4ConnectObjectContext(W);
#endif

        // Remove Event Lock
        defer_signal--;
        
        W->run(signal_pending);

        // Extinction du serveur
        if ( reload ) {
            LOGGER_INFO ( _ ( "Rechargement de la configuration" ) );
            sock = W->getFCGISocket();
        } else {
            LOGGER_INFO ( _ ( "Extinction du serveur ROK4" ) );
        }
#if BUILD_OBJECT
        rok4DisconnectObjectContext(W);
#endif
        rok4KillServer ( W );
        rok4ReloadLogger();
    }

    //CURL clean - one time for the whole program
    curl_global_cleanup();

    rok4KillLogger();
    return 0;
}
