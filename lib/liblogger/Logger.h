#ifndef _LOGGER_
#define _LOGGER_

#include <ostream>
#include <vector>
#include "Accumulator.h"

typedef enum {
	ROLLING_FILE = 0,
	STANDARD_OUTPUT_STREAM_FOR_ERRORS
} LogOutput;

typedef enum {	
	FATAL = 0,
	ERROR,
	WARN,
	INFO,
	DEBUG,
	nbLogLevel // ceci n'est pas un logLevel mais juste un hack pour avoir une constante qui compte le nombre de logLevel
} LogLevel;


class Logger {
	private:
		
		// TODO: ce serait plus propre d'utiliser des shared_ptr
		static Accumulator* accumulator[nbLogLevel];
		static LogOutput logOutput;
	public:
		/**
		 * Obtient un pointeur vers la sortie du niveau de log.
		 *
		 * @return la valeur de retour peut être un pointeur nul, ce qui signifie que le niveau de log est désactivé.
		 */
		inline static Accumulator* getAccumulator(LogLevel level) {
			return accumulator[level];
		}

		/**
		 * Définit la sortie d'un niveau de log.
		 *
		 * Pour désactiver un niveau de log, utiliser un pointeur nul
		 * Le même accumulateur peut être utilisé par plusieurs niveau de log.
		 *
		 * La classe Logger se charge de détruire les Accumulateurs non utilisés.
		 *
		 * Attention cette fonction n'est pas threadsafe et ne doit être utilisée que
		 * par un unique thread.
		 */
		static void setAccumulator(LogLevel level, Accumulator *A);

		/**
		 * utilisation : Logger(DEBUG) << message
		 */
		static std::ostream& getLogger(LogLevel level);

		inline static void setOutput(LogOutput output) {logOutput=output;}
		inline static LogOutput& getOutput() {return logOutput;}
};

/**
 * Un flux qui ne fait rien.
 * Utilisé pour ne pas formater les sorties des niveaux logs désactivés.
 */
extern std::ostream nullstream;

//#define LOGGER(x) (Logger::getAccumulator(x)?Logger::getLogger(x):nullstream)
#define LOGGER(x) (Logger::getOutput()==ROLLING_FILE?(Logger::getAccumulator(x)?Logger::getLogger(x):nullstream):std::cerr)

#define LOGGER_DEBUG(m) LOGGER(DEBUG)<<"pid="<<getpid()<<" "<<__FILE__<<":"<<__LINE__<<" in "<<__FUNCTION__<<" "<<m<<std::endl
#define LOGGER_INFO(m) LOGGER(INFO)<<"pid="<<getpid()<<" "<<m<<std::endl
#define LOGGER_WARN(m) LOGGER(WARN)<<"pid="<<getpid()<<" "<<m<<std::endl
#define LOGGER_ERROR(m) LOGGER(ERROR)<<"pid="<<getpid()<<" "<<m<<std::endl
#define LOGGER_FATAL(m) LOGGER(FATAL)<<"pid="<<getpid()<<" "<<m<<std::endl


#endif
