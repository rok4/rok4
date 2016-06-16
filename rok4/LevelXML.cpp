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

#include "FileContext.h"
#include "CephPoolContext.h"
#include "SwiftContext.h"

LevelXML::LevelXML( TiXmlElement* levelElement, std::string path, ServerXML* serverXML, ServicesXML* servicesXML, PyramidXML* pyr, bool times) : DocumentXML(path)
{
    ok = false;

    minTileRow = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    maxTileRow = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    minTileCol = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    maxTileCol = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    prefix = "";

    context = NULL;

    onDemand = false;
    onFly = false;

    //----

    //----TM
    TiXmlHandle hLvl ( levelElement );
    TiXmlElement* pElem = hLvl.FirstChild ( "tileMatrix" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( filePath <<_ ( " level " ) <<_ ( "id" ) <<_ ( " sans tileMatrix!!" ) );
        return;
    }
    id  = std::string( pElem->GetText() );

    TileMatrixSet* tms = pyr->getTMS();
    tm = tms->getTm(id);
    if ( tm == NULL ) {
        LOGGER_ERROR ( filePath <<_ ( " Le level " ) << id <<_ ( " ref. Le TM [" ) << id << _ ( "] qui n'appartient pas au TMS [" ) << tms->getId() << "]" );
        return;
    }

    pElem = hLvl.FirstChild ( "onFly" ).Element();
    if ( pElem && pElem->GetText() ) {
        onFly = true;
    }

    pElem = hLvl.FirstChild ( "onDemand" ).Element();
    if ( pElem && pElem->GetText() ) {
        onDemand = true;
    }

    pElem = hLvl.FirstChild ( "baseDir" ).Element();

    if ( pElem && pElem->GetText()) {

        baseDir = pElem->GetText() ;
        //Relative Path
        if ( baseDir.compare ( 0,2,"./" ) == 0 ) {
            baseDir.replace ( 0,1,parentDir );
        } else if ( baseDir.compare ( 0,1,"/" ) != 0 ) {
            baseDir.insert ( 0,"/" );
            baseDir.insert ( 0, parentDir );
        }

        pElem = hLvl.FirstChild ( "pathDepth" ).Element();
        if ( ! pElem || ! ( pElem->GetText() ) ) {
            LOGGER_ERROR ( filePath << " Level " << id << ": Pas de pathDepth alors que baseDir!!" );
            return;
        }

        if ( !sscanf ( pElem->GetText(),"%d",&pathDepth ) ) {
            LOGGER_ERROR ( filePath <<_ ( " Level " ) << id <<_ ( ": pathDepth=[" ) << pElem->GetText() <<_ ( "] is not an integer." ) );
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
            LOGGER_ERROR ( "OnFly pyramid cannot use a onFly/onDemand level as source" );
            return;
        }

        if ( context == NULL ) {
            LOGGER_ERROR ( "OnFly level have to own a basedir and pathdepth to store images" );
            return;
        }

        TiXmlElement* pElemS=hLvl.FirstChild ( "sources" ).Element();
        if (pElemS) {

        } else {
            LOGGER_ERROR ( "OnFly level need sources" );
            return;
        }

        TiXmlHandle hbdP ( pElemS );
        TiXmlElement* pElemSP=hbdP.FirstChild().ToElement();

        for (pElemSP; pElemSP; pElemSP = pElemSP->NextSiblingElement()) {
            // On lit chaque source, qui est soit une pyramide, soit un web service

            if (pElemSP->ValueStr() == "basedPyramid") {

                Pyramid* sourcePyr = ConfLoader::buildBasedPyramid(pElemSP, serverXML, servicesXML, id, tms, parentDir);
                if (sourcePyr == NULL) {
                    LOGGER_ERROR ("Impossible de charger une basedPyramid (un niveau) indique");
                    return;
                }

                sSources.push_back( sourcePyr ) ;
            }

            if (pElemSP->ValueStr() == "webService") {

                WebService* ws = ConfLoader::parseWebService(pElemSP,tms->getCrs(),pyr->getFormat(), serverXML->getProxy());
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
            LOGGER_ERROR ( "OnDemand level cannot use a onFly/onDemand pyramid as source" );
            return;
        }
        TiXmlElement* pElemS=hLvl.FirstChild ( "sources" ).Element();
        if (pElemS) {

        } else {
            LOGGER_ERROR ( "OnDemand level need sources" );
            return;
        }

        TiXmlHandle hbdP ( pElemS );
        TiXmlElement* pElemSP=hbdP.FirstChild().ToElement();

        for (pElemSP; pElemSP; pElemSP = pElemSP->NextSiblingElement()) {
            // On lit chaque source, qui est soit une pyramide, soit un web service

            if (pElemSP->ValueStr() == "basedPyramid") {

                Pyramid* sourcePyr = ConfLoader::buildBasedPyramid(pElemSP, serverXML, servicesXML, id, tms, parentDir);
                if (sourcePyr == NULL) {
                    LOGGER_ERROR ("Impossible de charger une basedPyramid (un niveau) indique");
                    return;
                }

                sSources.push_back( sourcePyr ) ;
            }

            if (pElemSP->ValueStr() == "webService") {

                WebService* ws = ConfLoader::parseWebService(pElemSP,tms->getCrs(),pyr->getFormat(), serverXML->getProxy());
                if (ws == NULL) {
                    LOGGER_ERROR("Impossible de charger le WebService indique");
                    return;
                }

                sSources.push_back(ws);
            }

        }
    }

    else if (context == NULL) {
        // Je suis sur une pyramide normale, et je n'ai a priori pas de base dir donc de stockage fichier
        // Il me faut un stockage objet

        pElem = hLvl.FirstChild ( "cephContext" ).Element();
        if ( pElem ) {

            std::string poolName;

            TiXmlElement* pElemCephContext = pElem->FirstChildElement("poolName");

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

            pElem = hLvl.FirstChild ( "imagePrefix" ).Element();
            if ( !pElem || ! ( pElem->GetText() ) ) {
                LOGGER_ERROR ( "imagePrefix absent pour le level " << id << " qui est stocke sur du Ceph");
                return;
            }
            prefix = pElem->GetText() ;
        }

        if (context == NULL) {
            // Je n'ai toujours pas de stockage, il me faut du swift

            pElem = hLvl.FirstChild ( "swiftContext" ).Element();
            if ( pElem ) {

                std::string container;

                TiXmlElement* pElemSwiftContext = pElem->FirstChildElement ( "container" );
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

                pElem = hLvl.FirstChild ( "imagePrefix" ).Element();
                if ( !pElem || ! ( pElem->GetText() ) ) {
                    LOGGER_ERROR ( "imagePrefix absent pour le level " << id << " qui est stocke sur du Swift");
                    return;
                }
                prefix = pElem->GetText() ;

            } else {
                LOGGER_ERROR("Level " << id << " sans indication de stockage et pas à la demande. Precisez un baseDir ou un cephContext ou un swiftContext");
                return;
            }
        }

    }
    //----

    //----TILEPERWIDTH

    pElem = hLvl.FirstChild ( "tilesPerWidth" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id << _ ( ": Pas de tilesPerWidth !!" ) );
        return;
    }
    if ( !sscanf ( pElem->GetText(),"%d",&tilesPerWidth ) ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id <<_ ( ": tilesPerWidth=[" ) << pElem->GetText() <<_ ( "] is not an integer." ) );
        return;
    }
    //----

    //----TILEPERHEIGHT
    pElem = hLvl.FirstChild ( "tilesPerHeight" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id << _ ( ": Pas de tilesPerHeight !!" ) );
        return;
    }
    if ( !sscanf ( pElem->GetText(),"%d",&tilesPerHeight ) ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id <<_ ( ": tilesPerHeight=[" ) << pElem->GetText() <<_ ( "] is not an integer." ) );
        return;
    }

    //----TMSLIMITS
    TiXmlElement *pElemLvlTMS =hLvl.FirstChild ( "TMSLimits" ).Element();
    if ( ! pElemLvlTMS ) {
        LOGGER_ERROR ( filePath << " Level " << id << ": TMSLimits have to be present." );
        return;
    }

    TiXmlHandle hTMSL ( pElemLvlTMS );
    TiXmlElement* pElemTMSL = hTMSL.FirstChild ( "minTileRow" ).Element();
    long int intBuff = -1;
    if ( !pElemTMSL ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id << _ ( ": no minTileRow in TMSLimits element !!" ) );
        return;
    }
    if ( !pElemTMSL->GetText() ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id << _ ( ": minTileRow is empty !!" ) );
        return;
    }
    if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id <<_ ( ": minTileRow=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
        return;
    }
    minTileRow = intBuff;
    intBuff = -1;
    pElemTMSL = hTMSL.FirstChild ( "maxTileRow" ).Element();
    if ( !pElemTMSL ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id << _ ( ": no maxTileRow in TMSLimits element !!" ) );
        return;
    }
    if ( !pElemTMSL->GetText() ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id <<_ ( ": maxTileRow is empty !!" ) );
        return;
    }
    if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id <<_ ( ": maxTileRow=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
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
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id << _ ( ": minTileCol is empty !!" ) );
        return;
    }

    if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id <<_ ( ": minTileCol=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
        return;
    }
    minTileCol = intBuff;
    intBuff = -1;
    pElemTMSL = hTMSL.FirstChild ( "maxTileCol" ).Element();
    if ( !pElemTMSL ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id << _ ( ": no maxTileCol in TMSLimits element !!" ) );
        return;
    }
    if ( !pElemTMSL->GetText() ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id << _ ( ": maxTileCol is empty !!" ) );
        return;
    }
    if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
        LOGGER_ERROR ( filePath <<_ ( " Level " ) << id <<_ ( ": maxTileCol=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
        return;
    }
    maxTileCol = intBuff;

    if ( minTileCol > tm->getMatrixW() || minTileCol < 0 ) minTileCol = 0;
    if ( minTileRow > tm->getMatrixH() || minTileRow < 0 ) minTileRow = 0;
    if ( maxTileCol > tm->getMatrixW() || maxTileCol < 0 ) maxTileCol = tm->getMatrixW();
    if ( maxTileRow > tm->getMatrixH() || maxTileRow < 0 ) maxTileRow = tm->getMatrixH();

    ok = true;
}

LevelXML::~LevelXML() {

    if (! ok) {
        // Ce niveau n'est pas valide, donc n'a pas été utilisé pour créer un objet Level. Il faut donc nettoyer tout ce qui a été créé.
        if (context) {
            if (context->getType() == FILECONTEXT) delete context;
        }

        for ( int i = 0; i < sSources.size(); i++ ) {
            Source* pS = sSources.at(i);
            delete pS;
        }
    }

}

std::string LevelXML::getId() { return id; }
bool LevelXML::isOnDemand() { return onDemand; }
bool LevelXML::isOnFly() { return onFly; }

bool LevelXML::isOk() { return ok; }
