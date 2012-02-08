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
Programme-test en C de l'API ROK4
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>    // En C99 seulement
#include <pthread.h>
#include "Rok4Api.h"

static pthread_mutex_t mutex_rok4= PTHREAD_MUTEX_INITIALIZER;

static int c;
static FILE* requestFile;

/**
* @fn void usage() Usage de la ligne de commande
*/

void usage() {
    fprintf ( stderr,"Usage : test_api -f [server-config-file] -t [nb_threads] -r [request_file]\n" );
}

/**
* @fn bool parseCommandLine(int argc, char* argv[], char* server_config_file, int* nb_threads, char* request_file)
* @brief Lecture de la ligne de commande
*/

bool parseCommandLine ( int argc, char* argv[], char* server_config_file, int* nb_threads, char* request_file ) {
    int i;
    strcpy ( server_config_file,"" );
    *nb_threads=0;
    strcpy ( request_file,"" );
    for ( i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'f':
                if ( ++i == argc ) {
                    fprintf ( stderr,"missing parameter in -f argument" );
                    return false;
                }
                strcpy ( server_config_file,argv[i] );
                break;
            case 't':
                if ( ++i == argc ) {
                    fprintf ( stderr,"missing parameter in -t argument" );
                    return false;
                }
                if ( ( *nb_threads=atoi ( argv[i] ) ) <=0 ) {
                    fprintf ( stderr,"wrong parameter in -t argument" );
                    return false;
                }
                break;
            case 'r' :
                if ( ++i == argc ) {
                    fprintf ( stderr,"missing parameter in -r argument" );
                    return false;
                }
                strcpy ( request_file,argv[i] );
            }
        }
    }
    if ( strcmp ( server_config_file,"" ) ==0 || *nb_threads==0 || strcmp ( request_file,"" ) ==0 )
        return false;

    fprintf ( stdout,"\tConfiguration serveur : %s\n",server_config_file );
    fprintf ( stdout,"\tNombre de threads : %d\n",*nb_threads );
    fprintf ( stdout,"\tFichier de requetes : %s\n",request_file );

    return true;
}

/**
* @fn void* processThread(void* arg)
* Fonction executee dans un thread
* @param[in] arg : pointeur sur le fichier de configuration du serveur
*/

