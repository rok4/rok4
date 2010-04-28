#ifndef _LISTENER_
#define _LISTENER_

#include "config.h"
#include "Tqueue.h"
#include <pthread.h>

class Listener {
  private:
  pthread_t thread_id;
  pthread_mutex_t mutex; 
  
  Tqueue<int> Q;
  int socket_fd;
  int port;
  bool running;

  static void* run(void* arg);
  int init();
  int listen_connection();
  public:

  /*
   * Lance le Listener.
   *
   * retourne 1 si lancé
   * retourne 0 si non-lancé ()
   */
  int start();

  /*
   * Arrête l'écoute du port.
   */
  void stop();

  /*
   * Retourne un descripteur de fichier vers une socket entrante.
   * Si aucune connection n'est disponible, cette fonction bloque jusqu'à l'arrivée d'une connection.
   * Cette fonction est thread safe et peut être apellée par plusieurs threads.
   * Si plusieurs threads appellent la fonction, chaque socket ne sera retournée qu'à un seul thread apellant.
   */  
  int get_connection();

  /*
   * Constructeur, prenant le port à écouter en paramètre.
   * Le Listener doit être lancé via start() après création.
   */
  Listener(int port = 8080) : socket_fd(-1), port(port), running(false) {
    pthread_mutex_init(&mutex, 0);
  }

  ~Listener() {    
    pthread_mutex_destroy(&mutex);
  }

};


#endif
