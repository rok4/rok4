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

#include "LevelXML.h"


LevelXML::LevelXML( TiXmlElement* levelElement, std::string fileName, std::string parentDir, ServerXML* serverXML, TileMatrixSet* tms, bool times)
{
    ok = false;

    minTileRow = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    maxTileRow = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    minTileCol = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    maxTileCol = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    noDataFilePath="";
    prefix = "";

    context = NULL;

    onDemand = false;
    onFly = false;

    //----

    //----TM
    TiXmlHandle hLvl ( levelElement );
    TiXmlElement* pElemLvl = hLvl.FirstChild ( "tileMatrix" ).Element();
    if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
        LOGGER_ERROR ( fileName <<_ ( " level " ) <<_ ( "id" ) <<_ ( " sans tileMatrix!!" ) );
        return;
    }
    std::string tmName ( pElemLvl->GetText() );
    std::string id ( tmName );
    //on va vérifier que le level qu'on veut charger n'a pas déjà été chargé
    if (levels.size() != 0) {
        for (std::map<std::string, Level *>::iterator lv = levels.begin(); lv != levels.end(); lv++) {
            if (lv->second->getId() == id) {
                LOGGER_ERROR ( _ ( "Level: " ) << id << _ ( " has already been loaded" ) );
                return ;
            }
        }
    }

    tm = tms->getTm(tmName);
    if ( tm == NULL ) {
        LOGGER_ERROR ( fileName <<_ ( " Le level " ) << id <<_ ( " ref. Le TM [" ) << tmName << _ ( "] qui n'appartient pas au TMS [" ) << tmsName << "]" );
        return;
    }

    TiXmlElement* pElemLvl = hLvl.FirstChild ( "onFly" ).Element();
    if ( pElemLvl && pElemLvl->GetText() ) {
        onFly = true;
    }

    pElemLvl = hLvl.FirstChild ( "onDemand" ).Element();
    if ( pElemLvl && pElemLvl->GetText() ) {
        onDemand = true;
    }

    pElemLvl = hLvl.FirstChild ( "baseDir" ).Element();

    if ( pElemLvl && pElemLvl->GetText()) {

        baseDir = pElemLvl->GetText() ;
        //Relative Path
        if ( baseDir.compare ( 0,2,"./" ) == 0 ) {
            baseDir.replace ( 0,1,parentDir );
        } else if ( baseDir.compare ( 0,1,"/" ) != 0 ) {
            baseDir.insert ( 0,"/" );
            baseDir.insert ( 0, parentDir );
        }

        pElemLvl = hLvl.FirstChild ( "pathDepth" ).Element();
        if ( ! pElemLvl || ! ( pElemLvl->GetText() ) ) {
            LOGGER_ERROR ( fileName << " Level " << id << ": Pas de pathDepth alors que baseDir!!" );
            return;
        }

        if ( !sscanf ( pElemLvl->GetText(),"%d",&pathDepth ) ) {
            LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": pathDepth=[" ) << pElemLvl->GetText() <<_ ( "] is not an integer." ) );
            return;
        }

        context = new FileContext("");
        if (! context->connection() ) {
            LOGGER_ERROR("Impossible de se connecter aux donnees.");
            return;
        }
    }

    if ( onFly ) {
        if (! times) {
            LOGGER_ERROR ( "OnFly pyramid cannot use a onFly/onDemand pyramid as source" );
            return;
        }

        if ( context == NULL ) {
            LOGGER_ERROR ( "OnFly pyramid have to own a basedir and pathdepth to store images" );
            return;
        }

        TiXmlElement* pElemS=hLvl.FirstChild ( "sources" ).Element();
        if (pElemS) {

        } else {
            LOGGER_ERROR ( "OnFly pyramid need sources" );
            return;
        }

        TiXmlHandle hbdP ( pElemS );
        TiXmlElement* pElemSP=hbdP.FirstChild().ToElement();

        for (pElemSP; pElemSP; pElemSP = pElemSP->NextSiblingElement()) {
            // On lit chaque source, qui est soit une pyramide, soit un web service

            if (pElemSP->ValueStr() == "basedPyramid") {

                PyramidLevelSource* pls = ConfLoader::buildPyramidLevelSource(pElemSP, serverXML, tmsList,false,stylesList,parentDir, proxy);

                if (pls == NULL) {
                    LOGGER_ERROR ("Impossible de charger une basedPyramid (un niveau) indique");
                    ConfLoader::cleanParsePyramid(specificSources,sSources,levels);
                    return;
                }

                sSources.push_back( pls ) ;
            }

            if (pElemSP->ValueStr() == "webService") {

                WebService* ws = ConfLoader::parseWebService(pElemSP,tms->getCrs(),format, proxy);
                if (ws == NULL) {
                    LOGGER_ERROR("Impossible de charger le WebService indique");
                    return;
                }
                
                sSources.push_back(ws);
            }

        }

    }

    else if (onDemand) {
        if (! times) {
            LOGGER_ERROR ( "OnDemand pyramid cannot use a onFly/onDemand pyramid as source" );
            return;
        }
        // quasi pareil context en moins
    }

    else if (context == NULL) {
        // Je suis sur une pyramide normale, et je n'ai a priori pas de base dir donc de stockage fichier
        // Il me faut un stockage objet

        pElemLvl = hLvl.FirstChild ( "cephContext" ).Element();
        if ( pElemLvl ) {

            std::string poolName;

            TiXmlElement* pElemCephContext;

            pElemCephContext = pElemLvl.FirstChild ( "poolName" ).Element();

            if ( ! pElemCephContext  || ! ( pElemCephContext->GetText() ) ) {
                LOGGER_ERROR ("L'utilisation d'un cephContext necessite de preciser un poolName" );
                return;
            }

            poolName = pElemCephContext->GetText();

            if (serverXML->getCephContextBook() != NULL) {
                context = serverXML->getCephContextBook()->addContext(poolName);
            } else {
                LOGGER_ERROR ( "L'utilisation d'un cephContext necessite de preciser les informations de connexions dans le server.conf");
                return;
            }

            pElemLvl = hLvl.FirstChild ( "imagePrefix" ).Element();
            if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
                LOGGER_ERROR ( "imagePrefix absent pour le level " << id << " qui est stocke sur du Ceph");
                return;
            }
            prefix = pElemLvl->GetText() ;
        }

        if (context == NULL) {
            // Je n'ai toujours pas de stockage, il me faut du swift

            pElemLvl = hLvl.FirstChild ( "swiftContext" ).Element();
            if ( pElemLvl ) {

            std::string container;

            TiXmlElement* pElemSwiftContext;

            pElemSwiftContext = pElemLvl.FirstChild ( "container" ).Element();
            if ( !pElemSwiftContext  || ! ( pElemSwiftContext->GetText() ) ) {
                LOGGER_ERROR ("L'utilisation d'un swiftContext necessite de preciser un container" );
                return;
            } else {
                container = pElemSwiftContext->GetText();
            }

            if (serverXML->getSwiftContextBook() != NULL) {
                context = serverXML->getSwiftContextBook()->addContext(container);
            } else {
                LOGGER_ERROR ( "L'utilisation d'un cephContext necessite de preciser les informations de connexions dans le server.conf");
                return;
            }

            pElemLvl = hLvl.FirstChild ( "imagePrefix" ).Element();
            if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
                LOGGER_ERROR ( "imagePrefix absent pour le level " << id << " qui est stocke sur du Swift");
                return;
            }
            prefix = pElemLvl->GetText() ;

        } else {
            LOGGER_ERROR("Level " << id << " sans indication de stockage et pas à la demande. Precisez un baseDir ou un cephContext ou un swiftContext");
            return;
        }

    }
    //----

    //----TILEPERWIDTH

    pElemLvl = hLvl.FirstChild ( "tilesPerWidth" ).Element();
    if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": Pas de tilesPerWidth !!" ) );
        return;
    }
    if ( !sscanf ( pElemLvl->GetText(),"%d",&tilesPerWidth ) ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": tilesPerWidth=[" ) << pElemLvl->GetText() <<_ ( "] is not an integer." ) );
        return;
    }
    //----

    //----TILEPERHEIGHT
    pElemLvl = hLvl.FirstChild ( "tilesPerHeight" ).Element();
    if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": Pas de tilesPerHeight !!" ) );
        return;
    }
    if ( !sscanf ( pElemLvl->GetText(),"%d",&tilesPerHeight ) ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": tilesPerHeight=[" ) << pElemLvl->GetText() <<_ ( "] is not an integer." ) );
        return;
    }

    //----TMSLIMITS
    TiXmlElement *pElemLvlTMS =hLvl.FirstChild ( "TMSLimits" ).Element();
    if ( ! pElemLvlTMS ) {
        LOGGER_ERROR ( fileName << " Level " << id << ": TMSLimits have to be present." );
        return;
    }

    TiXmlHandle hTMSL ( pElemLvlTMS );
    TiXmlElement* pElemTMSL = hTMSL.FirstChild ( "minTileRow" ).Element();
    long int intBuff = -1;
    if ( !pElemTMSL ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": no minTileRow in TMSLimits element !!" ) );
        return;
    }
    if ( !pElemTMSL->GetText() ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": minTileRow is empty !!" ) );
        return;
    }
    if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": minTileRow=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
        return;
    }
    minTileRow = intBuff;
    intBuff = -1;
    pElemTMSL = hTMSL.FirstChild ( "maxTileRow" ).Element();
    if ( !pElemTMSL ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": no maxTileRow in TMSLimits element !!" ) );
        return;
    }
    if ( !pElemTMSL->GetText() ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": maxTileRow is empty !!" ) );
        return;
    }
    if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": maxTileRow=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
        return;
    }
    maxTileRow = intBuff;
    intBuff = -1;
    pElemTMSL = hTMSL.FirstChild ( "minTileCol" ).Element();
    if ( !pElemTMSL ) {
        LOGGER_ERROR ( _ ( " Level " ) << id << _ ( ": no minTileCol in TMSLimits element !!" ) );
        return;
    }
    if ( !pElemTMSL->GetText() ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": minTileCol is empty !!" ) );
        return;
    }

    if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": minTileCol=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
        return;
    }
    minTileCol = intBuff;
    intBuff = -1;
    pElemTMSL = hTMSL.FirstChild ( "maxTileCol" ).Element();
    if ( !pElemTMSL ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": no maxTileCol in TMSLimits element !!" ) );
        return;
    }
    if ( !pElemTMSL->GetText() ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": maxTileCol is empty !!" ) );
        return;
    }
    if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
        LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": maxTileCol=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
        return;
    }
    maxTileCol = intBuff;

    if ( minTileCol > tm->getMatrixW() || minTileCol < 0 ) minTileCol = 0;
    if ( minTileRow > tm->getMatrixH() || minTileRow < 0 ) minTileRow = 0;
    if ( maxTileCol > tm->getMatrixW() || maxTileCol < 0 ) maxTileCol = tm->getMatrixW();
    if ( maxTileRow > tm->getMatrixH() || maxTileRow < 0 ) maxTileRow = tm->getMatrixH();

    //----

    //----NODATA
    // Must exist for normal pyramid but could possibly not exist for onDemand and onFly Pyramid
    // BUT the path must be written in conf file in these cases
    
    if (! onDemand) {
        TiXmlElement* pElemNoData=hLvl.FirstChild ( "nodata" ).Element();
        if ( ! pElemNoData ) {
            LOGGER_ERROR ( "Level " << id << " : nodata have to be present if onFly or regular pyramid");
            return;
        }

        if (onFly) {
            TiXmlElement* pElemNoDataPath = pElemNoData.FirstChild ( "filePath" ).Element();
            if (! pElemNoDataPath || ! pElemNoDataPath->GetText()) {
                LOGGER_ERROR ( "Level " << id << " : nodata.filePath have to be present if onFly pyramid");
                return;
            }
            noDataFilePath = pElemNoDataPath->GetText();
            if ( noDataFilePath.compare ( 0,2,"./" ) ==0 ) {
                noDataFilePath.replace ( 0,1,parentDir );
            } else if ( noDataFilePath.compare ( 0,1,"/" ) !=0 ) {
                noDataFilePath.insert ( 0,"/" );
                noDataFilePath.insert ( 0, parentDir );
            }
        }
        else if (context->getType() == FILECONTEXT) {
            TiXmlElement* pElemNoDataPath = pElemNoData.FirstChild ( "filePath" ).Element();
            if (! pElemNoDataPath || ! pElemNoDataPath->GetText()) {
                LOGGER_ERROR ( "Level " << id << " : nodata.filePath have to be present if regular FileSystem pyramid");
                return;
            }
            noDataFilePath = pElemNoDataPath->GetText();
            if ( noDataFilePath.compare ( 0,2,"./" ) == 0 ) {
                noDataFilePath.replace ( 0,1,parentDir );
            } else if ( noDataFilePath.compare ( 0,1,"/" ) != 0 ) {
                noDataFilePath.insert ( 0,"/" );
                noDataFilePath.insert ( 0, parentDir );
            }

            if (! context->exists(noDataFilePath)) {
                LOGGER_ERROR(fileName <<_ ( " Level " ) << id <<_ ( " specifiant une tuile NoData impossible a ouvrir" ));
                return;
            }
        }
        else {
            TiXmlElement* pElemNoDataName = pElemNoData.FirstChild ( "objectName" ).Element();
            if (! pElemNoDataName || ! pElemNoDataName->GetText()) {
                LOGGER_ERROR ( "Level " << id << " : nodata.objectName have to be present if regular ObjectStorage pyramid");
                return;
            }
            noDataObjectName = pElemNoDataName->GetText();

            if (! context->exists(noDataObjectName)) {
                LOGGER_ERROR(fileName <<_ ( " Level " ) << id <<_ ( " specifiant une tuile NoData (objet) impossible a ouvrir" ));
                return;
            }
        }

    }

    ok = true;
}

