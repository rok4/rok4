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

#ifndef SERVICESCONF_H_
#define SERVICESCONF_H_

#include <string>
#include <vector>
#include <algorithm>
#include "MetadataURL.h"
#include "CRS.h"
#include "Keyword.h"

class ServicesConf {
private:
    // WMS & WMTS
    std::string title;
    std::string abstract;
    std::vector<Keyword> keyWords;
    std::string serviceProvider;
    std::string fee;
    std::string accessConstraint;
    bool postMode;
    //Contact Info
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

    // WMS std::string
    std::string name;
    unsigned int layerLimit;
    unsigned int maxWidth;
    unsigned int maxHeight;
    unsigned int maxTileX;
    unsigned int maxTileY;
    std::vector<std::string> formatList;
    std::vector<std::string> infoFormatList;
    std::vector<CRS> globalCRSList;
    bool fullStyling;
    // WMTS
    std::string serviceType;
    std::string serviceTypeVersion;
    //Inspire
    bool inspire;
    MetadataURL metadataWMS;
    MetadataURL metadataWMTS;
    // List of equals CRS from file
    bool doweuselistofequalsCRS; // if true we check if 2 CRS don't have the same name but are equals before reprojecting
    std::vector<std::string> listofequalsCRS;
    bool addEqualsCRS;
    bool dowerestrictCRSList;
    std::vector<std::string> restrictedCRSList;

public:

    ServicesConf ( std::string name, std::string title, std::string abstract, std::vector<Keyword> keyWords,
                   std::string serviceProvider, std::string fee, std::string accessConstraint, unsigned int layerLimit,
                   unsigned int maxWidth, unsigned int maxHeight, unsigned int maxTileX, unsigned int maxTileY, std::vector<std::string> formatList,std::vector<std::string> infoFormatList,std::vector<CRS> globalCRSList ,  std::string serviceType,
                   std::string serviceTypeVersion, std::string providerSite, std::string individualName,
                   std::string individualPosition, std::string voice, std::string facsimile, std::string addressType,
                   std::string deliveryPoint, std::string city, std::string administrativeArea, std::string postCode,
                   std::string country, std::string electronicMailAddress , MetadataURL metadataWMS, MetadataURL metadataWMTS,
                   std::vector<std::string> listofequalsCRS, std::vector<std::string> restrictedCRSList,
                   bool postMode=0,bool fullStyling=0, bool inspire=0, bool doweuselistofequalsCRS=0, bool addEqualsCRS=0, bool dowerestrictCRSList=0) :
        name ( name ), title ( title ), abstract ( abstract ), keyWords ( keyWords ),
        serviceProvider ( serviceProvider ), fee ( fee ), accessConstraint ( accessConstraint ), layerLimit ( layerLimit ),
        maxWidth ( maxWidth ), maxHeight ( maxHeight ), maxTileX ( maxTileX ), maxTileY ( maxTileY ) , formatList ( formatList ), infoFormatList ( infoFormatList ), globalCRSList ( globalCRSList ), serviceType ( serviceType ),
        serviceTypeVersion ( serviceTypeVersion ) ,individualName ( individualName ),
        individualPosition ( individualPosition ), voice ( voice ), facsimile ( facsimile ), addressType ( addressType ),
        deliveryPoint ( deliveryPoint ), city ( city ), administrativeArea ( administrativeArea ), postCode ( postCode ),
        country ( country ), electronicMailAddress ( electronicMailAddress ), metadataWMS ( metadataWMS ), metadataWMTS ( metadataWMTS ),
        listofequalsCRS ( listofequalsCRS ), restrictedCRSList ( restrictedCRSList ),
        postMode ( postMode ), fullStyling ( fullStyling ), inspire ( inspire ),doweuselistofequalsCRS ( doweuselistofequalsCRS ), addEqualsCRS ( addEqualsCRS ), dowerestrictCRSList (dowerestrictCRSList)  {};
    //  WMS & WMTS
    std::string inline getAbstract() const      {
        return abstract;
    }
    std::string inline getAccessConstraint() const {
        return accessConstraint;
    }
    std::string inline getFee() const {
        return fee;
    }
    std::vector<Keyword> * getKeyWords() {
        return &keyWords;
    }
    bool inline isPostEnabled() {
        return postMode;
    }
    std::string inline getServiceProvider() const {
        return serviceProvider;
    }
    std::string inline getTitle() const {
        return title;
    }
    //ContactInfo
    std::string inline getProviderSite() const {
        return providerSite;
    }
    std::string inline getIndividualName() const {
        return individualName;
    }
    std::string inline getIndividualPosition() const {
        return individualPosition;
    }
    std::string inline getVoice() const {
        return voice;
    }
    std::string inline getFacsimile() const {
        return facsimile;
    }
    std::string inline getAddressType() const {
        return addressType;
    }
    std::string inline getDeliveryPoint() const {
        return deliveryPoint;
    }
    std::string inline getCity() const {
        return city;
    }
    std::string inline getAdministrativeArea() const {
        return administrativeArea;
    }
    std::string inline getPostCode() const {
        return postCode;
    }
    std::string inline getCountry() const {
        return country;
    }
    std::string inline getElectronicMailAddress() const {
        return electronicMailAddress;
    }
    // WMS
    unsigned int inline getLayerLimit() const {
        return layerLimit;
    }
    unsigned int inline getMaxHeight() const {
        return maxHeight;
    }
    unsigned int inline getMaxWidth() const {
        return maxWidth;
    }
    unsigned int inline getMaxTileX() const {
        return maxTileX;
    }
    unsigned int inline getMaxTileY() const {
        return maxTileY;
    }
    std::string inline getName() const {
        return name;
    }
    std::vector<std::string>* getFormatList() {
        return &formatList;
    }
    std::vector<std::string>* getInfoFormatList() {
        return &infoFormatList;
    }
    std::vector<CRS>* getGlobalCRSList() {
        return &globalCRSList;
    }
    bool inline isFullStyleCapable() {
        return fullStyling;
    }
    // WMTS
    std::string inline getServiceType() {
        return serviceType;
    }
    std::string inline getServiceTypeVersion() {
        return serviceTypeVersion;
    }
    bool inline isInspire() {
        return inspire;
    }

