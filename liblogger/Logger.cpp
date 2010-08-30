#include "Logger.h"

#include <sstream>
#include <ostream>
#include "sys/time.h"
#include "time.h"
#include <cstdio>
#include <iostream>
#include <fstream>

const char* LogLevelText[5] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

class logbuffer : public std::stringbuf {
  friend class Logger;
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

std::ostream *logbuffer::out = 0; // Définition du flux de sortie des log
pthread_mutex_t logbuffer::mutex = PTHREAD_MUTEX_INITIALIZER;




int logbuffer::sync() {    
  timeval tim;
  gettimeofday(&tim, NULL);
  char date[32+1];
  tm *now = localtime(&tim.tv_sec);
  //FIXME:NV:cause de plantage 64bit et pb de sécurité
  sprintf(date, "%s %04d/%02d/%02d %02d:%02d:%02d.%06d", level, now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, (int) (tim.tv_usec));

  pthread_mutex_lock(&mutex);
    if(out) {
      (*out) << date << "\t" << thread_id << "\t";
      out->write(pbase(), pptr() - pbase());
      out->flush();
    }

    std::cerr << date << "\t" << thread_id << "\t";
    std::cerr.write(pbase(), pptr() - pbase());
    std::cerr.flush();    
  pthread_mutex_unlock(&mutex);

  str("");
  return 0;
}


void Logger::configure(std::string confFileName) {
  static std::ofstream F;
  F.open(confFileName.c_str(), std::ios_base::app | std::ios_base::out);
  logbuffer::out = &F;
}

static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t logger_key[5];
static void init_key() {
  for(int i = 0; i < 5; i++) 
    pthread_key_create(&logger_key[i], 0);
}

std::ostream &logger(LogLevel level) {
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



