#include "Logger.h"

#include <fstream>
#include <sstream>
#include <ostream>
#include "sys/time.h"
#include "time.h"
#include <cstdio>
#include <iostream>

/*FIXME: est la bonne facon de definir NBLEVEL en C++?*/
const int NB_ACTION = 2;
typedef enum{IGNORE = 0, WRITE = 1} Action;

const int NBLEVEL = 5;
const char* LogLevelText[NBLEVEL] = { "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL" };


/** L'objet de cette classe est de permettre des écritures concurentes sur le
 * flux de log. De sorte qu'il n'est pas possible que plusieurs threads écrivent
 * sur même ligne.
 * Elle s'occupe de plus de l'horodatage des lignes de log.
 */
/*----------------------------------------------------------------------------*/
class LogBuffer: public std::stringbuf {
private:
	std::ostream &out;
	static pthread_mutex_t mutex;

protected:
	virtual int sync();

public:
	LogBuffer(std::ostream & out) :	std::stringbuf(std::ios_base::out), out(out) {}
};

/* TODO NV: cette valeur est à conservée jusqu'a ce qu'on ai pu lire dans le fichier
 *           de config le fichier de log voulu par l'utilisateur et qu'on a eu
 *           confirmation qu'on peut écrire dedans.
 */
//std::ostream LogBuffer::out = std::cerr; // Définition du flux de sortie des log
//std::ostream LogBuffer::out = std::ofstream ("/var/tmp/rok4.log", std::ios_base::app); // Définition du flux de sortie des log


pthread_mutex_t LogBuffer::mutex = PTHREAD_MUTEX_INITIALIZER;
/** Cette fonction est appelée automatiquement lors du flush.
 *  Elle définit la manière dont le message est écrit dans le fichier
 *  de log (membre "out").
 *  Etant en contexte multithread, elle interdit l'écriture simultanée
 *  par des threads différents.
 */
int LogBuffer::sync() {
	pthread_mutex_lock(&mutex);
	out.write(pbase(), pptr() - pbase());
	pthread_mutex_unlock(&mutex);

	str(""); //vide le stringBuffer (puisqu'on l'a copié)
	return 0;
}



/** On dispose grace à cette classe d'un "stream" qui ignore tout ce qu'on lui
 * envoie.
 */
class NullStream : public std::ostream{
};

template<class T>
NullStream& operator<<(NullStream &stream, T &message) {
	return stream;
};

/*------------------------ gestion des threads -------------------------------*/

static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static pthread_key_t logger_key[NB_ACTION];

/* on cree une clef par action possible (écrire ou ignorer le message)*/
static void init_key() {
	for (int i = 0; i < NB_ACTION; i++)
		pthread_key_create(&logger_key[i], NULL);
}


/* -----------implémentation de la classe principale: Logger -----------------*/

LogLevel Logger::minLevel = DEBUG;

LogLevel Logger::getMinLevel() {
	return minLevel;
}

void Logger::setMinLevel(LogLevel const minLevel) {
	Logger::minLevel = minLevel;

}

/** Renvoie une référence sur le std::ostream qui correspond -pour le processus
 * en cours- au niveau de log "level". Il est alors possible d'écrire dedans
 * simplement ainsi:
 *  Logger(WARN) << "message du warning" << std::endl;
 */
std::ostream &Logger::logStream(LogLevel level) {
	pthread_once(&key_once, init_key); // initialize une seule fois logger_key

	static std::ofstream out ("/var/tmp/rok4.log", std::ios_base::app); 

	std::ostream *L;
	Action action;

	if (level>=minLevel) action=WRITE; else action=IGNORE;

	if ((L = (std::ostream*) pthread_getspecific(logger_key[action])) == NULL) {
		if (action==IGNORE){
			L = new NullStream;
		}else{
			L = new std::ostream(new LogBuffer(out));
		}
		pthread_setspecific(logger_key[action], (void*) L);
	}

	/* mise en forme des informations affichées par défaut */

	timeval tim;
	gettimeofday(&tim, NULL);
	char date[26 + 1];
	tm *now = localtime(&tim.tv_sec);
	sprintf(date, "%04d/%02d/%02d %02d:%02d:%02d.%06d",
			now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour,
			now->tm_min, now->tm_sec, (int) (tim.tv_usec));

	*L << LogLevelText[level] <<" "<< date << " " << pthread_self() << " ";

	return *L;

}


