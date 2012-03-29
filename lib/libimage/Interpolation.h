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

#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <string>

//To declare a new interpolation change the implementation too



namespace Interpolation {
    
    /**
 * Type définissant les différentes méthodes de d'interpolation.
 *
 *
 */
enum KernelType {
    UNKNOWN = 0,
    NEAREST_NEIGHBOUR = 1,
    LINEAR = 2,
    CUBIC = 3,
    LANCZOS_2 = 4,
    LANCZOS_3 = 5,
    LANCZOS_4 = 6
};
    
    
    KernelType fromString(std::string strInterpolation);
    std::string toString(KernelType interpolation);
    std::string toBe4String(KernelType interpolation);

}

#endif //INTERPOLATION_H