void* processThread ( void* arg ) {
    // Initialisation du serveur

    pthread_mutex_lock ( &mutex_rok4 );
    void* server=rok4InitServer ( ( char* ) arg );
    pthread_mutex_unlock ( &mutex_rok4 );

    if ( server==0 ) {
        fprintf ( stdout,"Impossible d'initialiser le serveur\n" );
        return 0;
    }
    fprintf ( stdout,"Serveur initialise\n" );

    // Traitement des requetes
    while ( !feof ( requestFile ) ) {
        char query[400],host[400],script[400];
        memset ( query,'\0',400 );
        memset ( host,'\0',400 );
        memset ( script,'\0',400 );
//              pthread_mutex_lock(&mutex_rok4);
        if ( fscanf ( requestFile,"%s\t%s\t%s\n",host,script,query ) !=3 )
            continue;
//              pthread_mutex_unlock(&mutex_rok4);
        fprintf ( stdout,"\nRequete n°%d : %s\t%s\t%s\n",c,host,script,query );
        c++;
        HttpRequest* request=rok4InitRequest ( query,host, script, "" );

        if ( strcmp ( request->service,"wmts" ) !=0 ) {
            fprintf ( stdout,"\tService %s non gere\n",request->service );
            rok4DeleteRequest ( request );
            continue;
        }

        // GetCapabilities
        if ( strcmp ( request->operationType,"getcapabilities" ) ==0 ) {
            HttpResponse* capabilities=rok4GetWMTSCapabilities ( query,"localhost","/target/bin/rok4","",server );
            fprintf ( stdout,"\tStatut=%d\n",capabilities->status );
            fprintf ( stdout,"\ttype=%s\n",capabilities->type );
            fprintf ( stdout,"\tcontentSize=%d\n",capabilities->contentSize );
            FILE* C=fopen ( "capabilities.xml","w" );
            //fprintf(C,"%s",capabilities->content);
            fwrite ( capabilities->content,1,capabilities->contentSize,C );
            fclose ( C );
            rok4DeleteResponse ( capabilities );
        }
        // GetTile
        else if ( strcmp ( request->operationType,"gettile" ) ==0 ) {
            // TileReferences
            TileRef tileRef;
            TilePalette tilePalette;
            HttpResponse* error=rok4GetTileReferences ( query, "localhost", "/target/bin/rok4","", server, &tileRef, &tilePalette );
            if ( error ) {
                fprintf ( stdout,"\tStatut=%d\n",error->status );
                fprintf ( stdout,"\ttype=%s\n",error->type );
                fprintf ( stdout,"\terror content=%s\n",error->content );
                rok4DeleteResponse ( error );
            } else {
                fprintf ( stdout,"\tfilename : %s\noff=%d\nsize=%d\ntype=%s\n",tileRef.filename,tileRef.posoff,tileRef.possize,tileRef.type );
                if ( strcmp ( tileRef.type,"image/tiff" ) ==0 ) {
                    TiffHeader* header=rok4GetTiffHeader ( tileRef.width,tileRef.height,tileRef.channels );
                    fprintf ( stdout,"\tw=%d h=%d c=%d\n\theader=",tileRef.width,tileRef.height,tileRef.channels );
                    int i;
                    for ( i=0;i<128;i++ )
                        fprintf ( stdout,"%c",header->data[i] );
                    fprintf ( stdout,"\n" );
                    rok4DeleteTiffHeader ( header );
                }
                if ( strcmp ( tileRef.type,"image/png" ) ==0 && tileRef.channels==1 && tilePalette.size!=0 ) {
                    PngPaletteHeader* header = rok4GetPngPaletteHeader ( tileRef.width,tileRef.height,&tilePalette );
                    fprintf ( stdout,"\tw=%d h=%d ps=%d\n\theader=",tileRef.width,tileRef.height,tilePalette.size );
                    int i;
                    for ( i=0;i<header->size;i++ )
                        fprintf ( stdout,"%c",header->data[i] );
                    fprintf ( stdout,"\n" );
                    rok4DeletePngPaletteHeader ( header );
                }

                rok4FlushTileRef ( &tileRef );
            }
            free ( error );

            // Tile
            HttpResponse* tile=rok4GetTile ( query, "localhost", "/target/bin/rok4","", server );
            char tileName[20];
            sprintf ( tileName,"test_%d.png",c );
            FILE* T=fopen ( tileName,"w" );
            fwrite ( tile->content,tile->contentSize,1,T );
            fclose ( T );
            rok4DeleteResponse ( tile );

        }
        // Operation non prise en charge
        else {
            HttpResponse* response=rok4GetOperationNotSupportedException ( query, "localhost", "/target/bin/rok4","",server );
            fprintf ( stdout,"\tStatut=%d\n",response->status );
            fprintf ( stdout,"\ttype=%s\n",response->type );
            fprintf ( stdout,"\terror content=%s\n",response->content );
            rok4DeleteResponse ( response );
        }
        rok4DeleteRequest ( request );
    }

    // Extinction du serveur
    rok4KillServer ( server );

    return 0;
}

/**
* Fonction principale
* Lance n threads d'execution du serveur
*/

int main ( int argc, char* argv[] ) {

    // Lecture de la ligne de commande
    char server_config_file[200], request_file[200];
    int nb_threads;
    if ( !parseCommandLine ( argc,argv,server_config_file,&nb_threads,request_file ) ) {
        usage();
        return -1;
    }

    c=0;
    if ( ( requestFile=fopen ( request_file,"r" ) ) ==NULL ) {
        fprintf ( stderr,"Impossible d'ouvrir %s\n",request_file );
        return -1;
    }

    pthread_t* threads= ( pthread_t* ) malloc ( nb_threads*sizeof ( pthread_t ) );
    int i;
    for ( i = 0; i < nb_threads; i++ ) {
        pthread_create ( & ( threads[i] ), NULL, processThread, ( void* ) server_config_file );
    }
    for ( i = 0; i < nb_threads; i++ )
        pthread_join ( threads[i], NULL );

    return 0;
}
