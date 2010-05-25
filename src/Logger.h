#ifndef _LOGGER_
#define _LOGGER_

#include <ostream>


typedef enum{DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4} LogLevel;

class Logger {
private:
	static LogLevel minLevel; // niveau de log minimum écrit dans le fichier de log.
	//TODO: fichier de sortie
public:
	static LogLevel getMinLevel();
	static void setMinLevel(LogLevel const minLevel);

	static std::ostream &logStream(LogLevel level);
};


#define LOGGER(x) (Logger::getMinLevel()==DEBUG ? Logger::logStream(x) << "["<<__FILE__<<":"<<__LINE__<<" in "<<__FUNCTION__<<"]:" : Logger::logStream(x) )

#endif
