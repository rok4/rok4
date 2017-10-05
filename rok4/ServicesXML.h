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

class ServicesXML;

#ifndef SERVICESXML_H
#define SERVICESXML_H

#include <vector>
#include <string>

#include "Keyword.h"
#include "ConfLoader.h"
#include "MetadataURL.h"
#include "CRS.h"

#include "config.h"
#include "intl.h"

class ServicesXML
{
    public:
        ServicesXML(std::string servicesConfigFile);
        ServicesConf (const ServicesConf & obj): metadataWMS (obj.metadataWMS), metadataWMTS (metadataWMTS) {
        title = obj.title;
        abstract = obj.abstract;
        keyWords = obj.keyWords;
        serviceProvider = obj.serviceProvider;
        fee = obj.fee;
        accessConstraint = obj.accessConstraint;
        postMode = obj.postMode;
        providerSite = obj.providerSite;
        individualName = obj.individualName;
        individualPosition = obj.individualPosition;
        voice = obj.voice;
        facsimile = obj.facsimile;
        addressType = obj.addressType;
        deliveryPoint = obj.deliveryPoint;
        city = obj.city;
        administrativeArea = obj.administrativeArea;
        postCode = obj.postCode;
        country = obj.country;
        electronicMailAddress = obj.electronicMailAddress;
        name = obj.name;
        layerLimit = obj.layerLimit;
        maxWidth = obj.maxWidth;
        maxHeight = obj.maxHeight;
        maxTileX = obj.maxTileX;
        maxTileY = obj.maxTileY;
        formatList = obj.formatList;
        infoFormatList = obj.infoFormatList;
        globalCRSList = obj.globalCRSList;
        fullStyling = obj.fullStyling;
        serviceType = obj.serviceType;
        serviceTypeVersion = obj.serviceTypeVersion;
        inspire = obj.inspire;
        doweuselistofequalsCRS = obj.doweuselistofequalsCRS;
        listofequalsCRS = obj.listofequalsCRS;
        addEqualsCRS = obj.addEqualsCRS;
        dowerestrictCRSList = obj.dowerestrictCRSList;
        restrictedCRSList = obj.restrictedCRSList;
        }
        ~ServicesXML();

        bool isOk() ;

        std::string getAbstract() const      ;
        std::string getAccessConstraint() const ;
        std::string getFee() const ;
        std::vector<Keyword> * getKeyWords() ;
        bool isPostEnabled() ;
        std::string getServiceProvider() const ;
        std::string getTitle() const ;
        //ContactInfo
        std::string getProviderSite() const ;
        std::string getIndividualName() const ;
        std::string getIndividualPosition() const ;
        std::string getVoice() const ;
        std::string getFacsimile() const ;
        std::string getAddressType() const ;
        std::string getDeliveryPoint() const ;
        std::string getCity() const ;
        std::string getAdministrativeArea() const ;
        std::string getPostCode() const ;
        std::string getCountry() const ;
        std::string getElectronicMailAddress() const ;
        // WMS
        unsigned int getLayerLimit() const ;
        unsigned int getMaxHeight() const ;
        unsigned int getMaxWidth() const ;
        unsigned int getMaxTileX() const ;
        unsigned int getMaxTileY() const ;
        std::string getName() const ;
        std::vector<std::string>* getFormatList() ;
        std::vector<std::string>* getInfoFormatList() ;
        std::vector<CRS>* getGlobalCRSList() ;
        bool isFullStyleCapable() ;
        // WMTS
        std::string getServiceType() ;
        std::string getServiceTypeVersion() ;
        bool isInspire() ;

        MetadataURL* getWMSMetadataURL() ;
        MetadataURL* getWMTSMetadataURL() ;
        // CRS
        bool getDoWeUseListOfEqualsCRS() ;
        bool getAddEqualsCRS() ;
        std::vector<std::string> getListOfEqualsCRS() ;
        bool getDoWeRestrictCRSList() ;
        std::vector<std::string> getRestrictedCRSList() ;
        bool are_the_two_CRS_equal( std::string crs1, std::string crs2 );


    protected:

    private:

        std::string name;
        std::string title;
        std::string abstract;
        std::vector<Keyword> keyWords;
        std::string serviceProvider;
        std::string fee;
        std::string accessConstraint;
        unsigned int layerLimit;
        unsigned int maxWidth;
        unsigned int maxHeight;
        unsigned int maxTileX;
        unsigned int maxTileY;
        bool postMode;

        // Contact Info
        std::string providerSite;
        std::string individualName;
        std::string individualPosition;
        std::string voice;
        std::string facsimile;
        std::string addressType;
        std::string deliveryPoint;
        std::string city;
        std::string administrativeArea;
        std::string postCode;
        std::string country;
        std::string electronicMailAddress;

        //WMS
        std::vector<std::string> formatList;
        std::vector<std::string> infoFormatList;
        std::vector<CRS> globalCRSList;
        bool fullStyling;

        //WMTS
        std::string serviceType;
        std::string serviceTypeVersion;

        //INSPIRE
        bool inspire;
        std::vector<std::string> applicationProfileList;

        // CRS
        bool doweuselistofequalsCRS;
        bool addEqualsCRS;
        bool dowerestrictCRSList;
        std::vector<std::string> listofequalsCRS;
        std::vector<std::string> restrictedCRSList;

        MetadataURL* mtdWMS;
        MetadataURL* mtdWMTS;

        bool ok;
};

#endif // SERVICESXML_H

