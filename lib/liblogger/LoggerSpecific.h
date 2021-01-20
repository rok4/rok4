/*
 * Copyright © (2011-2013) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
 * \file LoggerSpecific.h
 * \~french
 * \brief Implémentation de la classe LoggerSpecific pour créer des logs supplémentaires
 * \~english
 * \brief Implement the LoggerSpecific class, to create specific logs
 */


#ifndef LOGGERSPECIFIC_H
#define LOGGERSPECIFIC_H

#include <string>
#include <iostream>
#include <fstream>
#include <map>



/**
 * \~french \brief Liste des types de log que l'on peut avoir
 * On peut avoir un fichier qui sera réécrit selon une période
 * Une sortie standard ou un autre flux
 * Un fichier static
 * Ou enfin ne rien rediriger
 * \~english \brief Log type we want
 */
typedef enum {
    STANDARD_OUTPUT_STREAM_SYNC,
    STATIC_FILE_SYNC,
    NOWHERE_SYNC
} LogType;

/**
 * \~french \brief Liste des niveaux de criticité du log que l'on souhaite
 * \~english \brief Log level we want
 */

typedef enum {
    FATAL_SYNC,
    ERROR_SYNC,
    WARN_SYNC,
    INFO_SYNC,
    DEBUG_SYNC
} LogDefinition;


/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Un LoggerSpecific stocke les informations utiles pour créer un log où et quand on veut.
 * Mais sans utilisation d'accumulateurs. Les écritures sont faites directement par les
 * threads qui les initient.
 * \brief Gestion des logs
 * \~english
 * The LoggerSpecific stores information concerning logs created when and where we want.
 * Without using accumulators.
 * \brief Manipulating logs
 */
class LoggerSpecific
{

private:

    /**
     * \~french \brief Type du logger
     * \~english \brief Logger type
     */
    LogType type;

    /**
     * \~french \brief Definition du logger
     * \~english \brief Logger Definition
     */
    LogDefinition definition;

    /**
     * \~french \brief Fichier où on peut logger
     * \~english \brief File used to log
     */
    std::string file;

    /**
     * \~french \brief Flux utilisé pour logger
     * \~english \brief Stream used to log
     */
    std::ostream &stream;

    /**
     * \~french \brief Priorité de référence
     * \~english \brief Reference priority
     */
    std::map<LogDefinition,int> referencePriority;


public:

    /**
     * \~french \brief Constructeur
     * \param[in] t type
     * \param[in] d definition
     * \param[in] f file
     * \param[in] s stream
     * \~english \brief Constructor
     * \param[in] t type
     * \param[in] d definition
     * \param[in] f file
     * \param[in] s stream
     */
    LoggerSpecific(LogType t = STANDARD_OUTPUT_STREAM_SYNC, LogDefinition d = ERROR_SYNC, std::string f = "", std::ostream &s = std::cerr);

    /**
     * \~french
     * \brief Modifie la valeur type
     * \param[in] t type
     * \~english
     * \brief Set type
     * \param[in] t type
     */
    void setType(LogType t) {
        switch (t) {
        case STANDARD_OUTPUT_STREAM_SYNC:
            type = t;
            break;
        case STATIC_FILE_SYNC:
            if (file != "") {
                type = t;
            }
            break;
        case NOWHERE_SYNC:
            type = t;
            break;
        default:
            break;
        }

    }

    /**
     * \~french
     * \brief Modifie la valeur definition
     * \param[in] d definition
     * \~english
     * \brief Set definition
     * \param[in] d definition
     */
    void setDefinition(LogDefinition d) {
        definition = d;
    }

    /**
     * \~french
     * \brief Récupère la valeur definition
     * \return definition
     * \~english
     * \brief Get definition
     * \return definition
     */
    LogDefinition getDefinition() {
        return definition;
    }

    /**
     * \~french
     * \brief Modifie la valeur file
     * \param[in] f file
     * \~english
     * \brief Set file
     * \param[in] f file
     */
    void setFile(std::string f) {
        std::fstream fstream;
        fstream.open(f.c_str(),std::ios::out | std::ios::app);
        if (fstream.is_open()) {
            file = f;
        }
    }

    /**
     * \~french
     * \brief Récupère la valeur file
     * \return file
     * \~english
     * \brief Get file
     * \return file
     */
    std::string getFile() {
        return file;
    }

    /**
     * \~french
     * \brief Modifie la valeur stream
     * \param[in] s stream
     * \~english
     * \brief Set stream
     * \param[in] s stream
     */
    void setStream(std::ostream &s) {
        std::streambuf *buf = s.rdbuf();
        stream.rdbuf(buf);
        //stream = s;
    }

    /**
     * \~french
     * \brief Récupère la valeur stream
     * \return stream
     * \~english
     * \brief Get stream
     * \return stream
     */
    std::ostream &getStream() {
        return stream;
    }

    /**
     * \~french \brief Ecrire dans la sortie désignée par file et/ou stream
     * param[in] message à écrire
     * param[in] level du message
     * \~english \brief Write in output pointed by file and/or stream
     * param[in] message to write
     * param[in] level of the message
     */
    void write(LogDefinition level, std::string message);

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    ~LoggerSpecific();
};

#endif // LOGGERSPECIFIC_H
