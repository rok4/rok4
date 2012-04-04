/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière 
 * 
 * Géoportail SAV <geop_services@geoportail.fr>
 * 
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 * 
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * 
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

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
		
		static void stopLogger();
};

/**
 * Un flux qui ne fait rien.
 * Utilisé pour ne pas formater les sorties des niveaux logs désactivés.
 */
extern std::ostream nullstream;

//#define LOGGER(x) (Logger::getAccumulator(x)?Logger::getLogger(x):nullstream)
//#define LOGGER(x) (Logger::getOutput()==ROLLING_FILE?(Logger::getAccumulator(x)?Logger::getLogger(x):nullstream):std::cerr)
#define LOGGER(x) (Logger::getAccumulator(x)?(Logger::getOutput()==ROLLING_FILE?Logger::getLogger(x):std::cerr):nullstream)

#define LOGGER_DEBUG(m) LOGGER(DEBUG)<<"pid="<<getpid()<<" "<<__FILE__<<":"<<__LINE__<<" in "<<__FUNCTION__<<" "<<m<<std::endl
#define LOGGER_INFO(m) LOGGER(INFO)<<"pid="<<getpid()<<" "<<m<<std::endl
#define LOGGER_WARN(m) LOGGER(WARN)<<"pid="<<getpid()<<" "<<m<<std::endl
#define LOGGER_ERROR(m) LOGGER(ERROR)<<"pid="<<getpid()<<" "<<m<<std::endl
#define LOGGER_FATAL(m) LOGGER(FATAL)<<"pid="<<getpid()<<" "<<m<<std::endl


#endif
