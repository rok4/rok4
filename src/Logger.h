#ifndef _LOGGER_
#define _LOGGER_

#include <ostream>


typedef enum{DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4} LogLevel;
typedef enum{IGNORE = 0, WRITE = 1} Action;
#define NB_ACTION 2

class Logger{
private:
	static int const NBLEVEL;
	static char* const logLevelName[];
	static LogLevel minLevel; // niveau de log minimum écrit dans le fichier de log.
	static string logFileName;

public:
	static LogLevel getMinLevel();
	static void setMinLevel(LogLevel const minLevel);
	static string getLogFileName();
	static void setLogFileName(string fileName);

	static std::ostream &logStream(LogLevel level);
};


#define LOGGER(x) (Logger::getMinLevel()==DEBUG ? Logger::logStream(x) << "["<<__FILE__<<":"<<__LINE__<<" in "<<__FUNCTION__<<"]:" : Logger::logStream(x) )

#endif
