/*
Programme-test en C de l'API ROK4
*/

#include <stdio.h>
#include <string.h>
#include "Rok4Api.h"

void usage() {
	fprintf(stderr,"Usage : test_api -f [server-config-file] -s [services-config-file]\n");
}

int main(int argc, char* argv[]) {

	// Lecture de la ligne de commande
	char server_config_file[100],services_config_file[100];
	int i;
	for(i = 1; i < argc; i++) {
    		if(argv[i][0] == '-') {
      			switch(argv[i][1]) {
        			case 'f':
          			if(++i == argc)  fprintf(stderr,"missing parameter in -f argument");
          			strcpy(server_config_file,argv[i]);
          			break;
				case 's':
                                if(++i == argc)  fprintf(stderr,"missing parameter in -s argument");
                                strcpy(services_config_file,argv[i]);
                                break;
				default:
          			break;
			}
		}
    	}

	fprintf(stdout,"Configuration serveur %s\n",server_config_file);
	fprintf(stdout,"Configuration services %s\n",services_config_file);

	// Initialisation du serveur
	void* server=rok4InitServer(server_config_file,services_config_file);
	if (server==0){
		fprintf(stdout,"Impossible d'initialiser le serveur\n");
		return -1;
	}

	fprintf(stdout,"server %p\n",server);

	fprintf(stdout,"Serveur initialise\n");

	// GetCapabilities
	fprintf(stdout,"GetCapabilites : \n");

	HttpResponse* capabilities=rok4GetWMTSCapabilities("localhost","/target/bin/rok4",server);

	fprintf(stdout,"Statut=%d\n",capabilities->status);
	fprintf(stdout,"type=%s\n",capabilities->type);
	
	FILE* C=fopen("capabilities.xml","w");
	fprintf(C,"%s",capabilities->content);
	fclose(C);	

	// GetTile
	char** filename;
	int posoff, possize, errorStatus;
	
	fprintf(stdout,"GetTile n°1 : \n");

	HttpResponse* error1=rok4GetTile("SERVICE=WMTS&REQUEST=GetCapabilities", "localhost", "/target/bin/rok4", server, filename, &posoff, &possize);

	if (error1){
		fprintf(stdout,"Statut=%d\n",error1->status);
	        fprintf(stdout,"type=%s\n",error1->type);
		fprintf(stdout,"error content=%s\n",error1->content);
	}

	fprintf(stdout,"GetTile n°2 : \n");

	HttpResponse* error2=rok4GetTile("SERVICE=WMTS&REQUEST=GetTile&tileCol=9479&tileRow=109474&tileMatrix=20&LAYER=PARCELLAIRE_PNG_IGNF_LAMB93&STYLES=&FORMAT=image/png&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=LAMB93_10cm&VERSION=1.0.0", "localhost", "/target/bin/rok4", server, filename, &posoff, &possize);

	fprintf(stdout,"filename : %s\noff=%d\nsize=%d\n",filename[0],posoff,possize);

	// Extinction du serveur
	rok4KillServer(server);	

	return 0;
}
