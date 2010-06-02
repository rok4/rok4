#include "ResponseSender.h"
#include <iostream>
#include <sys/socket.h> 
#include "Logger.h"
#include <stdio.h>


/**
Creation de l'en-tete de request.out
Copie de l'objet HttpResponse response dans le flux de sortie de l'objet FCGX_Request request
*/

int ResponseSender::sendresponse(HttpResponse* response, FCGX_Request* request) {

  FCGX_PutStr("Status: 200 OK\r\n",16,request->out);
  FCGX_PutStr("Content-Type: ",14,request->out);
  FCGX_PutStr(response->type, strlen(response->type),request->out);
  FCGX_PutStr("\r\n\r\n",4,request->out);

  uint8_t *buffer = new uint8_t[2 << 20];
  int pos = 0;
  while(true) {
    int size = 2 << 20;
    int nb = response->getdata(buffer, size);
    int wr = 0;
    while(wr < nb) {
      int w = FCGX_PutStr((char*)(buffer + wr), nb,request->out);

      if(w < 0) {
        LOGGER_DEBUG( "error" );
        perror("write");
        break;
      }
      wr += w;
    }
    if(wr != nb) {
        LOGGER_DEBUG( "error" );
        perror("write");
      break;
    }

    pos += nb;
    LOGGER_DEBUG( "pos " << pos);
    if(nb == 0) break;
  }
  delete response;

  delete[] buffer;

  return 1;
}


