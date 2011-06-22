/*
Programme-test en C de l'API ROK4
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>	// En C99 seulement
#include "Rok4Api.h"

void usage() {
	fprintf(stderr,"Usage : test_api -f [server-config-file]\n");
}

bool parseCommandLine(int argc, char* argv[], char* server_config_file){
	int i;
        for(i = 1; i < argc; i++) {
                if(argv[i][0] == '-') {
                        switch(argv[i][1]) {
                                case 'f':
                                if(++i == argc){
                                        fprintf(stderr,"missing parameter in -f argument");
					return false;
                                }
                                strcpy(server_config_file,argv[i]);
                        }
                }
        }
	fprintf(stdout,"Configuration serveur : %s\n",server_config_file);

	return true;
}

int main(int argc, char* argv[]) {

	// Lecture de la ligne de commande
	char server_config_file[200];
	if (!parseCommandLine(argc,argv,server_config_file)){
		usage();
                return -1;
	}

	// Initialisation du serveur
	void* server=rok4InitServer(server_config_file);
	if (server==0){
		fprintf(stdout,"Impossible d'initialiser le serveur\n");
		return -1;
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

	// GetTile
	char** filename;
	int posoff, possize, errorStatus;
	
	fprintf(stdout,"GetTile n°1 : \n");

	HttpResponse* error1=rok4GetTileReferences("SERVICE=WMTS&REQUEST=GetCapabilities", "localhost", "/target/bin/rok4", server, filename, &posoff, &possize);

	if (error1){
		fprintf(stdout,"Statut=%d\n",error1->status);
	        fprintf(stdout,"type=%s\n",error1->type);
		fprintf(stdout,"error content=%s\n",error1->content);
	}

	fprintf(stdout,"GetTile n°2 : \n");

	HttpResponse* error2=rok4GetTileReferences("SERVICE=WMTS&REQUEST=GetTile&tileCol=9479&tileRow=109474&tileMatrix=20&LAYER=PARCELLAIRE_PNG_IGNF_LAMB93&STYLES=&FORMAT=image/png&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=LAMB93_10cm&VERSION=1.0.0", "localhost", "/target/bin/rok4", server, filename, &posoff, &possize);

	fprintf(stdout,"filename : %s\noff=%d\nsize=%d\n",filename[0],posoff,possize);

	// Extinction du serveur
	rok4KillServer(server);	

	return 0;
}
