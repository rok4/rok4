#ifndef _LOGGER_
#define _LOGGER_

#include <ostream>




typedef enum{DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4} LogLevel;
std::ostream &logger(LogLevel l);

class Logger {
public:
   static void configure(std::string logFileName);
};


#define LOGGER(x) logger(x)<<" "<<__FILE__<<":"<<__LINE__<<" in "<<__FUNCTION__<<" "
#define LOGGER_DEBUG(m) logger(DEBUG)<<" "<<__FILE__<<":"<<__LINE__<<" in "<<__FUNCTION__<<" "<<m<<std::endl
#define LOGGER_INFO(m)  logger(INFO)<<m<<std::endl
#define LOGGER_WARN(m)  logger(WARN)<<m<<std::endl
#define LOGGER_ERROR(m) logger(ERROR)<<m<<std::endl
#define LOGGER_FATAL(m) logger(FATAL)<<m<<std::endl


#endif
