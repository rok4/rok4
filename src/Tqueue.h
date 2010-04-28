#ifndef _TQUEUE_
#define _TQUEUE_

#include "config.h"
#include <pthread.h>

/*
 * File D'attente FIFO thread safe.
 * Plusieurs thread peuvent lire et écrire sur cette file.
 */
template<typename T> 
class Tqueue {
  private:
  int capacity, front, size;
  T* Buffer;
  pthread_mutex_t mutex; 
  pthread_cond_t  condition;

  public:
  Tqueue<T>(int capacity = 1024) : capacity(capacity), front(0), size(0) {
    assert(capacity > 0);
    Buffer = new T[capacity];
    pthread_mutex_init(&mutex, 0);
    pthread_cond_init(&condition, 0);
  }

  ~Tqueue() {
    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&mutex);
    delete[] Buffer;
  }

  /*
   * Récupère un élément.
   * Attention bloque tant que la file est vide.
   */
  T pop() {
    pthread_mutex_lock(&mutex);
    while(size == 0) pthread_cond_wait(&condition, &mutex);

    std::cerr << "pop  " << size << std::endl;
    
    T ret = Buffer[front];
    front = (front+1) % capacity;
    size--;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
    return ret;
  }

  /*
   * Ajoute un élément.
   * Attention bloque tant que la file est pleine.
   */
  void push(T t) {
    pthread_mutex_lock(&mutex);
    while(size == capacity) pthread_cond_wait(&condition, &mutex);

    std::cerr << "push " << size << std::endl;

    Buffer[(front + size)%capacity] = t;
    size++;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
  }
};
#endif
