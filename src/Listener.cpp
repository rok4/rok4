

//#include <sys/socket.h> 
#include <netinet/in.h>
#include "Listener.h"
#include "Logger.h"
#include <cstdio>


  int Listener::init() {
    while((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 && running) {
      perror("Listener (socket)");
      sleep(1);
    }

    struct sockaddr_in sin;    
    memset(&sin, 0, sizeof(sin));  
    sin.sin_family = AF_INET;         
    sin.sin_port = htons(port);  
    sin.sin_addr.s_addr = INADDR_ANY;

    while(bind(socket_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0 && running) {
      perror("Listener (bind)");
      sleep(1);
    }

    if(listen(socket_fd, 1) < 0) {
      perror("Listener (listen) ");
      return -1;
    }
    return 1;
  }



  int Listener::start() {
    pthread_mutex_lock(&mutex);
    if(running || socket_fd >= 0) {
      pthread_mutex_unlock(&mutex);
      return 0;
    }
    running = true;
    pthread_create(&thread_id, NULL, Listener::run, (void*) this);
    pthread_mutex_unlock(&mutex);    
    return 1;
  }

  void Listener::stop() {
    //TODO: envoyer un signal pour tuer le thread ?
    pthread_mutex_lock(&mutex);
    running = false;
    pthread_mutex_unlock(&mutex);
  }



  /*
   * Boucle principale exécuté par un thread dédié
   */
  void* Listener::run(void* arg) {
    Listener *L = (Listener*) (arg);
    L->init();
    while(L->running) {
      int conn_fd = L->listen_connection();
      if(conn_fd >= 0) L->Q.push(conn_fd);
    }
    close(L->socket_fd);
    L->socket_fd = -1;
    return 0;
  }


  int Listener::listen_connection() {

      timeval tt;
      tt.tv_sec = 1;
      tt.tv_usec = 0;
      fd_set fdset_read;
      FD_ZERO(&fdset_read);
      FD_SET(socket_fd, &fdset_read);

//      if(select(FD_SETSIZE, &fdset_read, 0, 0, &tt) == 0) {
//        std::cerr << "select 0" << std::endl;
//        return -1;
//      }

    int conn_fd = accept(socket_fd, 0, 0);
    LOGGER(DEBUG) << "Listener : " << conn_fd << std::endl;
    if(conn_fd < 0) {
      perror("Listener (accept) ");
      return -1;
    }
    return conn_fd;
  }

  int Listener::get_connection() {
    return Q.pop();
  }
 

