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

#include "LevelXML.h"

#include "FileContext.h"

#if BUILD_OBJECT
#include "CephPoolContext.h"
#include "S3Context.h"
#include "SwiftContext.h"
#endif

LevelXML::LevelXML( TiXmlElement* levelElement, std::string path, ServerXML* serverXML, ServicesXML* servicesXML, PyramidXML* pyr, bool times) : DocumentXML(path)
{
    ok = false;

    minTileRow = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    maxTileRow = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    minTileCol = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    maxTileCol = -1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
    racine = "";

    context = NULL;

    onDemand = false;
    onFly = false;
    bool onDir = false;

    //----

    //----TM
    TiXmlHandle hLvl ( levelElement );
    TiXmlElement* pElem = hLvl.FirstChild ( "tileMatrix" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        BOOST_LOG_TRIVIAL(error) <<  filePath << " level " << "id" << " sans tileMatrix!!" ;
        return;
    }
    id  = std::string( pElem->GetText() );

    TileMatrixSet* tms = pyr->getTMS();
    tm = tms->getTm(id);
    if ( tm == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  filePath << " Le level " << id << " ref. Le TM [" << id <<  "] qui n'appartient pas au TMS [" << tms->getId() << "]" ;
        return;
    }

    /******************* STOCKAGE FICHIER *********************/
    pElem = hLvl.FirstChild ( "baseDir" ).Element();

    if ( pElem && pElem->GetText()) {

        onDir = true;
        racine = pElem->GetText() ;
        //Relative Path
        if ( racine.compare ( 0,2,"./" ) == 0 ) {
            racine.replace ( 0,1,parentDir );
        } else if ( racine.compare ( 0,1,"/" ) != 0 ) {
            racine.insert ( 0,"/" );
            racine.insert ( 0, parentDir );
        }

        pElem = hLvl.FirstChild ( "pathDepth" ).Element();
        if ( ! pElem || ! ( pElem->GetText() ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": Pas de pathDepth alors que baseDir!!" ;
            return;
        }

        if ( !sscanf ( pElem->GetText(),"%d",&pathDepth ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": pathDepth=[" << pElem->GetText() << "] is not an integer." ;
            return;
        }

        context = serverXML->getContextBook()->addContext(ContextType::FILECONTEXT,"");
        if (context == NULL) {
            return;
        }
    }


    /******************* PYRAMIDE RASTER OD *********************/
    pElem = hLvl.FirstChild ( "onFly" ).Element();
    if ( pElem && pElem->GetText() ) {
        onFly = true;
    }

    pElem = hLvl.FirstChild ( "onDemand" ).Element();
    if ( pElem && pElem->GetText() ) {
        onDemand = true;
    }

    if ( onFly ) {
        if (! times) {
            BOOST_LOG_TRIVIAL(error) <<  "OnFly pyramid cannot use a onFly/onDemand level as source" ;
            return;
        }

        if ( ! onDir ) {
            BOOST_LOG_TRIVIAL(error) <<  "OnFly level have to own a basedir and pathdepth to store images" ;
            return;
        }

        TiXmlElement* pElemS=hLvl.FirstChild ( "sources" ).Element();
        if (pElemS == NULL) {
            BOOST_LOG_TRIVIAL(error) <<  "OnFly level need sources" ;
            return;
        }

        TiXmlHandle hbdP ( pElemS );
        TiXmlElement* pElemSP=hbdP.FirstChild().ToElement();

        for (pElemSP; pElemSP; pElemSP = pElemSP->NextSiblingElement()) {
            // On lit chaque source, qui est soit une pyramide, soit un web service

            if (pElemSP->ValueStr() == "basedPyramid") {

                Pyramid* sourcePyr = ConfLoader::buildBasedPyramid(pElemSP, serverXML, servicesXML, id, tms, parentDir);
                if (sourcePyr == NULL) {
                    BOOST_LOG_TRIVIAL(error) << "Impossible de charger une basedPyramid (un niveau) indique";
                    return;
                }

                sSources.push_back( sourcePyr ) ;
            }

            if (pElemSP->ValueStr() == "webService") {

                WebService* ws = ConfLoader::parseWebService(pElemSP,tms->getCrs(),pyr->getFormat(), servicesXML);
                if (ws == NULL) {
                    BOOST_LOG_TRIVIAL(error) << "Impossible de charger le WebService indique";
                    return;
                }

                sSources.push_back(ws);
            }

        }

    }

    else if (onDemand) {
        if (! times) {
            BOOST_LOG_TRIVIAL(error) <<  "OnDemand level cannot use a onFly/onDemand pyramid as source" ;
            return;
        }
        TiXmlElement* pElemS=hLvl.FirstChild ( "sources" ).Element();
        if (pElemS == NULL) {
            BOOST_LOG_TRIVIAL(error) <<  "OnDemand level need sources" ;
            return;
        }

        TiXmlHandle hbdP ( pElemS );
        TiXmlElement* pElemSP=hbdP.FirstChild().ToElement();

        for (pElemSP; pElemSP; pElemSP = pElemSP->NextSiblingElement()) {
            // On lit chaque source, qui est soit une pyramide, soit un web service

            if (pElemSP->ValueStr() == "basedPyramid") {

                Pyramid* sourcePyr = ConfLoader::buildBasedPyramid(pElemSP, serverXML, servicesXML, id, tms, parentDir);
                if (sourcePyr == NULL) {
                    BOOST_LOG_TRIVIAL(error) << "Impossible de charger une basedPyramid (un niveau) indique";
                    return;
                }

                sSources.push_back( sourcePyr ) ;
            }

            if (pElemSP->ValueStr() == "webService") {

                WebService* ws = ConfLoader::parseWebService(pElemSP,tms->getCrs(),pyr->getFormat(), servicesXML);
                if (ws == NULL) {
                    BOOST_LOG_TRIVIAL(error) << "Impossible de charger le WebService indique";
                    return;
                }

                sSources.push_back(ws);
            }

        }
    }


#if ! BUILD_OBJECT
    else if (! onDir) {
        BOOST_LOG_TRIVIAL(error) << "Level " << id << " sans indication de stockage et pas à la demande. Precisez un baseDir";
        return;
    }
#else

    /******************* STOCKAGE OBJET *********************/
    else if (! onDir) {
        // Je suis sur une pyramide normale, et je n'ai a priori pas de base dir donc de stockage fichier
        // Il me faut un stockage objet

        pElem = hLvl.FirstChild ( "cephContext" ).Element();
        if ( pElem ) {
            std::string poolName;

            TiXmlElement* pElemCephContext = pElem->FirstChildElement("poolName");

            if ( ! pElemCephContext  || ! ( pElemCephContext->GetText() ) ) {
                BOOST_LOG_TRIVIAL(error) << "L'utilisation d'un cephContext necessite de preciser un poolName" ;
                return;
            }

            poolName = pElemCephContext->GetText();

            pElem = hLvl.FirstChild ( "imagePrefix" ).Element();
            if ( !pElem || ! ( pElem->GetText() ) ) {
                BOOST_LOG_TRIVIAL(error) <<  "imagePrefix absent pour le level " << id << " qui est stocke sur du Ceph";
                return;
            }
            racine = pElem->GetText() ;

            context = serverXML->getContextBook()->addContext(ContextType::CEPHCONTEXT,poolName);
            if (context == NULL) {
                return;
            }
        }

        if (context == NULL) {
            // Je n'ai toujours pas de stockage, il me faut du s3

            pElem = hLvl.FirstChild ( "s3Context" ).Element();
            if ( pElem ) {

                std::string bucket;

                TiXmlElement* pElemS3Context = pElem->FirstChildElement ( "bucketName" );
                if ( !pElemS3Context  || ! ( pElemS3Context->GetText() ) ) {
                    BOOST_LOG_TRIVIAL(error) << "L'utilisation d'un s3Context necessite de preciser un bucket" ;
                    return;
                } else {
                    bucket = pElemS3Context->GetText();
                }

                pElem = hLvl.FirstChild ( "imagePrefix" ).Element();
                if ( !pElem || ! ( pElem->GetText() ) ) {
                    BOOST_LOG_TRIVIAL(error) <<  "imagePrefix absent pour le level " << id << " qui est stocke sur du S3";
                    return;
                }
                racine = pElem->GetText() ;

                context = serverXML->getContextBook()->addContext(ContextType::S3CONTEXT,bucket);
                if (context == NULL) {
                    return;
                }

            }

            if (context == NULL) {
                // Je n'ai toujours pas de stockage, il me faut du swift

                pElem = hLvl.FirstChild ( "swiftContext" ).Element();
                if ( pElem ) {

                    std::string container;

                    TiXmlElement* pElemSwiftContext = pElem->FirstChildElement ( "containerName" );
                    if ( !pElemSwiftContext  || ! ( pElemSwiftContext->GetText() ) ) {
                        BOOST_LOG_TRIVIAL(error) << "L'utilisation d'un swiftContext necessite de preciser un containerName" ;
                        return;
                    } else {
                        container = pElemSwiftContext->GetText();
                    }

                    pElem = hLvl.FirstChild ( "imagePrefix" ).Element();
                    if ( !pElem || ! ( pElem->GetText() ) ) {
                        BOOST_LOG_TRIVIAL(error) <<  "imagePrefix absent pour le level " << id << " qui est stocke sur du Swift";
                        return;
                    }
                    racine = pElem->GetText() ;

                    context = serverXML->getContextBook()->addContext(ContextType::SWIFTCONTEXT,container);
                    if (context == NULL) {
                        return;
                    }

                } else {
                    BOOST_LOG_TRIVIAL(error) << "Level " << id << " sans indication de stockage et pas à la demande. Precisez un baseDir ou un cephContext ou un swiftContext ou un s3Context";
                    return;
                }
            }

            if (context == NULL) {
                BOOST_LOG_TRIVIAL(error) << "Aucun contexte utilisable";
                return;
            }

        }
    }
#endif


    /******************* PYRAMIDE VECTEUR *********************/
    
    for ( pElem=hLvl.FirstChild ( "table" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "table" ) ) {

        TiXmlHandle hElem ( pElem );

        // Nom de l'attribut
        TiXmlElement *pElemTable = hElem.FirstChild ( "name" ).Element();
        if ( !pElemTable ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": table without name !!" ;
            return;
        }
        if ( !pElemTable->GetText() ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": table with empty name !!" ;
            return;
        }
        std::string tableName  = std::string( pElemTable->GetText() );

        // Type de géométrie
        pElemTable = hElem.FirstChild ( "geometry" ).Element();
        if ( !pElemTable ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": table without geometry !!" ;
            return;
        }
        if ( !pElemTable->GetText() ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": table with empty geometry !!" ;
            return;
        }
        std::string geometry  = std::string( pElemTable->GetText() );
        

        std::vector<Attribute> atts;
        for ( pElemTable=hElem.FirstChild ( "attribute" ).Element(); pElemTable; pElemTable=pElemTable->NextSiblingElement ( "attribute" ) ) {
            TiXmlHandle hElemTable ( pElemTable );

            // Nom de l'attribut
            TiXmlElement *pElemAtt = hElemTable.FirstChild ( "name" ).Element();
            if ( !pElemAtt ) {
                BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": attribute without name !!" ;
                return;
            }
            if ( !pElemAtt->GetText() ) {
                BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": attribute with empty name !!" ;
                return;
            }

            std::string attName  = std::string( pElemAtt->GetText() );

            // Type de l'attribut
            pElemAtt = hElemTable.FirstChild ( "type" ).Element();
            if ( !pElemAtt ) {
                BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": attribute without type !!" ;
                return;
            }
            if ( !pElemAtt->GetText() ) {
                BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": attribute with empty type !!" ;
                return;
            }
            std::string attType  = std::string( pElemAtt->GetText() );

            // Count distinct de l'attribut
            pElemAtt = hElemTable.FirstChild ( "count" ).Element();
            if ( !pElemAtt ) {
                BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": attribute without count !!" ;
                return;
            }
            if ( !pElemAtt->GetText() ) {
                BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": attribute with empty count !!" ;
                return;
            }
            std::string attCount  = std::string( pElemAtt->GetText() );

            // Valeurs de l'attribut
            pElemAtt = hElemTable.FirstChild ( "values" ).Element();
            std::string attValues = "";
            if ( pElemAtt && pElemAtt->GetText() ) {
                attValues = std::string( pElemAtt->GetText() );
            }

            // Min de l'attribut
            pElemAtt = hElemTable.FirstChild ( "min" ).Element();
            std::string attMin = "";
            if ( pElemAtt && pElemAtt->GetText() ) {
                attMin = std::string( pElemAtt->GetText() );
            }

            // Max de l'attribut
            pElemAtt = hElemTable.FirstChild ( "max" ).Element();
            std::string attMax = "";
            if ( pElemAtt && pElemAtt->GetText() ) {
                attMax = std::string( pElemAtt->GetText() );
            }

            atts.push_back(Attribute(attName, attType, attCount, attValues, attMin, attMax));

        }

        tables.push_back(Table(tableName, geometry, atts));
    }

    /******************* PARTIE COMMUNE *********************/

    //----TILEPERWIDTH

    if (! onDemand) {
        pElem = hLvl.FirstChild ( "tilesPerWidth" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": Pas de tilesPerWidth !!" ;
            return;
        }
        if ( !sscanf ( pElem->GetText(),"%d",&tilesPerWidth ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": tilesPerWidth=[" << pElem->GetText() << "] is not an integer." ;
            return;
        }
        //----

        //----TILEPERHEIGHT
        pElem = hLvl.FirstChild ( "tilesPerHeight" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": Pas de tilesPerHeight !!" ;
            return;
        }
        if ( !sscanf ( pElem->GetText(),"%d",&tilesPerHeight ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": tilesPerHeight=[" << pElem->GetText() << "] is not an integer." ;
            return;
        }

        if (tilesPerHeight == 0 || tilesPerWidth == 0) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": slab tiles size have to be non zero integers" ;
            return;
        }
    }

    //----TMSLIMITS
    TiXmlElement *pElemLvlTMS =hLvl.FirstChild ( "TMSLimits" ).Element();
    if ( ! pElemLvlTMS ) {
        if (!onFly && !onDemand) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": TMSLimits have to be present." ;
            return;
        }
        
        if (calculateTileLimits(pyr)) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": TMSLimits is not present and cannot be calculated" ;
            return;
        }


    } else {

        TiXmlHandle hTMSL ( pElemLvlTMS );
        TiXmlElement* pElemTMSL = hTMSL.FirstChild ( "minTileRow" ).Element();
        long int intBuff = -1;
        if ( !pElemTMSL ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": no minTileRow in TMSLimits element !!" ;
            return;
        }
        if ( !pElemTMSL->GetText() ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": minTileRow is empty !!" ;
            return;
        }
        if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": minTileRow=[" << pElemTMSL->GetText() << "] is not an integer." ;
            return;
        }
        minTileRow = intBuff;
        intBuff = -1;
        pElemTMSL = hTMSL.FirstChild ( "maxTileRow" ).Element();
        if ( !pElemTMSL ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": no maxTileRow in TMSLimits element !!" ;
            return;
        }
        if ( !pElemTMSL->GetText() ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": maxTileRow is empty !!" ;
            return;
        }
        if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": maxTileRow=[" << pElemTMSL->GetText() << "] is not an integer." ;
            return;
        }
        maxTileRow = intBuff;
        intBuff = -1;
        pElemTMSL = hTMSL.FirstChild ( "minTileCol" ).Element();
        if ( !pElemTMSL ) {
            BOOST_LOG_TRIVIAL(error) <<   " Level " << id <<  ": no minTileCol in TMSLimits element !!" ;
            return;
        }
        if ( !pElemTMSL->GetText() ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": minTileCol is empty !!" ;
            return;
        }

        if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": minTileCol=[" << pElemTMSL->GetText() << "] is not an integer." ;
            return;
        }
        minTileCol = intBuff;
        intBuff = -1;
        pElemTMSL = hTMSL.FirstChild ( "maxTileCol" ).Element();
        if ( !pElemTMSL ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": no maxTileCol in TMSLimits element !!" ;
            return;
        }
        if ( !pElemTMSL->GetText() ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id <<  ": maxTileCol is empty !!" ;
            return;
        }
        if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
            BOOST_LOG_TRIVIAL(error) <<  filePath << " Level " << id << ": maxTileCol=[" << pElemTMSL->GetText() << "] is not an integer." ;
            return;
        }
        maxTileCol = intBuff;

        if ( minTileCol > tm->getMatrixW() || minTileCol < 0 ) minTileCol = 0;
        if ( minTileRow > tm->getMatrixH() || minTileRow < 0 ) minTileRow = 0;
        if ( maxTileCol > tm->getMatrixW() || maxTileCol < 0 ) maxTileCol = tm->getMatrixW();
        if ( maxTileRow > tm->getMatrixH() || maxTileRow < 0 ) maxTileRow = tm->getMatrixH();
    }
    ok = true;
}

