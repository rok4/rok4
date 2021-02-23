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


#include <dirent.h>
#include <tinyxml.h>
#include "ServicesXML.h"


ServicesXML::ServicesXML (const ServicesXML & obj) {
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
    mtdWMS = obj.mtdWMS;
    mtdWMTS = obj.mtdWMTS;
    mtdTMS = obj.mtdTMS;
}

ServicesXML::ServicesXML( std::string servicesConfigFile ) {
    ok = false;

    /********************** Default values */

    name="";
    title="";
    abstract="";
    serviceProvider="";
    fee="";
    accessConstraint="";
    postMode = false;
    providerSite="";
    individualName="";
    individualPosition="";
    voice="";
    facsimile="";
    addressType="";
    deliveryPoint="";
    city="";
    administrativeArea="";
    postCode="";
    country="";
    electronicMailAddress="";
    fullStyling = false;
    serviceType="";
    serviceTypeVersion="";
    inspire = false;
    doweuselistofequalsCRS = false;
    addEqualsCRS = false;
    dowerestrictCRSList = false;

    /********************** Parse */

    LOGGER_INFO ( _ ( "Construction de la configuration des services depuis " ) <<servicesConfigFile );
    TiXmlDocument doc ( servicesConfigFile );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "Ne peut pas charger le fichier " ) << servicesConfigFile );
        return;
    }

    TiXmlHandle hDoc ( &doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element();
    if ( !pElem ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( " impossible de recuperer la racine." ) );
        return;
    }
    if ( pElem->ValueStr() != "servicesConf" ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( " La racine n'est pas un servicesConf." ) );
        return;
    }
    hRoot=TiXmlHandle ( pElem );

    pElem=hRoot.FirstChild ( "name" ).Element();
    if ( pElem && pElem->GetText() ) name = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "title" ).Element();
    if ( pElem && pElem->GetText() ) title = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "abstract" ).Element();
    if ( pElem && pElem->GetText() ) abstract = DocumentXML::getTextStrFromElem(pElem);


    for ( pElem=hRoot.FirstChild ( "keywordList" ).FirstChild ( "keyword" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "keyword" ) ) {
        
        if ( ! ( pElem->GetText() ) ) continue;

        std::map<std::string,std::string> attributes;
        TiXmlAttribute* attrib = pElem->FirstAttribute();
        while ( attrib ) {
            attributes.insert ( attribute ( attrib->NameTStr(),attrib->ValueStr() ) );
            attrib = attrib->Next();
        }
        keyWords.push_back ( Keyword ( DocumentXML::getTextStrFromElem(pElem),attributes ) );
    }

    pElem=hRoot.FirstChild ( "postMode" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string postStr = DocumentXML::getTextStrFromElem(pElem);
        if ( postStr.compare ( "true" ) ==0 || postStr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Requete POST autorisee" ) );
            postMode = true;
        }
    }

    /********************** Contact Info */

    pElem=hRoot.FirstChild ( "serviceProvider" ).Element();
    if ( pElem && pElem->GetText() ) serviceProvider = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "providerSite" ).Element();
    if ( pElem && pElem->GetText() ) providerSite = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "fee" ).Element();
    if ( pElem && pElem->GetText() ) fee = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "accessConstraint" ).Element();
    if ( pElem && pElem->GetText() ) accessConstraint = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "individualName" ).Element();
    if ( pElem && pElem->GetText() ) individualName= DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "individualPosition" ).Element();
    if ( pElem && pElem->GetText() ) individualPosition= DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "voice" ).Element();
    if ( pElem && pElem->GetText() ) voice= DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "facsimile" ).Element();
    if ( pElem && pElem->GetText() ) facsimile= DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "addressType" ).Element();
    if ( pElem && pElem->GetText() ) addressType = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "deliveryPoint" ).Element();
    if ( pElem && pElem->GetText() ) deliveryPoint= DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "city" ).Element();
    if ( pElem && pElem->GetText() ) city = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "administrativeArea" ).Element();
    if ( pElem && pElem->GetText() ) administrativeArea = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "postCode" ).Element();
    if ( pElem && pElem->GetText() ) postCode = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "country" ).Element();
    if ( pElem && pElem->GetText() ) country = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "electronicMailAddress" ).Element();
    if ( pElem && pElem->GetText() )
    electronicMailAddress= DocumentXML::getTextStrFromElem(pElem);

    pElem = hRoot.FirstChild ( "layerLimit" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        layerLimit = DEFAULT_LAYER_LIMIT;
    } else if ( !sscanf ( pElem->GetText(),"%d",&layerLimit ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le layerLimit est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" );
        return;
    }

    pElem = hRoot.FirstChild ( "maxWidth" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        maxWidth=MAX_IMAGE_WIDTH;
    } else if ( !sscanf ( pElem->GetText(),"%d",&maxWidth ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le maxWidth est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" );
        return;
    }

    pElem = hRoot.FirstChild ( "maxHeight" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        maxHeight=MAX_IMAGE_HEIGHT;
    } else if ( !sscanf ( pElem->GetText(),"%d",&maxHeight ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le maxHeight est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" );
        return;
    }

    pElem = hRoot.FirstChild ( "maxTileX" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        maxTileX=MAX_TILE_X;
    } else if ( !sscanf ( pElem->GetText(),"%d",&maxTileX ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le maxTileX est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" );
        return;
    }

    pElem = hRoot.FirstChild ( "maxTileY" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        maxTileY=MAX_TILE_Y;
    } else if ( !sscanf ( pElem->GetText(),"%d",&maxTileY ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le maxTileY est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" );
        return;
    }

    for ( pElem=hRoot.FirstChild ( "formatList" ).FirstChild ( "format" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "format" ) ) {
        
        if ( ! ( pElem->GetText() ) ) continue;

        std::string format ( pElem->GetText() );
        if ( format != "image/jpeg" &&
            format != "image/png"  &&
            format != "image/tiff" &&
            format != "image/geotiff" &&
            format != "image/x-bil;bits=32" &&
            format != "image/gif" && 
            format != "text/asc" ) {
            LOGGER_ERROR ( servicesConfigFile << _ ( "le format d'image [" ) << format << _ ( "] n'est pas un type MIME pris en charge" ) );
        } else {
            formatList.push_back ( format );
        }
    }

    for ( pElem=hRoot.FirstChild ( "infoFormatList" ).FirstChild ( "format" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "format" ) ) {
        
        if ( ! ( pElem->GetText() ) ) continue;

        std::string format ( pElem->GetText() );
        // Pas de vérification pour pouvoir autoriser des formats non gérés par Rok4 mais par un Géoserver en back.
        infoFormatList.push_back ( format );
    }

    pElem=hRoot.FirstChild ( "avoidEqualsCRSReprojection" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string doweuselistofequalsCRSstr = DocumentXML::getTextStrFromElem(pElem);
        if ( doweuselistofequalsCRSstr.compare ( "true" ) ==0 || doweuselistofequalsCRSstr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Pas de reprojection pour les CRS equivalents" ) );
            doweuselistofequalsCRS = true;
            listofequalsCRS = ConfLoader::loadListEqualsCRS();
        }
    }

    pElem=hRoot.FirstChild ( "addEqualsCRS" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string addEqualsCRSstr = DocumentXML::getTextStrFromElem(pElem);
        if ( addEqualsCRSstr.compare ( "true" ) ==0 || addEqualsCRSstr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Ajout automatique des CRS equivalents" ) );
            addEqualsCRS = true;
            if (!doweuselistofequalsCRS){
                listofequalsCRS = ConfLoader::loadListEqualsCRS();
            }
        }
    }

    pElem=hRoot.FirstChild ( "restrictedCRSList" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string restritedCRSListfile = DocumentXML::getTextStrFromElem(pElem);
        if ( restritedCRSListfile.compare ( "" ) != 0 )  {
            LOGGER_INFO ( _ ( "Liste restreinte de CRS à partir du fichier " ) << restritedCRSListfile );
            dowerestrictCRSList = true;
            restrictedCRSList = ConfLoader::loadStringVectorFromFile(restritedCRSListfile);
        }
    }

    //Global CRS List
    for ( pElem=hRoot.FirstChild ( "globalCRSList" ).FirstChild ( "crs" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "crs" ) ) {
        
        if ( ! ( pElem->GetText() ) ) continue;

        std::string crsStr ( DocumentXML::getTextStrFromElem(pElem) );
        CRS crs ( crsStr );
        if ( !crs.isProj4Compatible() ) {
            LOGGER_ERROR ( servicesConfigFile << _ ( "The CRS [" ) << crsStr << _ ( "] is not present in Proj4" ) );
        } else {
            std::vector<CRS> tmpEquilist;
            if (addEqualsCRS){
                tmpEquilist = ConfLoader::getEqualsCRS(listofequalsCRS, crsStr );
            }
            bool allowedCRS = true;
            if ( dowerestrictCRSList ){
                allowedCRS = ConfLoader::isCRSAllowed(restrictedCRSList, crsStr, tmpEquilist);
            }
            if (allowedCRS) {
                bool found = false;
                for ( int i = 0; i<globalCRSList.size() ; i++ ) {
                    if (globalCRSList.at( i ).getRequestCode().compare( crs.getRequestCode() ) == 0 ){
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    globalCRSList.push_back ( crs );
                    LOGGER_INFO ( _ ( "Adding global CRS " ) << crsStr  );
                } else {
                    LOGGER_WARN ( _ ( "Already present in global CRS list " ) << crsStr  );
                }
                for (unsigned int l = 0; l< tmpEquilist.size();l++) {
                    found = false;
                    for ( int i = 0; i<globalCRSList.size() ; i++ ){
                        if ( globalCRSList.at( i ) == tmpEquilist.at( l ) ){
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        globalCRSList.push_back( tmpEquilist.at( l ) );
                        LOGGER_INFO ( _ ( "Adding equivalent global CRS [" ) << tmpEquilist.at( l ).getRequestCode() << _ ( "] of [" ) << crsStr << "]" );
                    } else {
                        LOGGER_WARN ( _ ( "Already present in global CRS list " ) << tmpEquilist.at( l ).getRequestCode()  );   
                    }
                }
            } else {
                LOGGER_WARN ( _ ( "Forbiden global CRS " ) << crsStr  );
            }
        }
    }

    //Add CRS:84 if not defined in services.config
    {
        bool crs84Found = false;
        for ( int i =0 ; i < globalCRSList.size(); i++ ) {
            if ( globalCRSList.at ( i ).getRequestCode().compare ( "CRS:84" ) ==0 ) {
                crs84Found = true;
                break;
            }
        }
        if ( !crs84Found ) {
            LOGGER_INFO ( _ ( "CRS:84 not found -> adding global CRS CRS:84" )  );
            globalCRSList.push_back ( CRS ( "CRS:84" ) );
            std::vector<CRS> tmpEquilist = ConfLoader::getEqualsCRS(listofequalsCRS, "CRS:84" );
            for (unsigned int l = 0; l< tmpEquilist.size();l++){
                globalCRSList.push_back( tmpEquilist.at( l ) );
                LOGGER_INFO ( _ ( "Adding equivalent global CRS [" ) << tmpEquilist.at( l ).getRequestCode() << _ ( "] of [CRS:84]") );
            }
        }
    }

    pElem=hRoot.FirstChild ( "fullStylingCapability" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string styleStr = DocumentXML::getTextStrFromElem(pElem);
        if ( styleStr.compare ( "true" ) ==0 || styleStr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Utilisation des styles pour tous les formats" ) );
            fullStyling = true;
        }
    }


    pElem=hRoot.FirstChild ( "serviceType" ).Element();
    if ( pElem && pElem->GetText() ) serviceType = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "serviceTypeVersion" ).Element();
    if ( pElem && pElem->GetText() ) serviceTypeVersion = DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "inspire" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string inspirestr = DocumentXML::getTextStrFromElem(pElem);
        if ( inspirestr.compare ( "true" ) ==0 || inspirestr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Utilisation du mode Inspire" ) );
            inspire = true;
        }
    }


    pElem=hRoot.FirstChild ( "styleURL" ).Element();
    if ( pElem && pElem->GetText() ) styleURL = DocumentXML::getTextStrFromElem(pElem);

    std::string metadataUrlWMS;
    std::string metadataMediaTypeWMS;
    std::string metadataUrlWMTS;
    std::string metadataMediaTypeWMTS;
    std::string metadataUrlTMS;
    std::string metadataMediaTypeTMS;

    pElem=hRoot.FirstChild ( "metadataWMS" ).Element();
    if ( pElem ) {
        pElem = pElem->FirstChildElement ( "url" );
        if ( pElem &&  pElem->GetText() ) {
            metadataUrlWMS = DocumentXML::getTextStrFromElem(pElem);
            pElem = pElem->NextSiblingElement ( "mediaType" );
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
        if ( pElem &&  pElem->GetText() ) {
            metadataMediaTypeWMS = DocumentXML::getTextStrFromElem(pElem);
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
    }

    pElem=hRoot.FirstChild ( "metadataWMTS" ).Element();
    if ( pElem ) {
        pElem = pElem->FirstChildElement ( "url" );
        if ( pElem &&  pElem->GetText() ) {
            metadataUrlWMTS = DocumentXML::getTextStrFromElem(pElem);
            pElem = pElem->NextSiblingElement ( "mediaType" );
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
        if ( pElem &&  pElem->GetText() ) {
            metadataMediaTypeWMTS = DocumentXML::getTextStrFromElem(pElem);
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
    }

    pElem=hRoot.FirstChild ( "metadataTMS" ).Element();
    if ( pElem ) {
        pElem = pElem->FirstChildElement ( "url" );
        if ( pElem &&  pElem->GetText() ) {
            metadataUrlTMS = DocumentXML::getTextStrFromElem(pElem);
            pElem = pElem->NextSiblingElement ( "mediaType" );
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
        if ( pElem &&  pElem->GetText() ) {
            metadataMediaTypeTMS = DocumentXML::getTextStrFromElem(pElem);
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
    }

    mtdWMS = new MetadataURL ( "simple",metadataUrlWMS,metadataMediaTypeWMS );
    mtdWMTS = new MetadataURL ( "simple",metadataUrlWMTS,metadataMediaTypeWMTS );
    mtdTMS = new MetadataURL ( "simple",metadataUrlTMS,metadataMediaTypeTMS );

    ok = true;
}

// Check if two CRS are equivalent
//   A list of equivalent CRS was created during server initialization
// TODO: return false if servicesconf tells we don't check the equality
bool ServicesXML::are_the_two_CRS_equal( std::string crs1, std::string crs2 ) {
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

ServicesXML::~ServicesXML(){ 
    delete mtdWMS;
    delete mtdWMTS;
    delete mtdTMS;
}

bool ServicesXML::isOk() { return ok; }

std::string ServicesXML::getAbstract() const { return abstract; }
std::string ServicesXML::getAccessConstraint() const { return accessConstraint; }
std::string ServicesXML::getFee() const { return fee; }
std::vector<Keyword> * ServicesXML::getKeyWords() { return &keyWords; }
bool ServicesXML::isPostEnabled() { return postMode; }
std::string ServicesXML::getServiceProvider() const { return serviceProvider; }
std::string ServicesXML::getTitle() const { return title; }
//ContactInfo
std::string ServicesXML::getProviderSite() const { return providerSite; }
std::string ServicesXML::getIndividualName() const { return individualName; }
std::string ServicesXML::getIndividualPosition() const { return individualPosition; }
std::string ServicesXML::getVoice() const { return voice; }
std::string ServicesXML::getFacsimile() const { return facsimile; }
std::string ServicesXML::getAddressType() const { return addressType; }
std::string ServicesXML::getDeliveryPoint() const { return deliveryPoint; }
std::string ServicesXML::getCity() const { return city; }
std::string ServicesXML::getAdministrativeArea() const { return administrativeArea; }
std::string ServicesXML::getPostCode() const { return postCode; }
std::string ServicesXML::getCountry() const { return country; }
std::string ServicesXML::getElectronicMailAddress() const { return electronicMailAddress; }
// WMS
unsigned int ServicesXML::getLayerLimit() const { return layerLimit; }
unsigned int ServicesXML::getMaxHeight() const { return maxHeight; }
unsigned int ServicesXML::getMaxWidth() const { return maxWidth; }
unsigned int ServicesXML::getMaxTileX() const { return maxTileX; }
unsigned int ServicesXML::getMaxTileY() const { return maxTileY; }
std::string ServicesXML::getName() const { return name; }
std::vector<std::string>* ServicesXML::getFormatList() { return &formatList; }
bool ServicesXML::isInFormatList(std::string f) {
    for ( unsigned int k = 0; k < formatList.size(); k++ ) {
        if ( formatList.at(k) == f ) {
            return true;
        }
    }
    return false;
}
std::vector<std::string>* ServicesXML::getInfoFormatList() { return &infoFormatList; }
bool ServicesXML::isInInfoFormatList(std::string f) {
    for ( unsigned int k = 0; k < infoFormatList.size(); k++ ) {
        if ( infoFormatList.at(k) == f ) {
            return true;
        }
    }
    return false;
}

std::vector<CRS>* ServicesXML::getGlobalCRSList() { return &globalCRSList; }
bool ServicesXML::isInGlobalCRSList(CRS* c) {
    for ( unsigned int k = 0; k < globalCRSList.size(); k++ ) {
        if ( c->cmpRequestCode ( globalCRSList.at (k).getRequestCode() ) ) {
            return true;
        }
    }
    return false;
}


bool ServicesXML::isFullStyleCapable() { return fullStyling; }
// WMTS
std::string ServicesXML::getServiceType() { return serviceType; }
std::string ServicesXML::getServiceTypeVersion() { return serviceTypeVersion; }
bool ServicesXML::isInspire() { return inspire; }
// TMS
std::string ServicesXML::getStyleURL() { return styleURL; }

MetadataURL* ServicesXML::getWMSMetadataURL() { return mtdWMS; }
MetadataURL* ServicesXML::getWMTSMetadataURL() { return mtdWMTS; }
// CRS
bool ServicesXML::getDoWeUseListOfEqualsCRS() { return doweuselistofequalsCRS; }
bool ServicesXML::getAddEqualsCRS() { return addEqualsCRS; }
std::vector<std::string> ServicesXML::getListOfEqualsCRS() { return listofequalsCRS; }
bool ServicesXML::getDoWeRestrictCRSList() { return dowerestrictCRSList; }
std::vector<std::string> ServicesXML::getRestrictedCRSList() { return restrictedCRSList; }
