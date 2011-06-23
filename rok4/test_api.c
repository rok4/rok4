/**
Programme-test en C de l'API ROK4
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>	// En C99 seulement
#include <pthread.h>
#include "Rok4Api.h"

/**
* @fn void usage() Usage de la ligne de commande
*/

void usage() {
	fprintf(stderr,"Usage : test_api -f [server-config-file] -t [nb_threads]\n");
}

/**
* @fn bool parseCommandLine(int argc, char* argv[], char* server_config_file, int* nb_threads)
* @brief Lecture de la ligne de commande
*/

bool parseCommandLine(int argc, char* argv[], char* server_config_file, int* nb_threads){
	int i;
	*nb_threads=0;
	strcpy(server_config_file,"");
        for(i = 1; i < argc; i++) {
                if(argv[i][0] == '-') {
                        switch(argv[i][1]) {
                                case 'f':
                                if(++i == argc){
                                        fprintf(stderr,"missing parameter in -f argument");
					return false;
                                }
                                strcpy(server_config_file,argv[i]);
				break;
				case 't':
				if(++i == argc){
                                        fprintf(stderr,"missing parameter in -t argument");
                                        return false;
                                }
				if ((*nb_threads=atoi(argv[i]))<=0){
                                        fprintf(stderr,"wrong parameter in -t argument");
                                        return false;
                                }
				break;
                        }
                }
        }
	if (strcmp(server_config_file,"")==0 || *nb_threads==0)
		return false;

	fprintf(stdout,"Configuration serveur : %s\n",server_config_file);
	fprintf(stdout,"Nombre de threads : %d\n",*nb_threads);

	return true;
}

/**
* @fn void* processThread(void* arg)
* Fonction executee dans un thread
* @param[in] arg : pointeur sur le fichier de configurtion du serveur
*/


void* processThread(void* arg){
	// Initialisation du serveur
        void* server=rok4InitServer((char*)arg);
        if (server==0){
                fprintf(stdout,"Impossible d'initialiser le serveur\n");
                return 0;
        }
        fprintf(stdout,"Serveur initialise\n");

        // GetCapabilities
        fprintf(stdout,"GetCapabilites : \n");

        HttpResponse* capabilities=rok4GetWMTSCapabilities("localhost","/target/bin/rok4",server);

        fprintf(stdout,"Statut=%d\n",capabilities->status);
        fprintf(stdout,"type=%s\n",capabilities->type);

        FILE* C=fopen("capabilities.xml","w");
        fprintf(C,"%s",capabilities->content);
        fclose(C);

/*        // GetTile
	TileRef tileRef;
        int errorStatus;

        fprintf(stdout,"GetTile n°1 : \n");

        HttpResponse* error1=rok4GetTileReferences("SERVICE=WMTS&REQUEST=GetCapabilities", "localhost", "/target/bin/rok4", server, &tileRef);

        if (error1){
                fprintf(stdout,"Statut=%d\n",error1->status);
                fprintf(stdout,"type=%s\n",error1->type);
                fprintf(stdout,"error content=%s\n",error1->content);
        }

        fprintf(stdout,"GetTile n°2 : \n");

        HttpResponse* error2=rok4GetTileReferences("SERVICE=WMTS&REQUEST=GetTile&tileCol=9479&tileRow=109474&tileMatrix=20&LAYER=PARCELLAIRE_PNG_IGNF_LAMB93&STYLES=&FORMAT=image/png&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=LAMB93_10cm&VERSION=1.0.0", "localhost", "/target/bin/rok4", server, &tileRef);

        fprintf(stdout,"filename : %s\noff=%d\nsize=%d\n",tileRef.filename,tileRef.posoff,tileRef.possize);

        // Extinction du serveur
        rok4KillServer(server);
*/	
	return 0;
}

/**
* Fonction principale
* Lance n threads d'execution du serveur
*/

int main(int argc, char* argv[]) {

	// Lecture de la ligne de commande
	char server_config_file[200];
	int nb_threads;
	if (!parseCommandLine(argc,argv,server_config_file,&nb_threads)){
		usage();
                return -1;
	}

	pthread_t* threads= (pthread_t*)malloc(nb_threads*sizeof(pthread_t));

	int i;
	for(i = 0; i < nb_threads; i++){
                pthread_create(&(threads[i]), NULL, processThread, (void*) server_config_file);
        }
        for(i = 0; i < nb_threads; i++)
                pthread_join(threads[i], NULL);

	return 0;
}
