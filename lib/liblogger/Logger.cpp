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

#include "Logger.h"

#include <sstream>
#include <ostream>
#include "sys/time.h"
#include "time.h"
#include <cstdio>
#include <iostream>
#include <fstream>

/* nullstream est initialisé sans streambuf, son badbit sera activé et les formatages seront alors ignorées */
std::ostream nullstream(0);

LogOutput Logger::logOutput=ROLLING_FILE;

const char* LogLevelText[nbLogLevel] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

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
    if (acc) acc->addMessage(str());
    str("");
    return 0;
}

Accumulator* Logger::accumulator[nbLogLevel] = {0};
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t logger_key[nbLogLevel];

static void init_key() {
    for (int i = 0; i < nbLogLevel; i++) pthread_key_create(&logger_key[i], 0);
}

void Logger::setAccumulator(LogLevel level, Accumulator* A) {
    // On ajoute une petite tache d'initialisation qui doit être
    // effectuée une seule fois. Il faut bien la mettre qqpart car
    // nous n'avons pas de constructeur ni de condition d'initialisation.
    pthread_once(&key_once, init_key); // initialize une seule fois logger_key


    Accumulator* prev = accumulator[level];
    accumulator[level] = A;

    // On cherche si l'Accumulateur est encore utilisé
    bool last = true;
    for (int i = 0; i < nbLogLevel; i++) if (prev == accumulator[level]) last = false;

    // On le détruit le cas échéant.
    if (prev && last) delete prev;
}

void Logger::setCurrentAccumulator(LogLevel level, Accumulator* A) {

    accumulator[level] = A;

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
    *L << date << "\t";
    return *L;
}

void Logger::stopLogger()
{
    for ( int i = 0 ; i < nbLogLevel ; i++ ) {
        std::ostream *L = (std::ostream*) pthread_getspecific(logger_key[( LogLevel ) i]);
        if (L != 0) {
            delete (logbuffer*) L->rdbuf(); // Delete the logbuffer associated with the outputstream
            delete L;
            pthread_setspecific(logger_key[( LogLevel ) i], (void*) NULL);
        }
    }
}

