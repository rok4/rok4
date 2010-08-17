#include "Logger.h"

log4cxx::LoggerPtr Logger::logger (log4cxx::Logger::getLogger( "Global" ));

void Logger::configure(std::string confFileName){
    log4cxx::xml::DOMConfigurator::configure(confFileName);
}

