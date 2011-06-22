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

int main(int argc, char** argv) {

        /* the following loop is for fcgi debugging purpose */
        int stopSleep = 0;
        while (getenv("SLEEP") != NULL && stopSleep == 0) {
                sleep(2);
        }

        // Lecture des arguments de la ligne de commande
        std::string serverConfigFile=DEFAULT_SERVER_CONF_PATH;
        for(int i = 1; i < argc; i++) {
                if(argv[i][0] == '-') {
                        switch(argv[i][1]) {
                        case 'f': // fichier de configuration du serveur
                                if(i++ >= argc){
                                        std::cerr<<"Erreur sur l'option -f"<<std::endl;
                                        usage();
                                        return -1;
                                }
                                serverConfigFile.assign(argv[i]);
                                break;
                        default:
                                usage();
                                return -1;
                        }
                }
        }

	// Demarrage du serveur
        std::cout<< "Lancement du serveur rok4..."<<std::endl;
	Rok4Server* W=rok4InitServer(serverConfigFile.c_str());
        W->run();

        // Extinction du serveur
        LOGGER_INFO("Extinction du serveur ROK4");
        //delete servicesConf;
	rok4KillServer(W);
	
	return 0;
}