LevelXML::~LevelXML() {

    if (! ok) {
        // Ce niveau n'est pas valide, donc n'a pas été utilisé pour créer un objet Level. Il faut donc nettoyer tout ce qui a été créé.
        
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


int LevelXML::calculateTileLimits(PyramidXML* pyrxml) {
    //On met à jour les Min et Max Tiles une fois que l'on a trouvé un équivalent dans chaque basedPyramid
    // pour le level créé

    int curMinCol, curMaxCol, curMinRow, curMaxRow, bPMinCol, bPMaxCol, bPMinRow, bPMaxRow, minCol, minRow, maxCol, maxRow;
    double xo, yo, res, tileW, tileH, xmin, xmax, ymin, ymax;

    int time = 1;

    if (sSources.size() != 0) {

        for (int ip = 0; ip < sSources.size(); ip++) {

            if (sSources.at(ip)->getType() == PYRAMID) {
                Pyramid *pyr = reinterpret_cast<Pyramid*>(sSources.at(ip));
                Level *lv;

                //On récupére les Min et Max de la basedPyramid
                lv = pyr->getLevels().begin()->second;


                bPMinCol = lv->getMinTileCol();
                bPMaxCol = lv->getMaxTileCol();
                bPMinRow = lv->getMinTileRow();
                bPMaxRow = lv->getMaxTileRow();

                //On récupère d'autres informations sur le TM
                xo = lv->getTm()->getX0();
                yo = lv->getTm()->getY0();
                res = lv->getTm()->getRes();
                tileW = lv->getTm()->getTileW();
                tileH = lv->getTm()->getTileH();

                //On transforme en bbox
                xmin = bPMinCol * tileW * res + xo;
                ymax = yo - bPMinRow * tileH * res;
                xmax = xo + (bPMaxCol+1) * tileW * res;
                ymin = ymax - (bPMaxRow - bPMinRow + 1) * tileH * res;

                BoundingBox<double> MMbbox(xmin,ymin,xmax,ymax);


                //On reprojette la bbox
                if (MMbbox.reproject(pyr->getTms()->getCrs().getProj4Code(), pyrxml->getTMS()->getCrs().getProj4Code()) != 0) {
                    BOOST_LOG_TRIVIAL(error) << "Ne peut pas reprojeter la bbox de base";
                    return 1;
                }

                //On récupère les Min et Max de Pyr pour ce level dans la nouvelle projection
                xo = tm->getX0();
                yo = tm->getY0();
                res = tm->getRes();
                tileW = tm->getTileW();
                tileH = tm->getTileH();

                curMinRow = floor((yo - MMbbox.ymax) / (tileW * res));
                curMinCol = floor((MMbbox.xmin - xo) / (tileH * res));
                curMaxRow = floor((yo - MMbbox.ymin) / (tileW * res));
                curMaxCol = floor((MMbbox.xmax - xo) / (tileH * res));

                if (curMinRow < 0) {
                    curMinRow = 0;
                }
                if (curMinCol < 0) {
                    curMinCol = 0;
                }
                if (curMaxRow < 0) {
                    curMaxRow = 0;
                }
                if (curMaxCol < 0) {
                    curMaxCol = 0;
                }

                if (time == 1) {
                    minCol = curMinCol;
                    maxCol = curMaxCol;
                    minRow = curMinRow;
                    maxRow = curMaxRow;
                }

                //On teste pour récupèrer la plus grande zone à l'intérieur du TMS
                if (curMinCol >= minTileCol && curMinCol >= 0 && curMinCol <= curMaxCol && curMinCol <= maxTileCol) {
                    if (curMinCol <= minCol) {
                        minCol = curMinCol;
                    }
                }
                if (curMinRow >= minTileRow && curMinRow >= 0 && curMinRow <= curMaxRow && curMinRow <= maxTileRow) {
                    if (curMinRow <= minRow) {
                        minRow = curMinRow;
                    }
                }
                if (curMaxCol <= maxTileCol && curMaxCol >= 0 && curMaxCol >= curMinCol && curMaxCol >= minTileCol) {
                    if (curMaxCol >= maxCol) {
                        maxCol = curMaxCol;
                    }
                }
                if (curMaxRow <= maxTileRow && curMaxRow >= 0 && curMaxRow >= curMinRow && curMaxRow >= minTileRow) {
                    if (curMaxRow >= maxRow) {
                        maxRow = curMaxRow;
                    }
                }

            }

            if (sSources.at(ip)->getType() == WEBSERVICE) {

                WebMapService *wms = reinterpret_cast<WebMapService*>(sSources.at(ip));

                BoundingBox<double> MMbbox = wms->getBbox();

                //On récupère les Min et Max de Pyr pour ce level dans la nouvelle projection
                xo = tm->getX0();
                yo = tm->getY0();
                res = tm->getRes();
                tileW = tm->getTileW();
                tileH = tm->getTileH();

                curMinRow = floor((yo - MMbbox.ymax) / (tileW * res));
                curMinCol = floor((MMbbox.xmin - xo) / (tileH * res));
                curMaxRow = floor((yo - MMbbox.ymin) / (tileW * res));
                curMaxCol = floor((MMbbox.xmax - xo) / (tileH * res));

                if (curMinRow < 0) {
                    curMinRow = 0;
                }
                if (curMinCol < 0) {
                    curMinCol = 0;
                }
                if (curMaxRow < 0) {
                    curMaxRow = 0;
                }
                if (curMaxCol < 0) {
                    curMaxCol = 0;
                }

                if (time == 1) {
                    minCol = curMinCol;
                    maxCol = curMaxCol;
                    minRow = curMinRow;
                    maxRow = curMaxRow;
                }

                //On teste pour récupèrer la plus grande zone à l'intérieur du TMS
                if (curMinCol >= minTileCol && curMinCol >= 0 && curMinCol <= curMaxCol && curMinCol <= maxTileCol) {
                    if (curMinCol <= minCol) {
                        minCol = curMinCol;
                    }
                }
                if (curMinRow >= minTileRow && curMinRow >= 0 && curMinRow <= curMaxRow && curMinRow <= maxTileRow) {
                    if (curMinRow <= minRow) {
                        minRow = curMinRow;
                    }
                }
                if (curMaxCol <= maxTileCol && curMaxCol >= 0 && curMaxCol >= curMinCol && curMaxCol >= minTileCol) {
                    if (curMaxCol >= maxCol) {
                        maxCol = curMaxCol;
                    }
                }
                if (curMaxRow <= maxTileRow && curMaxRow >= 0 && curMaxRow >= curMinRow && curMaxRow >= minTileRow) {
                    if (curMaxRow >= maxRow) {
                        maxRow = curMaxRow;
                    }
                }

            }


            time++;
        }

    } else {
        return 1;
    }

    if (minCol > minTileCol ) {
        minTileCol = minCol;
    }
    if (minRow > minTileRow ) {
        minTileRow = minRow;
    }
    if (maxCol < maxTileCol ) {
        maxTileCol = maxCol;
    }
    if (maxRow < maxTileRow ) {
        maxTileRow = maxRow;
    }

    return 0;
}