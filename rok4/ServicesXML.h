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

#ifndef SERVICESXML_H
#define SERVICESXML_H

#include <vector>
#include <string>

#include "Keyword.h"
#include "MetadataURL.h"
#include "CRS.h"

#include "config.h"
#include "intl.h"

class ServicesXML
{
    public:
        ServicesXML(std::string servicesConfigFile);
        ~ServicesXML(){
        }

        bool isOk() { return ok; }

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
            return &mtdWMS;
        }
        MetadataURL inline *getWMTSMetadataURL() {
            return &mtdWMTS;
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
        bool inline getDoWeRestrictCRSList() {
            return dowerestrictCRSList;
        }
        std::vector<std::string> getRestrictedCRSList() {
            return restrictedCRSList;
        }


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
        std::string metadataUrlWMS;
        std::string metadataMediaTypeWMS;
        std::string metadataUrlWMTS;
        std::string metadataMediaTypeWMTS;

        // CRS
        bool doweuselistofequalsCRS;
        bool addEqualsCRS;
        bool dowerestrictCRSList;
        std::vector<std::string> listofequalsCRS;
        std::vector<std::string> restrictedCRSList;

        MetadataURL mtdWMS;
        MetadataURL mtdWMTS;

        bool ok;
};

#endif // SERVICESXML_H

