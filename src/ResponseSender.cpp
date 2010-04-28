
#include "ResponseSender.h"
#include <iostream>
#include <sys/socket.h> 
#include "Logger.h"
#include <cstdio>



int ResponseSender::sendresponse(HttpResponse* response, int conn_fd) { 

  LOGGER(DEBUG) << "================" << std::endl;
  LOGGER(DEBUG) << send(conn_fd, "Status: 200 OK\r\n",16,0) << " " << 16 << std::endl;
  LOGGER(DEBUG) << send(conn_fd, "Content-Type: ",14,0) << " " << 14 << std::endl;
  LOGGER(DEBUG) << send(conn_fd, response->type, strlen(response->type),0) << " " << strlen(response->type) << std::endl;
  LOGGER(DEBUG) << send(conn_fd, "\r\n\r\n",4,0) << " " << 4 << std::endl;
  LOGGER(DEBUG) << "================" << std::endl;

  uint8_t *buffer = new uint8_t[2 << 20];
  int pos = 0;
  while(true) {
    LOGGER(DEBUG) << "position : " << pos << std::endl;
    int size = 2 << 20;
    int nb = response->getdata(buffer, size);
    LOGGER(DEBUG) << "nb " << nb << std::endl;    
    int wr = 0;
    while(wr < nb) {
      int w = 0;

      LOGGER(DEBUG) << "write1 " << wr << " " << w << " " << int(conn_fd) << std::endl;
      w = send(conn_fd, buffer + wr, nb, 0);
      LOGGER(DEBUG) << "write2 " << wr << " " << w << " " << int(conn_fd) <<  std::endl;

      if(w < 0) {
        LOGGER(DEBUG) << "error" << std::endl;
        perror("write");
        break;
      }
      wr += w;
    }
    if(wr != nb) {
        LOGGER(DEBUG) << "error" << std::endl;
        perror("write");
      break;
    }

    pos += nb;
    LOGGER(DEBUG) << "pos " << pos << std::endl;
    if(nb == 0) break;
  }
  LOGGER(DEBUG) << "fini" << std::endl;
  delete response;
  delete[] buffer;


  close(conn_fd);
  return 1;
}



