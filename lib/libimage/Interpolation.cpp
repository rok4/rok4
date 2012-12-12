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
 * \file Interpolation.cpp
 * \~french
 * \brief Implémentation de l'énumération des interpolations disponibles et des conversions
 * \~english
 * \brief Define available interpolations' enumeration and conversions
 */

#include "Interpolation.h"
#include <string.h>

namespace Interpolation {
    
    const char *einterpolation_name[] = {
        "UNKNOWN",
        "nn",
        "linear",
        "bicubic",
        "lanczos_2",
        "lanczos_3",
        "lanczos_4"
    };

    const int einterpolation_size = 6;


    Interpolation::KernelType fromString(std::string strInterpolation)
    {
        int i;
        //handle be4 element : lanczos to lanczos_2
        if (strInterpolation.compare("lanczos")==0){
            strInterpolation.append("_2");
        }

        for (i=einterpolation_size; i ; --i) {
            if (strInterpolation.compare(einterpolation_name[i])==0)
                break;
        }

        return static_cast<Interpolation::KernelType>(i);
    }

    std::string toString(Interpolation::KernelType KT)
    {
        return std::string(einterpolation_name[KT]);
    }

    std::string toBe4String(Interpolation::KernelType KT)
    {
        if (KT >= Interpolation::LANCZOS_2) {
            return std::string("lanczos");
        }
        return std::string(einterpolation_name[KT]);
    }

}
