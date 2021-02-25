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

#include "LayerXML.h"

#include <libgen.h>

LayerXML::LayerXML(std::string path, ServerXML* serverXML, ServicesXML* servicesXML ) : DocumentXML ( path )
{
    ok = false;

    TiXmlDocument doc ( filePath.c_str() );
    if ( !doc.LoadFile() ) {
        BOOST_LOG_TRIVIAL(error) <<  _ ( "Ne peut pas charger le fichier " ) << filePath ;
        return;
    }

    BOOST_LOG_TRIVIAL(info) <<  _ ( "           Ajout du layer " ) << filePath ;
    BOOST_LOG_TRIVIAL(info) <<  _ ( "           BaseDir Relative to : " ) << parentDir ;

    /********************** Default values */

    bool inspire = servicesXML->isInspire();

    title="";
    abstract="";
    styleName="";

    authority="";
    resamplingStr="";

    WMSauth = true;
    WMTSauth = true;
    TMSauth = true;

    getFeatureInfoAvailability = false;
    getFeatureInfoType = "";
    getFeatureInfoBaseURL = "";
    GFIService = "";
    GFIVersion = "";
    GFIQueryLayers = "";
    GFILayers = "";
    GFIForceEPSG = true;

    /********************** Parse */

    TiXmlHandle hDoc ( &doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        BOOST_LOG_TRIVIAL(error) <<  filePath << _ ( " impossible de recuperer la racine." ) ;
        return;
    }
    if ( strcmp ( pElem->Value(),"layer" ) ) {
        BOOST_LOG_TRIVIAL(error) <<  filePath << _ ( " La racine n'est pas un layer." ) ;
        return;
    }
    hRoot=TiXmlHandle ( pElem );

    unsigned int idBegin=filePath.rfind ( "/" );
    if ( idBegin == std::string::npos ) {
        idBegin=0;
    }
    unsigned int idEnd=filePath.rfind ( ".lay" );
    if ( idEnd == std::string::npos ) {
        idEnd=filePath.rfind ( ".LAY" );
        if ( idEnd == std::string::npos ) {
            idEnd=filePath.size();
        }
    }
    id=filePath.substr ( idBegin+1, idEnd-idBegin-1 );

    if ( Request::containForbiddenChars(id) ) {
        BOOST_LOG_TRIVIAL(error) <<  _ ( "Layer " ) << id <<_ ( " : l'identifiant de la couche contient des caracteres interdits" ) ;
        return;
    }


    pElem=hRoot.FirstChild ( "title" ).Element();
    if ( pElem && pElem->GetText() ) title= DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "abstract" ).Element();
    if ( pElem && pElem->GetText() ) abstract= DocumentXML::getTextStrFromElem(pElem);

    pElem=hRoot.FirstChild ( "WMSAuthorized" ).Element();
    if ( pElem && pElem->GetText() && DocumentXML::getTextStrFromElem(pElem)=="false") WMSauth= false;

    pElem=hRoot.FirstChild ( "TMSAuthorized" ).Element();
    if ( pElem && pElem->GetText() && DocumentXML::getTextStrFromElem(pElem)=="false") TMSauth= false;

    pElem=hRoot.FirstChild ( "WMTSAuthorized" ).Element();
    if ( pElem && pElem->GetText() && DocumentXML::getTextStrFromElem(pElem)=="false") WMTSauth= false;


    for ( pElem=hRoot.FirstChild ( "keywordList" ).FirstChild ( "keyword" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "keyword" ) ) {
        if ( ! ( pElem->GetText() ) )
            continue;
        std::map<std::string,std::string> attributes;
        TiXmlAttribute* attrib = pElem->FirstAttribute();
        while ( attrib ) {
            attributes.insert ( attribute ( attrib->NameTStr(),attrib->ValueStr() ) );
            attrib = attrib->Next();
        }
        keyWords.push_back ( Keyword ( DocumentXML::getTextStrFromElem(pElem),attributes ) );
    }


    // EX_GeographicBoundingBox
    pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).Element();
    if ( !pElem ) {
        BOOST_LOG_TRIVIAL(error) <<  _ ( "Pas de geographicBoundingBox = " ) ;
        return;
    } else {
        // westBoundLongitude
        pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).FirstChild ( "westBoundLongitude" ).Element();
        if ( !pElem  || ! ( pElem->GetText() ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Pas de westBoundLongitude" ) ;
            return;
        }
        if ( !sscanf ( pElem->GetText(),"%lf",&geographicBoundingBox.minx ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Le westBoundLongitude est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" ;
            return;
        }
        // southBoundLatitude
        pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).FirstChild ( "southBoundLatitude" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Pas de southBoundLatitude" ) ;
            return;
        }
        if ( !sscanf ( pElem->GetText(),"%lf",&geographicBoundingBox.miny ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Le southBoundLatitude est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" ;
            return;
        }
            // eastBoundLongitude
        pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).FirstChild ( "eastBoundLongitude" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Pas de eastBoundLongitude" ) ;
            return;
        }
        if ( !sscanf ( pElem->GetText(),"%lf",&geographicBoundingBox.maxx ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Le eastBoundLongitude est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" ;
            return;
        }
            // northBoundLatitude
        pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).FirstChild ( "northBoundLatitude" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Pas de northBoundLatitude" ) ;
            return;
        }
        if ( !sscanf ( pElem->GetText(),"%lf",&geographicBoundingBox.maxy ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Le northBoundLatitude est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" ;
            return;
        }
    }

    pElem = hRoot.FirstChild ( "boundingBox" ).Element();
    if ( !pElem ) {
        BOOST_LOG_TRIVIAL(error) <<  _ ( "Pas de BoundingBox" ) ;
    } else {
        if ( ! ( pElem->Attribute ( "CRS" ) ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Le CRS est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" ;
            return;
        }
        boundingBox.srs=pElem->Attribute ( "CRS" );
        if ( ! ( pElem->Attribute ( "minx" ) ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "minx attribute is missing" ) ;
            return;
        }
        if ( !sscanf ( pElem->Attribute ( "minx" ),"%lf",&boundingBox.minx ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Le minx est inexploitable:[" ) << pElem->Attribute ( "minx" ) << "]" ;
            return;
        }
        if ( ! ( pElem->Attribute ( "miny" ) ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "miny attribute is missing" ) ;
            return;
        }
        if ( !sscanf ( pElem->Attribute ( "miny" ),"%lf",&boundingBox.miny ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Le miny est inexploitable:[" ) << pElem->Attribute ( "miny" ) << "]" ;
            return;
        }
        if ( ! ( pElem->Attribute ( "maxx" ) ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "maxx attribute is missing" ) ;
            return;
        }
        if ( !sscanf ( pElem->Attribute ( "maxx" ),"%lf",&boundingBox.maxx ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Le maxx est inexploitable:[" ) << pElem->Attribute ( "maxx" ) << "]" ;
            return;
        }
        if ( ! ( pElem->Attribute ( "maxy" ) ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "maxy attribute is missing" ) ;
            return;
        }
        if ( !sscanf ( pElem->Attribute ( "maxy" ),"%lf",&boundingBox.maxy ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Le maxy est inexploitable:[" ) << pElem->Attribute ( "maxy" ) << "]" ;
            return;
        }
    }


    pElem=hRoot.FirstChild ( "authority" ).Element();
    if ( pElem && pElem->GetText() ) {
        authority= DocumentXML::getTextStrFromElem(pElem);
    }


    //MetadataURL Elements , mandatory in INSPIRE
    for ( pElem=hRoot.FirstChild ( "MetadataURL" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "MetadataURL" ) ) {
        std::string format;
        std::string href;
        std::string type;

        if ( pElem->QueryStringAttribute ( "type",&type ) != TIXML_SUCCESS ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << _ ( "MetadataURL type missing" ) ;
            continue;
        }

        TiXmlHandle hMetadata ( pElem );
        TiXmlElement *pElemMTD = hMetadata.FirstChild ( "Format" ).Element();
        if ( !pElemMTD || !pElemMTD->GetText() ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << _ ( "MetadataURL Format missing" ) ;
            continue;
        }
        format = pElemMTD->GetText();
        pElemMTD = hMetadata.FirstChild ( "OnlineResource" ).Element();
        if ( !pElemMTD || pElemMTD->QueryStringAttribute ( "xlink:href",&href ) != TIXML_SUCCESS ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << _ ( "MetadataURL HRef missing" ) ;
            continue;
        }

        metadataURLs.push_back ( MetadataURL ( format,href,type ) );
    }

    if ( metadataURLs.size() == 0 && inspire ) {
        BOOST_LOG_TRIVIAL(error) <<  _ ( "No MetadataURL found in the layer " ) << filePath <<_ ( " : not compatible with INSPIRE!!" ) ;
        return;
    }

    pElem=hRoot.FirstChild ( "pyramid" ).Element();
    if ( pElem && pElem->GetText() ) {

        pyramidFilePath = std::string( DocumentXML::getTextStrFromElem(pElem) );
        //Relative Path
        if ( pyramidFilePath.compare ( 0,2,"./" ) ==0 ) {
            pyramidFilePath.replace ( 0,1,parentDir );
        } else if ( pyramidFilePath.compare ( 0,1,"/" ) !=0 ) {
            pyramidFilePath.insert ( 0,"/" );
            pyramidFilePath.insert ( 0,parentDir );
        }
        pyramid = ConfLoader::buildPyramid ( pyramidFilePath, serverXML, servicesXML, true);
        if ( ! pyramid ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "La pyramide " ) << pyramidFilePath << _ ( " ne peut etre chargee" ) ;
            return;
        }
    } else {
        BOOST_LOG_TRIVIAL(error) <<  _ ( "Aucune pyramide associee au layer " ) << filePath ;
        return;
    }

    if (Rok4Format::isRaster(pyramid->getFormat())) {
        /******************* PYRAMIDE RASTER *********************/

        pElem=hRoot.FirstChild("getFeatureInfoAvailability").Element();
        if ( pElem && pElem->GetText() && DocumentXML::getTextStrFromElem(pElem)=="true") {
            getFeatureInfoAvailability= true;

            pElem=hRoot.FirstChild("getFeatureInfoType").Element();
            if ( pElem && pElem->GetText()) {
                getFeatureInfoType = DocumentXML::getTextStrFromElem(pElem);
            }

            // en fonction du type : pas le meme schema xml
            if(getFeatureInfoType.compare("PYRAMID") == 0){
                // Donnee elle-meme
            }else if(getFeatureInfoType.compare("EXTERNALWMS") == 0){
                // WMS
                hDoc=hRoot.FirstChild("getFeatureInfoUrl");
                pElem=hDoc.FirstChild("getFeatureInfoBaseURL").Element();
                if ( pElem && pElem->GetText()) {
                    getFeatureInfoBaseURL = DocumentXML::getTextStrFromElem(pElem);
                    std::string a = getFeatureInfoBaseURL.substr(getFeatureInfoBaseURL.length()-1, 1);
                    if ( a.compare("?") != 0 ) {
                       getFeatureInfoBaseURL = getFeatureInfoBaseURL + "?";
                    }
                }
                pElem=hDoc.FirstChild("layers").Element();
                if ( pElem && pElem->GetText()) {
                    GFILayers = DocumentXML::getTextStrFromElem(pElem);
                }
                pElem=hDoc.FirstChild("queryLayers").Element();
                if ( pElem && pElem->GetText()) {
                    GFIQueryLayers = DocumentXML::getTextStrFromElem(pElem);
                }
                pElem=hDoc.FirstChild("version").Element();
                if ( pElem && pElem->GetText()) {
                    GFIVersion = DocumentXML::getTextStrFromElem(pElem);
                }
                pElem=hDoc.FirstChild("service").Element();
                if ( pElem && pElem->GetText()) {
                    GFIService = DocumentXML::getTextStrFromElem(pElem);
                }
                pElem=hDoc.FirstChild("forceEPSG").Element();
                if ( pElem && pElem->GetText()=="false") {
                    GFIForceEPSG = false;
                }
            }else if(getFeatureInfoType.compare("SQL") == 0){
                    // SQL
            }else{
                BOOST_LOG_TRIVIAL(error) <<  filePath << _ ( "La source du GetFeatureInfo n'est pas autorisée." ) ;
                return;
            }
        }

        std::string inspireStyleName = DEFAULT_STYLE_INSPIRE;
        for ( pElem=hRoot.FirstChild ( "style" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "style" ) ) {
            if ( !pElem || ! ( pElem->GetText() ) ) {
                BOOST_LOG_TRIVIAL(error) <<  _ ( "Pas de style => style = " ) << ( inspire?DEFAULT_STYLE_INSPIRE:DEFAULT_STYLE ) ;
                styleName = ( inspire?DEFAULT_STYLE_INSPIRE:DEFAULT_STYLE );
            } else {
                styleName = DocumentXML::getTextStrFromElem(pElem);
            }

            Style* sty = serverXML->getStyle(styleName);
            if ( sty == NULL ) {
                BOOST_LOG_TRIVIAL(error) <<  _ ( "Style " ) << styleName << _ ( " non defini" ) ;
                continue;
            }

        if ( sty->getIdentifier().compare ( DEFAULT_STYLE_INSPIRE_ID ) == 0 ) {
                inspireStyleName = styleName;
            }
            styles.push_back ( sty );
            if ( inspire && ( styleName == inspireStyleName ) ) {
                styles.pop_back();
            }
        }

        if ( inspire ) {

            Style* sty = serverXML->getStyle(inspireStyleName);
            if ( sty == NULL ) {
                BOOST_LOG_TRIVIAL(error) <<  _ ( "Style " ) << inspireStyleName << _ ( "non defini" ) ;
                return;
            }
            styles.insert ( styles.begin(), sty );

        }

        if ( styles.size() ==0 ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Pas de Style defini, Layer non valide" ) ;
            return;
        }

        pElem = hRoot.FirstChild ( "minRes" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            minRes=0.;
        } else if ( !sscanf ( pElem->GetText(),"%lf",&minRes ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "La resolution min est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" ;
            return;
        }

        pElem = hRoot.FirstChild ( "maxRes" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            maxRes=0.;
        } else if ( !sscanf ( pElem->GetText(),"%lf",&maxRes ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "La resolution max est inexploitable:[" ) << DocumentXML::getTextStrFromElem(pElem) << "]" ;
            return;
        }


        if ( serverXML->getReprojectionCapability() == true ) {
            for ( pElem=hRoot.FirstChild ( "WMSCRSList" ).FirstChild ( "WMSCRS" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "WMSCRS" ) ) {
                if ( ! ( pElem->GetText() ) ) continue;
                std::string str_crs ( DocumentXML::getTextStrFromElem(pElem) );
                // On verifie que la CRS figure dans la liste des CRS de proj4 (sinon, le serveur n est pas capable de la gerer)
                CRS crs ( str_crs );
                bool crsOk=true;
                if ( !crs.isProj4Compatible() ) {
                    BOOST_LOG_TRIVIAL(warning) <<  _ ( "Le CRS " ) <<str_crs<<_ ( " n est pas reconnu par Proj4 et n est donc par ajoute aux CRS de la couche" ) ;
                    crsOk = false;
                } else {
                    //Test if already define in Global CRS

                    for ( unsigned int k=0; k<servicesXML->getGlobalCRSList()->size(); k++ ) {
                        if ( crs.cmpRequestCode ( servicesXML->getGlobalCRSList()->at ( k ).getRequestCode() ) ) {
                            crsOk = false;
                            BOOST_LOG_TRIVIAL(info) <<  _ ( "         CRS " ) <<str_crs << _ ( " already present in global CRS list" ) ;
                            break;
                        }
                    }

                    // Test if the current layer bounding box is compatible with the current CRS
                    if ( inspire && !crs.validateBBoxGeographic ( geographicBoundingBox.minx,geographicBoundingBox.miny,geographicBoundingBox.maxx,geographicBoundingBox.maxy ) ) {
                        BoundingBox<double> cropBBox = crs.cropBBoxGeographic ( geographicBoundingBox.minx,geographicBoundingBox.miny,geographicBoundingBox.maxx,geographicBoundingBox.maxy );
                        // Test if the remaining bbox contain useful data
                        if ( cropBBox.xmax - cropBBox.xmin <= 0 || cropBBox.ymax - cropBBox.ymin <= 0 ) {
                            BOOST_LOG_TRIVIAL(warning) <<  _ ( "         Le CRS " ) <<str_crs<<_ ( " n est pas compatible avec l'emprise de la couche" ) ;
                            crsOk = false;
                        }
                    }
                    
                    if ( crsOk ){
                        bool allowedCRS = true;
                        std::vector<CRS> tmpEquilist;
                        if (servicesXML->getAddEqualsCRS()){
                            tmpEquilist = ConfLoader::getEqualsCRS(servicesXML->getListOfEqualsCRS(), str_crs );
                        }
                        if ( servicesXML->getDoWeRestrictCRSList() ){
                            allowedCRS = ConfLoader::isCRSAllowed(servicesXML->getRestrictedCRSList(), str_crs, tmpEquilist);
                        }
                        if (!allowedCRS){
                            BOOST_LOG_TRIVIAL(warning) <<  _ ( "         Forbiden CRS " ) << str_crs  ;
                            crsOk = false;
                        }
                    }
                    
                    if ( crsOk ) {
                        bool found = false;
                        for ( int i = 0; i<WMSCRSList.size() ; i++ ){
                            if ( WMSCRSList.at( i ) == crs ){
                                found = true;
                                break;
                            }
                        }
                        if (!found){
                            BOOST_LOG_TRIVIAL(info) <<  _ ( "         Adding CRS " ) <<str_crs ;
                            WMSCRSList.push_back ( crs );
                        } else {
                            BOOST_LOG_TRIVIAL(warning) <<  _ ( "         Already present CRS " ) << str_crs  ;
                        }
                        std::vector<CRS> tmpEquilist = ConfLoader::getEqualsCRS(servicesXML->getListOfEqualsCRS() , str_crs );
                        for (unsigned int l = 0; l< tmpEquilist.size();l++){
                            found = false;
                            for ( int i = 0; i<WMSCRSList.size() ; i++ ){
                                if ( WMSCRSList.at( i ) == tmpEquilist.at( l ) ){
                                    found = true;
                                    break;
                                }
                            }
                            if (!found){
                                WMSCRSList.push_back( tmpEquilist.at( l ) );
                                BOOST_LOG_TRIVIAL(info) <<  _ ( "         Adding Equivalent CRS " ) << tmpEquilist.at( l ).getRequestCode() ;
                            } else {
                                BOOST_LOG_TRIVIAL(warning) <<  _ ( "         Already present CRS " ) << tmpEquilist.at( l ).getRequestCode()  ;
                            }
                        }
                    }
                }
            }
        }

        if ( WMSCRSList.size() ==0 ) {
            BOOST_LOG_TRIVIAL(info) <<  filePath <<_ ( ": Aucun CRS specifique autorise pour la couche" ) ;
        }
        pElem=hRoot.FirstChild ( "resampling" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            BOOST_LOG_TRIVIAL(error) <<  _ ( "Pas de resampling => resampling = " ) << DEFAULT_RESAMPLING ;
            resamplingStr = DEFAULT_RESAMPLING;
        } else {
            resamplingStr = DocumentXML::getTextStrFromElem(pElem);
        }
        resampling = Interpolation::fromString ( resamplingStr );
    }

    ok = true;
}


LayerXML::~LayerXML(){ }

std::string LayerXML::getId() { return id; }

bool LayerXML::isOk() { return ok; }
