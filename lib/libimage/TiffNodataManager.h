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

#ifndef _TIFFNODATAMANAGER_
#define _TIFFNODATAMANAGER_

#include <stdint.h>
#include <tiff.h>

/**
* @file TiffNodataManager.h
* @brief Tool which remove white pixels in images and keep this color for nodata pixels (front pixels)
* Pixels in images (which don't touch sides) : 'white' -> 'whiteForData
* Front pixels (which touch sides) : 'white' -> 'whiteForNodata'
* By default, whiteForNodata = (255,255,255) and whiteForData = (254,254,254)
* @author IGN
*/

class TiffNodataManager {
private:
    uint16 channels;

    uint8_t *targetValue;
    uint8_t *dataValue;
    uint8_t *nodataValue;

    bool bRemoveTargetValue;
    bool bAddNodataValue;
    
    void addNodataValue(uint8_t* IM, uint32 width , uint32 height,uint16 samplesperpixel);

public: 

    /**
     * Constructor
     * Configure the nodata manager
     */
    TiffNodataManager(uint16 channels, uint8_t* targetValue, uint8_t* dataValue, uint8_t* nodataValue,
                      bool bRemoveTargetValue, bool bAddNodataValue);

    bool treatNodata(char* input, char* output);

};

#endif // _TIFFNODATAMANAGER_
