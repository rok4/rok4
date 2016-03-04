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

/**
 * \file Context.h
 ** \~french
 * \brief Définition de la classe Context
 * \details
 * \li Context : classe d'abstraction du contexte de stockage (fichier, ceph ou swift)
 ** \~english
 * \brief Define classe Context
 * \details
 * \li Context : storage context abstraction
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>// pour uint8_t
#include "Logger.h"
#include <string.h>

/**
 * \~french \brief Énumération des types de contextes
 * \~english \brief Available context type
 */
enum eContextType {
    FILECONTEXT,
    CEPHCONTEXT,
    SWIFTCONTEXT
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un contexte de stockage abstrait 
 */
class Context {  

protected:

    bool connected;

    /** Constructeurs */
    Context () : connected(false) {}

public:

    virtual bool connection() = 0;

    virtual int read(uint8_t* data, int offset, int size, std::string name) = 0;
    virtual bool write(uint8_t* data, int offset, int size, std::string name) = 0;
    virtual bool writeFull(uint8_t* data, int size, std::string name) = 0;

    virtual bool openToWrite(std::string name) = 0;
    virtual bool closeToWrite(std::string name) = 0;
    virtual eContextType getType() = 0;

    /**
     * \~french
     * \brief Sortie des informations sur le contexte
     * \~english
     * \brief Context description output
     */
    virtual void print() = 0;
    
    virtual ~Context() {}
};

#endif
