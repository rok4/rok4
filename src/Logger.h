#ifndef _LOGGER_
#define _LOGGER_

#include <ostream>


typedef enum{DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4} LogLevel;
std::ostream &Logger(LogLevel l);

#define LOGGER(x) Logger(x)<<" "<<__FILE__<<":"<<__LINE__<<" in "<<__FUNCTION__<<" "

#endif
