#include "Logger.h"

#include <sstream>
#include <ostream>
#include "sys/time.h"
#include "time.h"
#include <cstdio>
#include <iostream>

const char* LogLevelText[5] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

class logbuffer : public std::stringbuf {
  private:
  static std::ostream *out;
  static pthread_mutex_t mutex;
  pthread_t thread_id;
  const char* level;

  protected:
  virtual int sync();
  public:
  logbuffer(LogLevel level) : std::stringbuf(std::ios_base::out), thread_id(pthread_self()), level(LogLevelText[level]) {}
};

std::ostream *logbuffer::out = &std::cerr; // Définition du flux de sortie des log
pthread_mutex_t logbuffer::mutex = PTHREAD_MUTEX_INITIALIZER;




int logbuffer::sync() {    
  timeval tim;
  gettimeofday(&tim, NULL);
  char date[32+1];
  tm *now = localtime(&tim.tv_sec);
  //FIXME:NV:cause de plantage 64bit et pb de sécurité
  sprintf(date, "%s %04d/%02d/%02d %02d:%02d:%02d.%06d", level, now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, (int) (tim.tv_usec));

  pthread_mutex_lock(&mutex);
    (*out) << date << "\t" << thread_id << "\t";
    out->write(pbase(), pptr() - pbase());
  pthread_mutex_unlock(&mutex);

  str("");
  return 0;
}




static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t logger_key[5];
static void init_key() {
  for(int i = 0; i < 5; i++) 
    pthread_key_create(&logger_key[i], 0);
}

std::ostream &Logger(LogLevel level) {
  pthread_once(&key_once, init_key); // initialize une seule fois logger_key

  std::ostream *L;
  if ((L = (std::ostream*) pthread_getspecific(logger_key[level])) == 0) {
    L = new std::ostream(new logbuffer(level));
    pthread_setspecific(logger_key[level], (void*) L);
  }
  return *L;
}


class NullStream {};
template<class T> 
NullStream& operator<<(NullStream &stream, T &message) {return stream;};



