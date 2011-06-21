#include "Rok4Server.h"
#include "ConfLoader.h"
#include <proj_api.h>

/* Usage de la ligne de commande */

void usage() {
        std::cerr<<" Usage : rok4 [-f server_config_file]"<<std::endl;
}

/*
* main
* @return -1 en cas d'erreur
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

        std::cout<< "Lancement du serveur rok4..."<<std::endl;

        // Initialisation de l'accès au paramétrage de la libproj
        // Cela evite d'utiliser la variable d'environnement PROJ_LIB
        pj_set_finder( pj_finder );

        /* Chargement de la conf technique du serveur */

        // Chargement de la conf technique
        std::string logFileprefix;
        int logFilePeriod;
        int nbThread;
        std::string layerDir;
        std::string tmsDir;
        if(!ConfLoader::getTechnicalParam(serverConfigFile, logFileprefix, logFilePeriod, nbThread, layerDir, tmsDir)){
                std::cerr<<"ERREUR FATALE : Impossible d'interpreter le fichier de configuration du serveur "<<serverConfigFile<<std::endl;
                std::cerr<<"Extinction du serveur ROK4"<<std::endl;
                // On attend 10s pour eviter que le serveur web ne le relance tout de suite
                // avec les mêmes conséquences et sature les logs trop rapidement.
                // sleep(10);
                return -1;
        }


        // Initialisation des loggers
        RollingFileAccumulator* acc = new RollingFileAccumulator(logFileprefix,logFilePeriod);
        Logger::setAccumulator(DEBUG, acc);
        Logger::setAccumulator(INFO , acc);
        Logger::setAccumulator(WARN , acc);
        Logger::setAccumulator(ERROR, acc);
        Logger::setAccumulator(FATAL, acc);

        std::ostream &log = LOGGER(DEBUG);
        log.precision(8);
        log.setf(std::ios::fixed,std::ios::floatfield);

        std::cout<<"Envoi des messages dans la sortie du logger"<< std::endl;
        LOGGER_INFO("*** DEBUT DU FONCTIONNEMENT DU LOGGER ***");

        // Chargement de la conf services
        ServicesConf* servicesConf=ConfLoader::buildServicesConf(DEFAULT_SERVICES_CONF_PATH);
        if(!servicesConf){
                LOGGER_FATAL("Impossible d'interpreter le fichier de conf services.conf");
                LOGGER_FATAL("Extinction du serveur ROK4");
                // On attend 10s pour eviter que le serveur web ne le relance tout de suite
                // avec les mêmes conséquences et sature les logs trop rapidement.
                // sleep(10);
                return -1;
        }

        // Chargement des TMS
        std::map<std::string,TileMatrixSet*> tmsList;
        if(!ConfLoader::buildTMSList(tmsDir,tmsList)){
                LOGGER_FATAL("Impossible de charger la conf des TileMatrix");
                LOGGER_FATAL("Extinction du serveur ROK4");
                // On attend 10s pour eviter que le serveur web ne le relance tout de suite
                // avec les mêmes conséquences et sature les logs trop rapidement.
                // sleep(10);
                return -1;
        }

        // Chargement des Layers
        std::map<std::string, Layer*> layerList;
        if(!ConfLoader::buildLayersList(layerDir,tmsList,layerList)){
                LOGGER_FATAL("Impossible de charger la conf des Layers/pyramides");
                LOGGER_FATAL("Extinction du serveur ROK4");
                // On attend 10s pour eviter que le serveur web nele relance tout de suite
                // avec les mêmes conséquences et sature les logs trop rapidement.
                /* DEBUG
            sleep(10);*/
                return -1;
        }

        // Construction du serveur.
        Rok4Server W(nbThread, *servicesConf, layerList, tmsList);

        W.run();

        // Extinction du serveur
        LOGGER_INFO( "Extinction du serveur ROK4");

        delete servicesConf;

        std::map<std::string,TileMatrixSet*>::iterator iTms;
        for (iTms=tmsList.begin();iTms!=tmsList.end();iTms++)
                delete (*iTms).second;

        std::map<std::string, Layer*>::iterator iLayer;
        for (iLayer=layerList.begin();iLayer!=layerList.end();iLayer++)
                delete (*iLayer).second;

        delete acc;
}

