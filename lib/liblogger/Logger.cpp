#include "Logger.h"

#include <sstream>
#include <ostream>
#include "sys/time.h"
#include "time.h"
#include <cstdio>
#include <iostream>
#include <fstream>

const char* LogLevelText[nbLogLevel] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

class logbuffer : public std::stringbuf {
  private:
	LogLevel level;

  protected:
  virtual int sync();
  public:
  logbuffer(LogLevel level) : std::stringbuf(std::ios_base::out), level(level) {}
};

int logbuffer::sync() {	
	Accumulator* acc = Logger::getAccumulator(level);
	if(acc) acc->addMessage(str());
	str("");
	return 0;
}


Accumulator* Logger::accumulator[nbLogLevel] = {0};
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t logger_key[nbLogLevel];
static void init_key() {
  for(int i = 0; i < nbLogLevel; i++) pthread_key_create(&logger_key[i], 0);
}

void Logger::setAccumulator(LogLevel level, Accumulator* A) {
  // On ajoute une petite tache d'initialisation qui doit être 
  // effectuée une seule fois. Il faut bien la mettre qqpart car 
  // nous n'avons pas de constructeur ni de contion d'initialisation.
  pthread_once(&key_once, init_key); // initialize une seule fois logger_key


  Accumulator* prev = accumulator[level];
  accumulator[level] = A;

  // On cherche si l'Accumulateur est encore utilisé 
  bool last = true;
  for(int i = 0; i <= FATAL; i++) if(prev == accumulator[level]) last = false;
  
  // On le détruit le cas échéant.
  if(prev && last) delete prev;
}



std::ostream& Logger::getLogger(LogLevel level) {
	std::ostream *L;
	if ((L = (std::ostream*) pthread_getspecific(logger_key[level])) == 0) {
		L = new std::ostream(new logbuffer(level));
		pthread_setspecific(logger_key[level], (void*) L);
	}

	timeval tim;
	gettimeofday(&tim, NULL);
	char date[64];
	tm *now = localtime(&tim.tv_sec);
  sprintf(date, "%04d/%02d/%02d %02d:%02d:%02d.%06d\t", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, (int) (tim.tv_usec));
	*L << date << LogLevelText[level] << '\t';
	return *L;
}
