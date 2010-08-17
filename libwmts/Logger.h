#ifndef _LOGGER_
#define _LOGGER_

#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
#include <iostream>


typedef enum{DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4} LogLevel;

class Logger {
private:
public:
        static log4cxx::LoggerPtr logger;

public:
        static void configure(std::string confFileName);
};

#define LOGGER_DEBUG(m) LOG4CXX_DEBUG(Logger::logger,m)
#define LOGGER_INFO(m)  LOG4CXX_INFO( Logger::logger,m)
#define LOGGER_WARN(m)  LOG4CXX_WARN( Logger::logger,m)
#define LOGGER_ERROR(m) LOG4CXX_ERROR(Logger::logger,m)
#define LOGGER_FATAL(m) LOG4CXX_FATAL(Logger::logger,m)
 

#endif