    MetadataURL inline *getWMSMetadataURL() {
        return &metadataWMS;
    }
    MetadataURL inline *getWMTSMetadataURL() {
        return &metadataWMTS;
    }
    // CRS
    bool inline getDoWeUseListOfEqualsCRS() {
        return doweuselistofequalsCRS;
    }
     bool inline getAddEqualsCRS() {
        return addEqualsCRS;
    }
    std::vector<std::string> getListOfEqualsCRS() {
        return listofequalsCRS;
    }
    void setListOfEqualsCRS(std::vector<std::string> newList) {
        listofequalsCRS.clear();
        for (unsigned i=0; i<newList.size(); i++) {
           listofequalsCRS.push_back(newList[i]);
        }
    }
    bool inline getDoWeRestrictCRSList() {
        return dowerestrictCRSList;
    }
    std::vector<std::string> getRestrictedCRSList() {
        return restrictedCRSList;
    }
    void setRestrictedCRSList(std::vector<std::string> newList) {
        restrictedCRSList.clear();
        for (unsigned i=0; i<newList.size(); i++) {
           restrictedCRSList.push_back(newList[i]);
        }
    }
    // Check if two CRS are equivalent
    //   A list of equivalent CRS was created during server initialization
    // TODO: return false if servicesconf tells we don't check the equality
    bool are_the_two_CRS_equal( std::string crs1, std::string crs2 ) {
        // Could have issues with lowercase name -> we put the CRS in upercase
        transform(crs1.begin(), crs1.end(), crs1.begin(), toupper);
        transform(crs2.begin(), crs2.end(), crs2.begin(), toupper);
        crs1.append(" ");
        crs2.append(" ");
        for (int line_number = 0 ; line_number < listofequalsCRS.size() ; line_number++) {
            std::string line = listofequalsCRS.at(line_number);
            // We check if the two CRS are on the same line inside the file. If yes then they are equivalent.
            std::size_t found1 = line.find(crs1);
            if ( found1 != std::string::npos  )  {
                std::size_t found2 = line.find(crs2);
                if ( found2 != std::string::npos  )  {
                    LOGGER_DEBUG ( "The two CRS (source and destination) are equals and were found on line  " << line );
                    return true;
                }
            }
        }
        return false; // The 2 CRS were not found on the same line inside the list
    }
};

#endif /* SERVICESCONF_H_ */
