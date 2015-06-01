/*
 * Copyright © (2011-2013) Institut national de l'information
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

/**
 * \file LoggerSpecific.cpp
 * \~french
 * \brief Implémentation de la classe LoggerSpecific pour créer des logs supplémentaires
 * \~english
 * \brief Implement the LoggerSpecific class, to create specific logs
 */

#include "LoggerSpecific.h"

LoggerSpecific::LoggerSpecific(LogType t, LogDefinition d, std::string f, std::ostream &s):type (t),definition (d),file (f), stream(s)
{
    switch (type) {
    case STANDARD_OUTPUT_STREAM_SYNC:
        stream << "**** DEBUT DU LOGGER ****" << std::endl;
        break;
    case STATIC_FILE_SYNC:
        if (file == "") {
            type = STANDARD_OUTPUT_STREAM_SYNC;
            stream << "Impossible d'ecrire dans un fichier non spécifié " << std::endl;
            stream << "**** DEBUT DU LOGGER ****" << std::endl;
        } else {
            std::fstream fstream;
            fstream.open(file.c_str(),std::ios::out | std::ios::app);
            if (!fstream.is_open()) {
                type = STANDARD_OUTPUT_STREAM_SYNC;
                stream << "Impossible d'ouvrir " << file << std::endl;
                stream << "**** DEBUT DU LOGGER ****" << std::endl;
            } else {
                fstream << "**** DEBUT DU LOGGER ****" << std::endl;
                fstream.close();
            }
        }
        break;
    case NOWHERE_SYNC:
        break;
    default:
        break;
    }

    referencePriority[DEBUG_SYNC] = 5;
    referencePriority[INFO_SYNC] = 4;
    referencePriority[WARN_SYNC] = 3;
    referencePriority[ERROR_SYNC] = 2;
    referencePriority[FATAL_SYNC] = 1;
}

void LoggerSpecific::write(LogDefinition level, std::string message) {

    std::fstream fstream;
    int t = referencePriority[definition];
    int u = referencePriority[level];

    if(referencePriority[level] <= referencePriority[definition] ) {

        switch (type) {
        case STANDARD_OUTPUT_STREAM_SYNC:
            stream << message << std::endl;
            break;
        case STATIC_FILE_SYNC:
            fstream.open(file.c_str(),std::ios::out | std::ios::app);
            fstream << message << std::endl;
            fstream.close();
            break;
        case NOWHERE_SYNC:
            break;
        default:
            break;
        }

    }

}

LoggerSpecific::~LoggerSpecific()
{

}

