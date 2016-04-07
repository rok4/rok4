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


/**
 * \file ConfLoader.cpp
 * \~french
 * \brief Implémenation des fonctions de chargement de la configuration
 * \brief pendant l'initialisation du serveur
 * \~english
 * \brief Implements configuration loader functions
 * \brief during server initialization
 */


#include <dirent.h>
#include <cstdio>
#include "ConfLoader.h"
#include "Rok4Server.h"
#include "Pyramid.h"
#include "tinyxml.h"
#include "tinystr.h"
#include "config.h"
#include "Format.h"
#include "MetadataURL.h"
#include "LegendURL.h"
#include <malloc.h>
#include <stdlib.h>
#include <libgen.h>
#include "Interpolation.h"
#include "intl.h"
#include "config.h"
#include "Keyword.h"
#include <fcntl.h>
#include <cstddef>
#include <string>
#include "WebService.h"


// Load style
Style* ConfLoader::parseStyle ( TiXmlDocument* doc,std::string fileName,bool inspire ) {
    LOGGER_INFO ( _ ( "     Ajout du Style " ) << fileName );
    std::string id ="";
    std::vector<std::string> title;
    std::vector<std::string> abstract;
    std::vector<Keyword> keyWords;
    std::vector<LegendURL> legendURLs;
    std::map<double, Colour> colourMap;
    bool rgbContinuous = false;
    bool alphaContinuous = false;
    bool noAlpha = false;
    int angle =-1;
    float exaggeration=1;
    int center=0;
    int errorCode;

    /*TiXmlDocument doc(fileName.c_str());
    if (!doc.LoadFile()){
        LOGGER_ERROR("          Ne peut pas charger le fichier " << fileName);
        return NULL;
    }*/
    TiXmlHandle hDoc ( doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( fileName << _ ( "            Impossible de recuperer la racine." ) );
        return NULL;
    }
    if ( strcmp ( pElem->Value(),"style" ) ) {
        LOGGER_ERROR ( fileName << _ ( "            La racine n'est pas un style." ) );
        return NULL;
    }
    hRoot=TiXmlHandle ( pElem );

    pElem=hRoot.FirstChild ( "Identifier" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "Style " ) << fileName <<_ ( " pas de d'identifiant!!" ) );
        return NULL;
    }
    id = pElem->GetTextStr();

    for ( pElem=hRoot.FirstChild ( "Title" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "Title" ) ) {
        if ( ! ( pElem->GetText() ) )
            continue;
        std::string curtitle = pElem->GetTextStr();
        title.push_back ( curtitle );
    }
    if ( title.size() ==0 ) {
        LOGGER_ERROR ( _ ( "Aucun Title trouve dans le Style" ) << id <<_ ( " : il est invalide!!" ) );
        return NULL;
    }

    for ( pElem=hRoot.FirstChild ( "Abstract" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "Abstract" ) ) {
        if ( ! ( pElem->GetText() ) )
            continue;
        std::string curAbstract = pElem->GetTextStr();
        abstract.push_back ( curAbstract );
    }
    if ( abstract.size() ==0 && inspire ) {
        LOGGER_ERROR ( _ ( "Aucun Abstract trouve dans le Style" ) << id <<_ ( " : il est invalide au sens INSPIRE!!" ) );
        return NULL;
    }

    for ( pElem=hRoot.FirstChild ( "Keywords" ).FirstChild ( "Keyword" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "Keyword" ) ) {
        if ( ! ( pElem->GetText() ) )
            continue;
        std::map<std::string,std::string> attributes;
        TiXmlAttribute* attrib = pElem->FirstAttribute();
        while ( attrib ) {
            attributes.insert ( attribute ( attrib->NameTStr(),attrib->ValueStr() ) );
            attrib = attrib->Next();
        }
        keyWords.push_back ( Keyword ( pElem->GetTextStr(),attributes ) );
    }

    for ( pElem=hRoot.FirstChild ( "LegendURL" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "LegendURL" ) ) {
        std::string format;
        std::string href;
        int width=0;
        int height=0;
        double minScaleDenominator=0.0;
        double maxScaleDenominator=0.0;


        if ( pElem->QueryStringAttribute ( "format",&format ) != TIXML_SUCCESS ) {
            LOGGER_ERROR ( _ ( "Aucun format trouve dans le LegendURL du Style " ) << id <<_ ( " : il est invalide!!" ) );
            continue;
        }

        if ( pElem->QueryStringAttribute ( "xlink:href",&href ) != TIXML_SUCCESS ) {
            LOGGER_ERROR ( _ ( "Aucun href trouve dans le LegendURL du Style " ) << id <<_ ( " : il est invalide!!" ) );
            continue;
        }

        errorCode = pElem->QueryIntAttribute ( "width",&width );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "L'attribut width doit etre un entier dans le Style " ) << id <<_ ( " : il est invalide!!" ) );
            continue;
        }

        errorCode = pElem->QueryIntAttribute ( "height",&height );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "L'attribut height doit etre un entier dans le Style " ) << id <<_ ( " : il est invalide!!" ) );
            continue;
        }

        errorCode = pElem->QueryDoubleAttribute ( "minScaleDenominator",&minScaleDenominator );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "L'attribut minScaleDenominator doit etre un double dans le Style " ) << id <<_ ( " : il est invalide!!" ) );
            continue;
        }

        errorCode = pElem->QueryDoubleAttribute ( "maxScaleDenominator",&maxScaleDenominator );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "L'attribut maxScaleDenominator doit etre un double dans le Style " ) << id <<_ ( " : il est invalide!!" ) );
            continue;
        }

        legendURLs.push_back ( LegendURL ( format,href,width,height,minScaleDenominator,maxScaleDenominator ) );
    }

    if ( legendURLs.size() ==0 && inspire ) {
        LOGGER_ERROR ( _ ( "Aucun legendURL trouve dans le Style " ) << id <<_ ( " : il est invalide au sens INSPIRE!!" ) );
        return NULL;
    }



    pElem = hRoot.FirstChild ( "palette" ).Element();

    if ( pElem ) {
        double maxValue=0.0;

        std::string continuousStr;

        errorCode = pElem->QueryStringAttribute ( "rgbContinuous",&continuousStr );
        if ( errorCode != TIXML_SUCCESS ) {
            LOGGER_DEBUG ( _ ( "L'attribut rgbContinuous n'a pas ete trouve dans la palette du Style " ) << id <<_ ( " : Faux par defaut" ) );
        } else {
            if ( continuousStr.compare ( "true" ) ==0 )
                rgbContinuous=true;
        }

        errorCode = pElem->QueryStringAttribute ( "alphaContinuous",&continuousStr );
        if ( errorCode != TIXML_SUCCESS ) {
            LOGGER_DEBUG ( _ ( "L'attribut alphaContinuous n'a pas ete trouve dans la palette du Style " ) << id <<_ ( " : Faux par defaut" ) );
        } else {
            if ( continuousStr.compare ( "true" ) ==0 )
                alphaContinuous=true;
        }
        
        errorCode = pElem->QueryStringAttribute ( "noAlpha",&continuousStr );
        if ( errorCode != TIXML_SUCCESS ) {
            LOGGER_DEBUG ( _ ( "L'attribut noAlpha n'a pas ete trouve dans la palette du Style " ) << id <<_ ( " : Faux par defaut" ) );
        } else {
            if ( continuousStr.compare ( "true" ) ==0 )
                noAlpha=true;
        }

        errorCode = pElem->QueryDoubleAttribute ( "maxValue",&maxValue );
        if ( errorCode != TIXML_SUCCESS ) {
            LOGGER_ERROR ( _ ( "L'attribut maxValue n'a pas ete trouve dans la palette du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return NULL;
        } else {
            LOGGER_DEBUG ( _ ( "MaxValue " ) << maxValue );
            /*if ( maxValue <= 0 ) {
                LOGGER_ERROR ( "L'attribut maxValue est negatif ou nul " << id <<" : il est invalide!!" );
                return NULL;
            }*/

            double value=0;
            uint8_t r=0,g=0,b=0;
            int a=0;
            for ( pElem=hRoot.FirstChild ( "palette" ).FirstChild ( "colour" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "colour" ) ) {
                LOGGER_DEBUG ( _ ( "Value avant Couleur" ) << value );
                errorCode = pElem->QueryDoubleAttribute ( "value",&value );
                if ( errorCode == TIXML_WRONG_TYPE ) {
                    LOGGER_ERROR ( _ ( "Un attribut value invalide a ete trouve dans la palette du Style " ) << id <<_ ( " : il est invalide!!" ) );
                    continue;
                } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
                    value=0;
                }
                LOGGER_DEBUG ( _ ( "Couleur de la valeur " ) << value );
                TiXmlHandle cHdl ( pElem );
                TiXmlElement* colourElem;

                //Red
                colourElem = cHdl.FirstChild ( "red" ).Element();
                if ( ! ( colourElem ) || ! ( colourElem->GetText() ) ) {
                    LOGGER_ERROR ( _ ( "Un attribut colour invalide a ete trouve dans la palette du Style " ) << id <<_ ( " : il est invalide!!" ) );
                    continue;
                }
                r = atoi ( colourElem->GetText() );
                if ( r < 0 || r > 255 ) {
                    LOGGER_ERROR ( _ ( "Un attribut colour invalide a ete trouve dans la palette du Style " ) << id <<_ ( " : il est invalide!!" ) );
                    continue;
                }

                //Green
                colourElem = cHdl.FirstChild ( "green" ).Element();
                if ( ! ( colourElem ) || ! ( colourElem->GetText() ) ) {
                    LOGGER_ERROR ( _ ( "Un attribut colour invalide a ete trouve dans la palette du Style " ) << id <<_ ( " : il est invalide!!" ) );
                    continue;
                }

                g = atoi ( colourElem->GetText() );
                if ( g < 0 || g > 255 ) {
                    LOGGER_ERROR ( _ ( "Un attribut colour invalide a ete trouve dans la palette du Style " ) << id <<_ ( " : il est invalide!!" ) );
                    continue;
                }

                //Blue
                colourElem = cHdl.FirstChild ( "blue" ).Element();
                if ( ! ( colourElem ) || ! ( colourElem->GetText() ) ) {
                    LOGGER_ERROR ( _ ( "Un attribut colour invalide a ete trouve dans la palette du Style " ) << id <<_ ( " : il est invalide!!" ) );
                    continue;
                }
                b = atoi ( colourElem->GetText() );
                if ( b < 0 || b > 255 ) {
                    LOGGER_ERROR ( _ ( "Un attribut colour invalide a ete trouve dans la palette du Style " ) << id <<_ ( " : il est invalide!!" ) );
                    continue;
                }

                //Alpha
                colourElem = cHdl.FirstChild ( "alpha" ).Element();
                if ( ! ( colourElem ) || ! ( colourElem->GetText() ) ) {
                    a = 0 ;
                } else {
                    a = atoi ( colourElem->GetText() );
                }
                LOGGER_DEBUG ( _ ( "Style : " ) << id <<_ ( " Couleur XML de " ) <<value<<" = " <<r<<","<<g<<","<<b<<","<<a );
                colourMap.insert ( std::pair<double,Colour> ( value, Colour ( r,g,b,a ) ) );
            }

            if ( colourMap.size() == 0 ) {
                LOGGER_ERROR ( _ ( "Palette sans Couleur " ) << id <<_ ( " : il est invalide!!" ) );
                return NULL;
            }

        }
    }
    Palette pal ( colourMap, rgbContinuous, alphaContinuous, noAlpha );

    pElem = hRoot.FirstChild ( "estompage" ).Element();
    if ( pElem ) {
        errorCode = pElem->QueryIntAttribute ( "angle",&angle );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut angle invalide a ete trouve dans l'estompage du Style " ) << id <<_ ( " : il est invalide!!" ) );
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            angle=-1;
        }
        errorCode = pElem->QueryFloatAttribute ( "exaggeration",&exaggeration );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut exaggeration invalide a ete trouve dans l'estompage du Style " ) << id <<_ ( " : il est invalide!!" ) );
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            exaggeration=1;
        }

        errorCode = pElem->QueryIntAttribute ( "center",&center );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut center invalide a ete trouve dans l'estompage du Style " ) << id <<_ ( " : il est invalide!!" ) );
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            center=0;
        }
    }
    Style * style = new Style ( id,title,abstract,keyWords,legendURLs,pal ,angle,exaggeration,center );
    LOGGER_DEBUG ( _ ( "Style Cree" ) );
    return style;

}//parseStyle(TiXmlDocument* doc,std::string fileName,bool inspire)

Style* ConfLoader::buildStyle ( std::string fileName,bool inspire ) {
    TiXmlDocument doc ( fileName.c_str() );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "                Ne peut pas charger le fichier " )  << fileName );
        return NULL;
    }
    return parseStyle ( &doc,fileName,inspire );
}//buildStyle(std::string fileName,bool inspire)

TileMatrixSet* ConfLoader::parseTileMatrixSet ( TiXmlDocument* doc,std::string fileName ) {
    LOGGER_INFO ( _ ( "     Ajout du TMS " ) << fileName );
    std::string id;
    std::string title="";
    std::string abstract="";
    std::vector<Keyword> keyWords;
    std::map<std::string, TileMatrix> listTM;



    TiXmlHandle hDoc ( doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( fileName << _ ( "            Impossible de recuperer la racine." ) );
        return NULL;
    }
    if ( strcmp ( pElem->Value(),"tileMatrixSet" ) ) {
        LOGGER_ERROR ( fileName << _ ( "            La racine n'est pas un tileMatrixSet." ) );
        return NULL;
    }
    hRoot=TiXmlHandle ( pElem );

    unsigned int idBegin=fileName.rfind ( "/" );
    if ( idBegin == std::string::npos ) {
        idBegin=0;
    }
    unsigned int idEnd=fileName.rfind ( ".tms" );
    if ( idEnd == std::string::npos ) {
        idEnd=fileName.rfind ( ".TMS" );
        if ( idEnd == std::string::npos ) {
            idEnd=fileName.size();
        }
    }
    id=fileName.substr ( idBegin+1, idEnd-idBegin-1 );

    pElem=hRoot.FirstChild ( "crs" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( " pas de crs!!" ) );
        return NULL;
    }
    CRS crs ( pElem->GetTextStr() );

    pElem=hRoot.FirstChild ( "title" ).Element();
    if ( pElem && pElem->GetText() ) {
        title = pElem->GetTextStr();
    }

    pElem=hRoot.FirstChild ( "abstract" ).Element();
    if ( pElem && pElem->GetText() ) {
        abstract = pElem->GetTextStr();
    }

    for ( pElem=hRoot.FirstChild ( "keywordList" ).FirstChild ( "keyword" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "keyword" ) ) {
        if ( ! ( pElem->GetText() ) )
            continue;
        std::map<std::string,std::string> attributes;
        TiXmlAttribute* attrib = pElem->FirstAttribute();
        while ( attrib ) {
            attributes.insert ( attribute ( attrib->NameTStr(),attrib->ValueStr() ) );
            attrib = attrib->Next();
        }
        keyWords.push_back ( Keyword ( pElem->GetTextStr(),attributes ) );
    }

    for ( pElem=hRoot.FirstChild ( "tileMatrix" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "tileMatrix" ) ) {
        std::string tmId;
        double res;
        double x0;
        double y0;
        int tileW;
        int tileH;
        long int matrixW;
        long int matrixH;

        TiXmlHandle hTM ( pElem );
        TiXmlElement* pElemTM = hTM.FirstChild ( "id" ).Element();
        if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<", TileMatrix sans id!!" );
            return NULL;
        }
        tmId=pElemTM->GetText();

        pElemTM = hTM.FirstChild ( "resolution" ).Element();
        if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( " sans resolution!!" ) );
            return NULL;
        }
        if ( !sscanf ( pElemTM->GetText(),"%lf",&res ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( ": La resolution est inexploitable." ) );
            return NULL;
        }

        pElemTM = hTM.FirstChild ( "topLeftCornerX" ).Element();
        if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( " sans topLeftCornerX!!" ) );
            return NULL;
        }
        if ( !sscanf ( pElemTM->GetText(),"%lf",&x0 ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( ": Le topLeftCornerX est inexploitable." ) );
            return NULL;
        }

        pElemTM = hTM.FirstChild ( "topLeftCornerY" ).Element();
        if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( " sans topLeftCornerY!!" ) );
            return NULL;
        }
        if ( !sscanf ( pElemTM->GetText(),"%lf",&y0 ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( ": Le topLeftCornerY est inexploitable." ) );
            return NULL;
        }

        pElemTM = hTM.FirstChild ( "tileWidth" ).Element();
        if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( " sans tileWidth!!" ) );
            return NULL;
        }
        if ( !sscanf ( pElemTM->GetText(),"%d",&tileW ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( ": Le tileWidth est inexploitable." ) );
            return NULL;
        }

        pElemTM = hTM.FirstChild ( "tileHeight" ).Element();
        if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( " sans tileHeight!!" ) );
            return NULL;
        }
        if ( !sscanf ( pElemTM->GetText(),"%d",&tileH ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( ": Le tileHeight est inexploitable." ) );
            return NULL;
        }

        pElemTM = hTM.FirstChild ( "matrixWidth" ).Element();
        if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( " sans MatrixWidth!!" ) );
            return NULL;
        }
        if ( !sscanf ( pElemTM->GetText(),"%ld",&matrixW ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( ": Le MatrixWidth est inexploitable." ) );
            return NULL;
        }

        pElemTM = hTM.FirstChild ( "matrixHeight" ).Element();
        if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", TileMatrix " ) << tmId <<_ ( " sans matrixHeight!!" ) );
            return NULL;
        }
        if ( !sscanf ( pElemTM->GetText(),"%ld",&matrixH ) ) {
            LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( ", tileMatrix " ) << tmId <<_ ( ": Le matrixHeight est inexploitable." ) );
            return NULL;
        }

        TileMatrix tm ( tmId, res, x0, y0, tileW, tileH, matrixW, matrixH );
        listTM.insert ( std::pair<std::string, TileMatrix> ( tmId, tm ) );

    }// boucle sur le TileMatrix

    if ( listTM.size() ==0 ) {
        LOGGER_ERROR ( _ ( "Aucun tileMatrix trouve dans le tileMatrixSet" ) << id <<_ ( " : il est invalide!!" ) );
        return NULL;
    }
    TileMatrixSet * tms = new TileMatrixSet ( id,title,abstract,keyWords,crs,listTM );
    return tms;

}//buildTileMatrixSet(std::string)

TileMatrixSet* ConfLoader::buildTileMatrixSet ( std::string fileName ) {
    TiXmlDocument doc ( fileName.c_str() );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "                Ne peut pas charger le fichier " ) << fileName );
        return NULL;
    }
    return parseTileMatrixSet ( &doc,fileName );
}//buildTileMatrixSet(std::string fileName)

// Load a pyramid
Pyramid* ConfLoader::parsePyramid ( TiXmlDocument* doc,std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList, bool times, std::map<std::string,Style*> stylesList, Proxy proxy) {

    LOGGER_INFO ( _ ( "             Ajout de la pyramide : " ) << fileName );
    // Relative file Path
    char * fileNameChar = ( char * ) malloc ( strlen ( fileName.c_str() ) + 1 );
    strcpy ( fileNameChar, fileName.c_str() );
    char * parentDirChar = dirname ( fileNameChar );
    std::string parentDir ( parentDirChar );
    free ( fileNameChar );
    fileNameChar=NULL;
    parentDirChar=NULL;
    LOGGER_INFO ( _ ( "           BaseDir Relative to : " ) << parentDir );
    //----

    //----Variables
    TileMatrixSet *tms;
    std::string formatStr="";
    Rok4Format::eformat_data format;
    int channels;
    std::map<std::string, Level *> levels;
    bool onDemand = false;
    bool onDemandSpecific = false;
    int nbSpecificLevel = 0;
    Pyramid* basedPyramid = NULL;
    WebService *ws = NULL;
    bool onFly = false;
    bool testOnFly = true;
    std::map<std::string,std::vector<Pyramid*> > specificPyramids;
    std::map<std::string,std::vector<WebService*> > specificWebServices;
    TiXmlHandle hDoc ( doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );
    std::string photometricStr;
    std::string ndValuesStr;
    std::vector<int> noDataValues;
    //----

    //----------------------------------------------------------------------------------------------------
    // LECTURE DU FICHIER

    //----RACINE
    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( fileName << _ ( " impossible de recuperer la racine." ) );
        return NULL;
    }
    if ( strcmp ( pElem->Value(),"Pyramid" ) ) {
        LOGGER_ERROR ( fileName << _ ( " La racine n'est pas une Pyramid." ) );
        return NULL;
    }
    hRoot=TiXmlHandle ( pElem );
    //----

    //----TMS
    pElem=hRoot.FirstChild ( "tileMatrixSet" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << fileName <<_ ( "] n'a pas de TMS. C'est un probleme." ) );
        return NULL;
    }
    std::string tmsName= pElem->GetTextStr();
    std::map<std::string, TileMatrixSet *>::iterator it;
    it=tmsList.find ( tmsName );
    if ( it == tmsList.end() ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << fileName <<_ ( "] reference un TMS [" ) << tmsName <<_ ( "] qui n'existe pas." ) );
        return NULL;
    }
    tms=it->second;
    //----

    //----FORMAT
    pElem=hRoot.FirstChild ( "format" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << fileName <<_ ( "] n'a pas de format." ) );
        return NULL;
    }
    formatStr= pElem->GetTextStr();
    //  to remove when TIFF_RAW_INT8 et TIFF_RAW_FLOAT32 only will be used
    if ( formatStr.compare ( "TIFF_INT8" ) ==0 ) formatStr = "TIFF_RAW_INT8";
    if ( formatStr.compare ( "TIFF_FLOAT32" ) ==0 ) formatStr = "TIFF_RAW_FLOAT32";

    /*if (formatStr.compare("TIFF_RAW_INT8")!=0
         && formatStr.compare("TIFF_JPG_INT8")!=0
         && formatStr.compare("TIFF_PNG_INT8")!=0
         && formatStr.compare("TIFF_LZW_INT8")!=0
         && formatStr.compare("TIFF_RAW_FLOAT32")!=0
         && formatStr.compare("TIFF_LZW_FLOAT32")!=0){
                LOGGER_ERROR(fileName << "Le format ["<< formatStr <<"] n'est pas gere.");
                return NULL;
    }*/

    format = Rok4Format::fromString ( formatStr );
    if ( ! ( format ) ) {
        LOGGER_ERROR ( fileName << _ ( "Le format [" ) << formatStr <<_ ( "] n'est pas gere." ) );
        return NULL;
    }
    //----

    //----PHOTOMETRIE
    //on lit l'élément photometric, il n'est pas obligatoire pour
    //une pyramide normale mais il le devient si la pyramide
    //est à la volée
    pElem=hRoot.FirstChild ( "photometric" ).Element();
    if ( pElem && pElem->GetText() ) {
        photometricStr = pElem->GetTextStr();
    } else {
        photometricStr = "UNKNOWN";
    }
    //----


    //----CHANNELS
    pElem=hRoot.FirstChild ( "channels" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << fileName <<_ ( "] Pas de channels => channels = " ) << DEFAULT_CHANNELS );
        channels=DEFAULT_CHANNELS;
        return NULL;
    } else if ( !sscanf ( pElem->GetText(),"%d",&channels ) ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << fileName <<_ ( "] : channels=[" ) << pElem->GetTextStr() <<_ ( "] is not an integer." ) );
        return NULL;
    }
    //----

    //----NODATAVALUE
    //on lit l'élément nodatavalues, il n'est pas obligatoire pour
    //une pyramide normale mais il le devient si la pyramide
    //est à la volée
    pElem=hRoot.FirstChild ( "nodataValue" ).Element();
    if ( pElem && pElem->GetText() ) {
        ndValuesStr = pElem->GetTextStr();

        //conversion string->vector
        std::size_t found = ndValuesStr.find_first_of(",");
        std::string currentValue = ndValuesStr.substr(0,found);
        std::string endOfValues = ndValuesStr.substr(found+1);
        int curVal = atoi(currentValue.c_str());
        if (currentValue == "") {
            curVal = DEFAULT_NODATAVALUE;
        }
        noDataValues.push_back(curVal);
        while (found!=std::string::npos) {
            found = endOfValues.find_first_of(",");
            currentValue = endOfValues.substr(0,found);
            endOfValues = endOfValues.substr(found+1);
            curVal = atoi(currentValue.c_str());
            if (currentValue == "") {
                curVal = DEFAULT_NODATAVALUE;
            }
            noDataValues.push_back(curVal);
        }
        if (noDataValues.size() < channels) {
            LOGGER_ERROR("Le nombre de channels indique est different du nombre de noDataValue donne");
            int min = noDataValues.size();
            for (int i=min;i<channels;i++) {
                noDataValues.push_back(DEFAULT_NODATAVALUE);
            }
        }
    } else {
        for (int i=0;i<channels;i++) {
            noDataValues.push_back(DEFAULT_NODATAVALUE);
        }
    }
    //----

    //----LEVELS SECTION------------------------------------------------

    //on va vérifier que les levels sont spécifiés
    //si c'est une pyramide à la demande, ce n'est pas obligatoire
    if (hRoot.FirstChild ( "level" ).Element()) {

        for ( pElem=hRoot.FirstChild ( "level" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "level" ) ) {

            //----VARIABLE
            TileMatrix *tm;
            int32_t minTileRow=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
            int32_t maxTileRow=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
            int32_t minTileCol=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
            int32_t maxTileCol=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
            int tilesPerWidth;
            int tilesPerHeight;
            int pathDepth;
            std::string noDataFilePath="";
            std::vector<Pyramid*> sPyramids;
            std::vector<WebService*> sWebServices;
            int nsPyramids;
            int ntPyramids;
            int nsWebServices;
            int ntWebServices;
            bool specificLevel = false;
            bool alreadyLoad = false;
            //----

            //----TM
            TiXmlHandle hLvl ( pElem );
            TiXmlElement* pElemLvl = hLvl.FirstChild ( "tileMatrix" ).Element();
            if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
                LOGGER_ERROR ( fileName <<_ ( " level " ) <<_ ( "id" ) <<_ ( " sans tileMatrix!!" ) );
                return NULL;
            }
            std::string tmName ( pElemLvl->GetText() );
            std::string id ( tmName );
            //on va vérifier que le level qu'on veut charger n'a pas déjà été chargé
            if (levels.size() != 0) {
                for (std::map<std::string, Level *>::iterator lv = levels.begin(); lv != levels.end(); lv++) {
                    if (lv->second->getId() == id) {
                        LOGGER_ERROR ( _ ( "Level: " ) << id << _ ( " has already been loaded" ) );
                        alreadyLoad = true;
                        break;
                    }
                }
            }
            if (alreadyLoad) {
                continue;
            }
            std::map<std::string, TileMatrix>* tmList = tms->getTmList();
            std::map<std::string, TileMatrix>::iterator itTM = tmList->find ( tmName );

            if ( itTM == tmList->end() ) {
                LOGGER_ERROR ( fileName <<_ ( " Le level " ) << id <<_ ( " ref. Le TM [" ) << tmName << _ ( "] qui n'appartient pas au TMS [" ) << tmsName << "]" );
                return NULL;
            }
            tm = & ( itTM->second );
            //----

            //----ONDEMAND AND ONFLY SECTION------------------------------------

            //Si c'est la première fois qu'on parse une pyramide, times est true
            //  Cette pyramide peut être construite à partir d'une autre
            if (times) {


                //Si on se base sur d'autres pyramides pour faire la nouvelle pyramide
                TiXmlElement* pElemSP=hLvl.FirstChild ( "sources" ).Element();
                if ( pElemSP ) {

                    TiXmlHandle hbdP ( pElemSP );
                    bool timesSpecific = false;

                    nsPyramids = 0;
                    ntPyramids = 0;
                    nsWebServices = 0;
                    ntWebServices = 0;

                    TiXmlElement* sPyr = hbdP.FirstChild("basedPyramid").Element();

                    if (sPyr) {

                        for ( sPyr; sPyr; sPyr=sPyr->NextSiblingElement("basedPyramid") ) {

                            basedPyramid = parseBasedPyramid(sPyr,tmsList,timesSpecific,stylesList,parentDir, proxy);

                            if (basedPyramid) {

                                int up = updatePyrLevel(basedPyramid, tm, tms);
                                ntPyramids++;
                                if (up != 0 ) {
                                    sPyramids.push_back( basedPyramid ) ;
                                    nsPyramids++;
                                } else {
                                    LOGGER_ERROR("Impossible de supprimer les levels en trop dans la basedPyramid ");
                                }

                            } else {
                                LOGGER_ERROR ("Impossible de charger une basedPyramid indique");
                                cleanParsePyramid(specificPyramids,sPyramids,specificWebServices,sWebServices,levels);
                                return NULL;
                            }

                        }

                        //Aucune basedPyramid n'a pu être chargée pour ce level
                        if ( nsPyramids !=  ntPyramids) {
                            LOGGER_ERROR ( nsPyramids << _ (" equivalent level found for level ") << id << _ ( " but " ) << ntPyramids << _ ( " should be found" ) );
                            cleanParsePyramid(specificPyramids,sPyramids,specificWebServices,sWebServices,levels);
                            delete basedPyramid;
                            basedPyramid = NULL;
                            return NULL;
                        } else {
                            onDemandSpecific = true;
                            if (!specificLevel) {
                                specificLevel = true;
                                nbSpecificLevel++;
                            }
                            specificPyramids.insert(std::pair< std::string, std::vector<Pyramid*> > ( id, sPyramids));
                        }

                    }


                    TiXmlElement* sWeb = hbdP.FirstChild("webService").Element();
                    if (sWeb) {

                        for ( sWeb; sWeb; sWeb=sWeb->NextSiblingElement("webService") ) {

                            ws = parseWebService(sWeb,tms->getCrs(),format, proxy);
                            ntWebServices++;
                            if (ws) {
                                sWebServices.push_back(ws);
                                nsWebServices++;
                            } else {
                                LOGGER_ERROR("Impossible de charger le WebService indique");
                                return NULL;
                            }

                        }

                        if (ntWebServices != nsWebServices) {
                            LOGGER_ERROR ( nsWebServices << _ (" WebServices have been found for level ") << id << _ ( " but " ) << ntWebServices << _ ( " should be found" ) );
                            return NULL;
                        } else {
                            if (!specificLevel) {
                                specificLevel = true;
                                nbSpecificLevel++;
                            }
                            onDemandSpecific = true;
                            specificWebServices.insert(std::pair< std::string, std::vector<WebService*> > ( id, sWebServices));
                        }

                    }


                    if (ntWebServices == 0 && ntPyramids == 0) {
                        //sources est indiqué mais pas de basedPyramid, ni de WebService
                        LOGGER_ERROR ( _ ( "Pyramid: " ) << fileName << _ ( " can't be loaded bacause no basedPyramid or WebServices are specified" ) );
                        cleanParsePyramid(specificPyramids,sPyramids,specificWebServices,sWebServices,levels);
                        return NULL;
                    }





                }


            } else {
            //Si c'est la deuxième fois qu'on parse une pyramide

                if (hRoot.FirstChild ( "sources" ).Element()) {
                    LOGGER_ERROR ( _ ( "Pyramid: " ) << fileName << _ ( " can't depend on other pyramids" ) );
                    cleanParsePyramid(specificPyramids,sPyramids,specificWebServices,sWebServices,levels);
                    return NULL;
                }

            }

            //----END OF ONDEMAND AND ONFLY SECTION------------------------------------

            //----BASEDIR
            pElemLvl = hLvl.FirstChild ( "baseDir" ).Element();
            std::string baseDir;

            if (!onDemandSpecific) {
                if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( " sans baseDir!!" ) );
                    return NULL;
                }
                baseDir = pElemLvl->GetText() ;
                //Relative Path
                if ( baseDir.compare ( 0,2,"./" ) ==0 ) {
                    baseDir.replace ( 0,1,parentDir );
                } else if ( baseDir.compare ( 0,1,"/" ) !=0 ) {
                    baseDir.insert ( 0,"/" );
                    baseDir.insert ( 0,parentDir );
                }

            } else {

                 if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
                     baseDir = "";
                     testOnFly = false;
                 } else {
                     baseDir = pElemLvl->GetText() ;
                     //Relative Path
                     if ( baseDir.compare ( 0,2,"./" ) ==0 ) {
                         baseDir.replace ( 0,1,parentDir );
                     } else if ( baseDir.compare ( 0,1,"/" ) !=0 ) {
                         baseDir.insert ( 0,"/" );
                         baseDir.insert ( 0,parentDir );
                     }
                 }

            }
            //----

            //----TILEPERWIDTH
            pElemLvl = hLvl.FirstChild ( "tilesPerWidth" ).Element();
            if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
                LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": Pas de tilesPerWidth !!" ) );
                return NULL;
            }
            if ( !sscanf ( pElemLvl->GetText(),"%d",&tilesPerWidth ) ) {
                LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": tilesPerWidth=[" ) << pElemLvl->GetText() <<_ ( "] is not an integer." ) );
                return NULL;
            }
            //----

            //----TILEPERHEIGHT
            pElemLvl = hLvl.FirstChild ( "tilesPerHeight" ).Element();
            if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
                LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": Pas de tilesPerHeight !!" ) );
                return NULL;
            }
            if ( !sscanf ( pElemLvl->GetText(),"%d",&tilesPerHeight ) ) {
                LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": tilesPerHeight=[" ) << pElemLvl->GetText() <<_ ( "] is not an integer." ) );
                return NULL;
            }
            //----

            //----PATHDEPTH
            pElemLvl = hLvl.FirstChild ( "pathDepth" ).Element();
            if ( !pElemLvl || ! ( pElemLvl->GetText() ) ) {
                LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": Pas de pathDepth !!" ) );
                return NULL;
            }
            if ( !sscanf ( pElemLvl->GetText(),"%d",&pathDepth ) ) {
                LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": pathDepth=[" ) << pElemLvl->GetText() <<_ ( "] is not an integer." ) );
                return NULL;
            }
            //----

            //----TMSLIMITS
            TiXmlElement *pElemLvlTMS =hLvl.FirstChild ( "TMSLimits" ).Element();
            if ( pElemLvlTMS ) { // le bloc TMSLimits n'est pas obligatoire, mais s'il est là, il doit y avoir tous les champs.

                TiXmlHandle hTMSL ( pElemLvlTMS );
                TiXmlElement* pElemTMSL = hTMSL.FirstChild ( "minTileRow" ).Element();
                long int intBuff = -1;
                if ( !pElemTMSL ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": no minTileRow in TMSLimits element !!" ) );
                    return NULL;
                }
                if ( !pElemTMSL->GetText() ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": minTileRow is empty !!" ) );
                    return NULL;
                }
                if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": minTileRow=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
                    return NULL;
                }
                minTileRow = intBuff;
                intBuff = -1;
                pElemTMSL = hTMSL.FirstChild ( "maxTileRow" ).Element();
                if ( !pElemTMSL ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": no maxTileRow in TMSLimits element !!" ) );
                    return NULL;
                }
                if ( !pElemTMSL->GetText() ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": maxTileRow is empty !!" ) );
                    return NULL;
                }
                if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": maxTileRow=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
                    return NULL;
                }
                maxTileRow = intBuff;
                intBuff = -1;
                pElemTMSL = hTMSL.FirstChild ( "minTileCol" ).Element();
                if ( !pElemTMSL ) {
                    LOGGER_ERROR ( _ ( " Level " ) << id << _ ( ": no minTileCol in TMSLimits element !!" ) );
                    return NULL;
                }
                if ( !pElemTMSL->GetText() ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": minTileCol is empty !!" ) );
                    return NULL;
                }

                if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": minTileCol=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
                    return NULL;
                }
                minTileCol = intBuff;
                intBuff = -1;
                pElemTMSL = hTMSL.FirstChild ( "maxTileCol" ).Element();
                if ( !pElemTMSL ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": no maxTileCol in TMSLimits element !!" ) );
                    return NULL;
                }
                if ( !pElemTMSL->GetText() ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id << _ ( ": maxTileCol is empty !!" ) );
                    return NULL;
                }
                if ( !sscanf ( pElemTMSL->GetText(),"%ld",&intBuff ) ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( ": maxTileCol=[" ) << pElemTMSL->GetText() <<_ ( "] is not an integer." ) );
                    return NULL;
                }
                maxTileCol = intBuff;

            }

            if ( minTileCol > tm->getMatrixW() || minTileCol < 0 ) {
                minTileCol = 0;
            }
            if ( minTileRow > tm->getMatrixH() || minTileRow < 0 ) {
                minTileRow = 0;
            }
            if ( maxTileCol > tm->getMatrixW() || maxTileCol < 0 ) {
                maxTileCol = tm->getMatrixW();
            }
            if ( maxTileRow > tm->getMatrixH() || maxTileRow < 0 ) {
                maxTileRow = tm->getMatrixH();
            }

            //----

            //----NODATA
            // Must exist for normal pyramid but could possibly not exist for onDemand and onFly Pyramid
            // BUT the path must be written in conf file in these cases
            TiXmlElement* pElemNoData=hLvl.FirstChild ( "nodata" ).Element();

            if ( pElemNoData ) {    // FilePath must be specified if nodata tag exist

                TiXmlElement* pElemNoDataPath;
                pElemNoDataPath = hLvl.FirstChild ( "nodata" ).FirstChild ( "filePath" ).Element();
                if ( !pElemNoDataPath  || ! ( pElemNoDataPath->GetText() ) ) {
                    LOGGER_ERROR ( fileName <<_ ( " Level " ) << id <<_ ( " specifiant une tuile NoData sans chemin" ) );
                    return NULL;
                }

                noDataFilePath=pElemNoDataPath->GetText();
                //Relative Path
                if ( noDataFilePath.compare ( 0,2,"./" ) ==0 ) {
                    noDataFilePath.replace ( 0,1,parentDir );
                } else if ( noDataFilePath.compare ( 0,1,"/" ) !=0 ) {
                    noDataFilePath.insert ( 0,"/" );
                    noDataFilePath.insert ( 0,parentDir );
                }
                int file = open(noDataFilePath.c_str(),O_RDONLY);
                if (file < 0) {
                    if (!specificLevel) {
                        LOGGER_ERROR(fileName <<_ ( " Level " ) << id <<_ ( " specifiant une tuile NoData impossible a ouvrir" ));
                        cleanParsePyramid(specificPyramids,sPyramids,specificWebServices,sWebServices,levels);
                        return NULL;
                    }
                } else {
                    close(file);
                }

            } else {
                if (specificLevel) {
                    LOGGER_ERROR("NoDataTile must be specified for OnDemand Pyramid");
                    cleanParsePyramid(specificPyramids,sPyramids,specificWebServices,sWebServices,levels);
                    return NULL;
                }
            }
            //----

            Level *TL = new Level ( *tm, channels, baseDir, tilesPerWidth, tilesPerHeight,
                                    maxTileRow,  minTileRow, maxTileCol, minTileCol, pathDepth, format, noDataFilePath );

            levels.insert(std::pair<std::string,Level*> (id,TL));

            if (specificLevel && (sPyramids.size() != 0 || sWebServices.size() != 0)) {
                if ( !pElemLvlTMS ) {
                    updateTileLimits(*TL->getrefMinTileCol(),*TL->getrefMaxTileCol(),*TL->getrefMinTileRow(),*TL->getrefMaxTileRow(),TL->getTm(),tms,sPyramids,sWebServices);
                }
            }

        }// boucle sur les levels

        if (onDemandSpecific && testOnFly) {
            onFly = true;
        }

    } //if level

    //----END OF LEVELS SECTION------------------------------------------------

    // FIN DE LA LECTURE DU FICHIER
    //----------------------------------------------------------------------------------------------------


    if ( levels.size() ==0 ) {
        LOGGER_ERROR ( _ ( "Aucun level n'a pu etre charge pour la pyramide " ) << fileName );
        return NULL;
    }

    if ( onDemandSpecific ) {
        if (nbSpecificLevel == levels.size() ) {
             onDemand = true;
        } else {
            LOGGER_ERROR("Probleme lors du chargement de la pyramide => " << nbSpecificLevel << " trouvés pour " << levels.size() << " chargés");
            if (specificPyramids.size() != 0) {
                for ( std::map<std::string,std::vector<Pyramid*> >::iterator lv = specificPyramids.begin(); lv != specificPyramids.end(); lv++) {

                    if (lv->second.size() != 0) {
                        for ( std::vector<int>::size_type i = 0; i != lv->second.size(); i++) {
                            delete lv->second[i];
                            lv->second[i] = NULL;
                        }
                        lv->second.clear();
                    }
                }
                specificPyramids.clear();
            }
            if (levels.size() != 0) {
                for ( std::map<std::string,Level*>::iterator lv = levels.begin(); lv != levels.end(); lv++) {
                    delete lv->second;
                    lv->second = NULL;
                }
                levels.clear();
            }
            return NULL;
        }

    }

    //----PYRAMID

    Pyramid* pyr;

    if (onFly) {
        pyr = new PyramidOnFly(levels, *tms, format, channels, onDemand, onFly, Photometric::fromString(photometricStr),noDataValues,specificPyramids,specificWebServices);
    } else {
        if (onDemand) {
            pyr = new PyramidOnDemand(levels, *tms, format, channels, onDemand, onFly,specificPyramids,specificWebServices);
        } else {
            pyr = new Pyramid ( levels, *tms, format, channels, onDemand, onFly );
        }
    }

    //----

    return pyr;

}// buildPyramid()

void ConfLoader::cleanParsePyramid(std::map<std::string,std::vector<Pyramid*> > &specificPyramids, std::vector<Pyramid*> &sPyramids,std::map<std::string,std::vector<WebService*> > &specificWebServices, std::vector<WebService*> &sWebService,std::map<std::string, Level *> &levels) {

    if (sPyramids.size() != 0) {
        for ( std::vector<int>::size_type i = 0; i != sPyramids.size(); i++) {
            delete sPyramids[i];
            sPyramids[i] = NULL;
        }
        sPyramids.clear();
    }
    if (specificPyramids.size() != 0) {
        for ( std::map<std::string,std::vector<Pyramid*> >::iterator lv = specificPyramids.begin(); lv != specificPyramids.end(); lv++) {

            if (lv->second.size() != 0) {
                for ( std::vector<int>::size_type i = 0; i != lv->second.size(); i++) {
                    delete lv->second[i];
                    lv->second[i] = NULL;
                }
                lv->second.clear();
            }
        }
        specificPyramids.clear();
    }
    if (sWebService.size() != 0) {
        for ( std::vector<int>::size_type i = 0; i != sWebService.size(); i++) {
            delete sWebService[i];
            sWebService[i] = NULL;
        }
        sWebService.clear();
    }
    if (specificWebServices.size() != 0) {
        for ( std::map<std::string,std::vector<WebService*> >::iterator lv = specificWebServices.begin(); lv != specificWebServices.end(); lv++) {

            if (lv->second.size() != 0) {
                for ( std::vector<int>::size_type i = 0; i != lv->second.size(); i++) {
                    delete lv->second[i];
                    lv->second[i] = NULL;
                }
                lv->second.clear();
            }
        }
        specificWebServices.clear();
    }
    if (levels.size() != 0) {
        for ( std::map<std::string,Level*>::iterator lv = levels.begin(); lv != levels.end(); lv++) {
            delete lv->second;
            lv->second = NULL;
        }
        levels.clear();
    }

}


int ConfLoader::updatePyrLevel(Pyramid* pyr, TileMatrix *tm, TileMatrixSet *tms) {

    double Res, ratioX, ratioY, resX, resY;
    std::string best_h;

    Res = tm->getRes();

    BoundingBox<double> nBbox = tms->getCrs().getCrsDefinitionArea();

    BoundingBox<double> cBbox = pyr->getTms().getCrs().cropBBoxGeographic(nBbox);

    cBbox = tms->getCrs().cropBBoxGeographic(cBbox);
    BoundingBox<double> cBboxOld = cBbox;
    BoundingBox<double> cBboxNew = cBbox;

    if (cBboxNew.reproject("epsg:4326",tms->getCrs().getProj4Code())==0 &&
        cBboxOld.reproject("epsg:4326",pyr->getTms().getCrs().getProj4Code())==0)
    {

        ratioX = (cBboxOld.xmax - cBboxOld.xmin) / (cBboxNew.xmax - cBboxNew.xmin);
        ratioY = (cBboxOld.ymax - cBboxOld.ymin) / (cBboxNew.ymax - cBboxNew.ymin);

        resX = Res * ratioX;
        resY = Res * ratioY;

        //On recupère le best level de la basedPyramid en cours pour le tm en cours
        best_h = pyr->best_level(resX,resY,true);

    } else {
        //Si une des reprojections n'a pas marché

        best_h = "";

    }

    if (best_h != "") {

        std::vector<std::string> to_delete;
        std::map<std::string, Level*>::iterator lv = pyr->getLevels().begin();

        for ( ; lv != pyr->getLevels().end(); lv++) {
            if (lv->second->getId() != best_h) {
                to_delete.push_back(lv->second->getId());
            }
        }

        for (std::vector<int>::size_type i = 0; i != to_delete.size(); i++) {
            lv = pyr->getLevels().find(to_delete[i]);
            delete lv->second;
            lv->second = NULL;
            pyr->getLevels().erase(lv);
        }

        return 1;

    } else {
        return 0;
    }

}

void ConfLoader::updateTileLimits(uint32_t &minTileCol, uint32_t &maxTileCol, uint32_t &minTileRow, uint32_t &maxTileRow, TileMatrix tm, TileMatrixSet *tms, std::vector<Pyramid *> bPyramids,std::vector<WebService *> bWebServices) {

    //On met à jour les Min et Max Tiles une fois que l'on a trouvé un équivalent dans chaque basedPyramid
    // pour le level créé

    int curMinCol, curMaxCol, curMinRow, curMaxRow, bPMinCol, bPMaxCol, bPMinRow, bPMaxRow, minCol, minRow, maxCol, maxRow;
    double xo, yo, res, tileW, tileH, xmin, xmax, ymin, ymax;

    int time = 1;

    if (bPyramids.size() != 0) {

        for (int ip = 0; ip < bPyramids.size(); ip++) {
            Level *lv;

            //On récupére les Min et Max de la basedPyramid
            lv = bPyramids.at(ip)->getLevels().begin()->second;


            bPMinCol = lv->getMinTileCol();
            bPMaxCol = lv->getMaxTileCol();
            bPMinRow = lv->getMinTileRow();
            bPMaxRow = lv->getMaxTileRow();

            //On récupère d'autres informations sur le TM
            xo = lv->getTm().getX0();
            yo = lv->getTm().getY0();
            res = lv->getTm().getRes();
            tileW = lv->getTm().getTileW();
            tileH = lv->getTm().getTileH();

            //On transforme en bbox
            xmin = bPMinCol * tileW * res + xo;
            ymax = yo - bPMinRow * tileH * res;
            xmax = xo + (bPMaxCol+1) * tileW * res;
            ymin = ymax - (bPMaxRow - bPMinRow + 1) * tileH * res;

            BoundingBox<double> MMbbox(xmin,ymin,xmax,ymax);


            //On reprojette la bbox
            MMbbox.reproject(bPyramids.at(ip)->getTms().getCrs().getProj4Code(), tms->getCrs().getProj4Code());

            //On récupère les Min et Max de Pyr pour ce level dans la nouvelle projection
            xo = tm.getX0();
            yo = tm.getY0();
            res = tm.getRes();
            tileW = tm.getTileW();
            tileH = tm.getTileH();

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

            time++;

        }
    }

    if (bWebServices.size() != 0) {

        for (int ip = 0; ip < bWebServices.size(); ip++) {
            WebMapService *wms = reinterpret_cast<WebMapService*>(bWebServices.at(ip));

            BoundingBox<double> MMbbox = wms->getBbox();

            //On récupère les Min et Max de Pyr pour ce level dans la nouvelle projection
            xo = tm.getX0();
            yo = tm.getY0();
            res = tm.getRes();
            tileW = tm.getTileW();
            tileH = tm.getTileH();

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

            time++;

        }
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


}

WebService *ConfLoader::parseWebService(TiXmlElement* sWeb, CRS pyrCRS, Rok4Format::eformat_data pyrFormat, Proxy proxy_default) {

    WebService * ws = NULL;
    std::string url, user, proxy, noProxy,pwd, referer, userAgent, version, layers, styles, format, crs;
    std::map<std::string,std::string> options;
    int timeout, retry, interval, channels;
    std::string name,ndValuesStr,value;
    BoundingBox<double> bbox = BoundingBox<double> (0.,0.,0.,0.);
    std::vector<int> noDataValues;

    TiXmlElement* sUrl = sWeb->FirstChildElement("url");
    if (sUrl && sUrl->GetText()) {
        url = sUrl->GetTextStr();

        std::size_t found = url.find(" ");
        if (found!=std::string::npos) {
          LOGGER_ERROR("Une URL ne peut contenir des espaces");
          return NULL;
        }

        found = url.find("?");
        size_t size = url.size()-1;
        if (found!=std::string::npos && found!=size) {
            LOGGER_ERROR("Une URL ne peut contenir un ou des '?' hormis le dernier qui est un séparateur");
            return NULL;
        }

        if (found==std::string::npos) {
            url = url + "?";
        }

    } else {
        LOGGER_ERROR("Une URL doit etre specifiee pour un WebService");
        return NULL;
    }

    TiXmlElement* sProxy = sWeb->FirstChildElement("proxy");
    if (sProxy && sProxy->GetText()) {
        proxy = sProxy->GetTextStr();
    } else {
        proxy = proxy_default.proxyName;
    }

    sProxy = sWeb->FirstChildElement("noProxy");
    if (sProxy && sProxy->GetText()) {
        noProxy = sProxy->GetTextStr();
    } else {
        noProxy = proxy_default.noProxy;
    }

    TiXmlElement* sTimeOut = sWeb->FirstChildElement("timeout");
    if (sTimeOut && sTimeOut->GetText()) {
        timeout = atoi(sTimeOut->GetText());
    } else {
        timeout = DEFAULT_TIMEOUT;
    }

    TiXmlElement* sRetry = sWeb->FirstChildElement("retry");
    if (sRetry && sRetry->GetText()) {
        retry = atoi(sRetry->GetText());
    } else {
        retry = DEFAULT_RETRY;
    }

    TiXmlElement* sInterval = sWeb->FirstChildElement("interval");
    if (sInterval && sInterval->GetText()) {
        interval = atoi(sInterval->GetText());
    } else {
        interval = DEFAULT_INTERVAL;
    }

    TiXmlElement* sUser = sWeb->FirstChildElement("user");
    if (sUser && sUser->GetText()) {
        user = sUser->GetTextStr();
    } else {
        user = "";
    }

    TiXmlElement* sPwd = sWeb->FirstChildElement("password");
    if (sPwd && sPwd->GetText()) {
        pwd = sPwd->GetTextStr();
    } else {
        pwd = "";
    }

    TiXmlElement* sReferer = sWeb->FirstChildElement("referer");
    if (sReferer && sReferer->GetText()) {
        referer = sReferer->GetTextStr();
    } else {
        referer = "";
    }

    TiXmlElement* sUserAgent = sWeb->FirstChildElement("userAgent");
    if (sUserAgent && sUserAgent->GetText()) {
        userAgent = sUserAgent->GetTextStr();
    } else {
        userAgent = "";
    }


    TiXmlElement* sWMS = sWeb->FirstChildElement("wms");
    if (sWMS) {

        TiXmlElement* sVersion = sWMS->FirstChildElement("version");
        if (sVersion && sVersion->GetText()) {
            version = sVersion->GetTextStr();
        } else {
            LOGGER_ERROR("Un WMS doit contenir une version");
            return NULL;
        }

        TiXmlElement* sLayers= sWMS->FirstChildElement("layers");
        if (sLayers && sLayers->GetText()) {
            layers = sLayers->GetTextStr();
        } else {
            LOGGER_ERROR("Un WMS doit contenir un ou des layers séparés par des virgules");
            return NULL;
        }

        TiXmlElement* sStyles = sWMS->FirstChildElement("styles");
        if (sStyles && sStyles->GetText()) {
            styles = sStyles->GetTextStr();
        } else {
            LOGGER_ERROR("Un WMS doit contenir un ou des styles séparés par des virgules");
            return NULL;
        }

        TiXmlElement* sFormat = sWMS->FirstChildElement("format");
        if (sFormat && sFormat->GetText()) {
            format = sFormat->GetTextStr();
            Rok4Format::eformat_data fmt = Rok4Format::fromMimeType(format);
            if (fmt == Rok4Format::UNKNOWN) {
                LOGGER_ERROR("Un WMS doit être requete dans un format lisible par rok4");
                return NULL;
            }
            //Pour le moment, on autorise que deux formats (jpeg et png)
            //car les autres ne sont pas gérer correctement par les decodeurs de Rok4
            //il faudrait notamment creer un decodeur pour le tiff (lecture de l'en-tête, puis decompression)
            if (format != "image/jpeg" && format != "image/png") {
                LOGGER_ERROR("Un WMS doit être requete en image/jpeg ou image/png");
                return NULL;
            }
        } else {
            format = Rok4Format::toString(pyrFormat);
            LOGGER_ERROR("Un WMS doit contenir un format. Par défaut => " << format);
        }

        TiXmlElement* sCrs = sWMS->FirstChildElement("crs");
        if (sCrs && sCrs->GetText()) {
            crs = sCrs->GetTextStr();

            //le crs demandé et le crs de la pyramide en construction doivent être le même
            CRS askedCRS = CRS(crs);
            if (askedCRS != pyrCRS) {
                LOGGER_ERROR("Un WMS doit contenir un crs équivalent à celui de la pyramide en construction");
                return NULL;
            }

        } else {
            crs = pyrCRS.getProj4Code();
            LOGGER_ERROR("Un WMS doit contenir un crs. Par défaut => " << crs);
        }

        TiXmlElement* sChannels = sWMS->FirstChildElement("channels");
        if (sChannels && sChannels->GetText()) {
            channels = atoi(sChannels->GetTextStr().c_str());
        } else {
            LOGGER_ERROR("Un WMS doit contenir un channels");
            return NULL;
        }

        TiXmlElement* sOpt = sWMS->FirstChildElement("option");
        if (sOpt) {

            for ( sOpt; sOpt; sOpt=sOpt->NextSiblingElement() ) {

                name = sOpt->Attribute("name");
                value = sOpt->Attribute("value");

                if (name != "" && value != "") {
                    options.insert(std::pair<std::string,std::string> ( name, value));
                }

            }

        }

        TiXmlElement* sBbox = sWMS->FirstChildElement("bbox");
        if (sBbox) {
            if ( ! ( sBbox->Attribute ( "minx" ) ) ) {
                LOGGER_ERROR ( "minx attribute is missing" );
                return NULL;
            }
            if ( !sscanf ( sBbox->Attribute ( "minx" ),"%lf",&bbox.xmin) ) {
                LOGGER_ERROR ( "Le minx est inexploitable:[" << sBbox->Attribute ( "minx" ) << "]" );
                return NULL;
            }
            if ( ! ( sBbox->Attribute ( "miny" ) ) ) {
                LOGGER_ERROR ( "miny attribute is missing" );
                return NULL;
            }
            if ( !sscanf ( sBbox->Attribute ( "miny" ),"%lf",&bbox.ymin ) ) {
                LOGGER_ERROR ("Le miny est inexploitable:[" << sBbox->Attribute ( "miny" ) << "]" );
                return NULL;
            }
            if ( ! ( sBbox->Attribute ( "maxx" ) ) ) {
                LOGGER_ERROR (  "maxx attribute is missing"  );
                return NULL;
            }
            if ( !sscanf ( sBbox->Attribute ( "maxx" ),"%lf",&bbox.xmax ) ) {
                LOGGER_ERROR (  "Le maxx est inexploitable:["  << sBbox->Attribute ( "maxx" ) << "]" );
                return NULL;
            }
            if ( ! ( sBbox->Attribute ( "maxy" ) ) ) {
                LOGGER_ERROR (  "maxy attribute is missing" );
                return NULL;
            }
            if ( !sscanf ( sBbox->Attribute ( "maxy" ),"%lf",&bbox.ymax ) ) {
                LOGGER_ERROR (  "Le maxy est inexploitable:["  << sBbox->Attribute ( "maxy" ) << "]" );
                return NULL;
            }

        } else {
            LOGGER_ERROR("Un WMS doit contenir une bbox");
            return NULL;
        }

        TiXmlElement* pND=sWMS->FirstChildElement ( "noDataValue" );
        if ( pND && pND->GetText() ) {
            ndValuesStr = pND->GetTextStr();

            //conversion string->vector
            std::size_t found = ndValuesStr.find_first_of(",");
            std::string currentValue = ndValuesStr.substr(0,found);
            std::string endOfValues = ndValuesStr.substr(found+1);
            int curVal = atoi(currentValue.c_str());
            if (currentValue == "") {
                curVal = DEFAULT_NODATAVALUE;
            }
            noDataValues.push_back(curVal);
            while (found!=std::string::npos) {
                found = endOfValues.find_first_of(",");
                currentValue = endOfValues.substr(0,found);
                endOfValues = endOfValues.substr(found+1);
                curVal = atoi(currentValue.c_str());
                if (currentValue == "") {
                    curVal = DEFAULT_NODATAVALUE;
                }
                noDataValues.push_back(curVal);
            }
            if (noDataValues.size() < channels) {
                LOGGER_ERROR("Le nombre de channels indique est different du nombre de noDataValue donne");
                return NULL;
            }
        } else {
            for (int i=0;i<channels;i++) {
                noDataValues.push_back(DEFAULT_NODATAVALUE);
            }
        }

        ws = new WebMapService(url, proxy, noProxy, retry, interval, timeout, version, layers, styles, format, channels, crs, bbox, noDataValues,options);

    } else {
        //On retourne une erreur car le WMS est le seul WebService disponible pour le moment
        LOGGER_ERROR("Un WebService doit contenir un WMS pour être utilisé");
        return NULL;
    }

    return ws;

}

Pyramid *ConfLoader::parseBasedPyramid(TiXmlElement* sPyr, std::map<std::string, TileMatrixSet*> &tmsList, bool timesSpecific, std::map<std::string,Style*> stylesList, std::string parentDir, Proxy proxy) {

    Pyramid *basedPyramid;

    TiXmlElement* sFile = sPyr->FirstChildElement("file");
    TiXmlElement* sTransparent = sPyr->FirstChildElement("transparent");
    TiXmlElement* sStyle = sPyr->FirstChildElement("style");

    bool transparent = false;
    std::string str_transparent,basedPyramidFilePath;
    std::string str_style = "";
    Style *style = NULL;

    if (sFile && sTransparent && sStyle && sFile->GetText() && sTransparent->GetText() && sStyle->GetText()) {

        str_transparent = sTransparent->GetTextStr();
        str_style = sStyle->GetTextStr();

        basedPyramidFilePath = sFile->GetTextStr() ;
        //Relative Path
        if ( basedPyramidFilePath.compare ( 0,2,"./" ) ==0 ) {
            basedPyramidFilePath.replace ( 0,1,parentDir );
        } else if ( basedPyramidFilePath.compare ( 0,1,"/" ) !=0 ) {
            basedPyramidFilePath.insert ( 0,"/" );
            basedPyramidFilePath.insert ( 0,parentDir );
        }

        basedPyramid = buildPyramid ( basedPyramidFilePath, tmsList, timesSpecific, stylesList, proxy );

        if ( !basedPyramid) {
            LOGGER_ERROR ( _ ( "La pyramide " ) << basedPyramidFilePath << _ ( " ne peut etre chargee" ) );
            return NULL;
        } else {


            if (str_transparent == "true") {
                transparent = true;
                basedPyramid->setTransparent(transparent);
            } else {
                basedPyramid->setTransparent(transparent);
            }

            std::map<std::string, Style*>::iterator styleIt= stylesList.find ( str_style );
            if ( styleIt == stylesList.end() ) {
                LOGGER_ERROR ( _ ( "Style " ) << str_style << _ ( "non defini" ) );
                styleIt= stylesList.find ( "normal" );
                if (styleIt != stylesList.end()) {
                    style = styleIt->second;
                }
            } else {
                style = styleIt->second;
            }

            basedPyramid->setStyle(style);

        }

    } else {
        //Il manque un des trois elements necessaires pour initialiser une
        //nouvelle pyramide de base
        LOGGER_ERROR ( _ ( "Pyramid: " ) << basedPyramidFilePath << _ ( " can't be loaded because information are missing" ) );
        return NULL;
    }

    return basedPyramid;

}


Pyramid* ConfLoader::buildPyramid ( std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList, bool times, std::map<std::string,Style*> stylesList, Proxy proxy) {
    TiXmlDocument doc ( fileName.c_str() );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "Ne peut pas charger le fichier " ) << fileName );
        return NULL;
    }
    return parsePyramid ( &doc,fileName,tmsList, times, stylesList, proxy);
}


//TODO avoid opening a pyramid file directly
Layer * ConfLoader::parseLayer ( TiXmlDocument* doc,std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList,std::map<std::string,Style*> stylesList , bool reprojectionCapability, ServicesConf* servicesConf, Proxy proxy ) {
    LOGGER_INFO ( _ ( "     Ajout du layer " ) << fileName );
    // Relative file Path
    char * fileNameChar = ( char * ) malloc ( strlen ( fileName.c_str() ) + 1 );
    strcpy ( fileNameChar, fileName.c_str() );
    char * parentDirChar = dirname ( fileNameChar );
    std::string parentDir ( parentDirChar );
    free ( fileNameChar );
    fileNameChar=NULL;
    parentDirChar=NULL;
    LOGGER_INFO ( _ ( "           BaseDir Relative to : " ) << parentDir );

    bool inspire = servicesConf->isInspire();

    std::string id;
    std::string title="";
    std::string abstract="";
    std::vector<Keyword> keyWords;
    std::string styleName="";
    std::vector<Style*> styles;
    double minRes;
    double maxRes;
    std::vector<CRS> WMSCRSList;
    bool opaque;
    std::string authority="";
    std::string resamplingStr="";
    Interpolation::KernelType resampling;
    Pyramid* pyramid;
    GeographicBoundingBoxWMS geographicBoundingBox;
    BoundingBoxWMS boundingBox;
    std::vector<MetadataURL> metadataURLs;
    bool WMSauth = true;
    bool WMTSauth = true;
    bool times = true;

    TiXmlHandle hDoc ( doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    bool getFeatureInfoAvailability = false;
    std::string getFeatureInfoType = "";
    std::string getFeatureInfoBaseURL = "";
    std::string GFIService = "";
    std::string GFIVersion = "";
    std::string GFIQueryLayers = "";
    std::string GFILayers = "";
    bool GFIForceEPSG = true;

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( fileName << _ ( " impossible de recuperer la racine." ) );
        return NULL;
    }
    if ( strcmp ( pElem->Value(),"layer" ) ) {
        LOGGER_ERROR ( fileName << _ ( " La racine n'est pas un layer." ) );
        return NULL;
    }
    hRoot=TiXmlHandle ( pElem );

    unsigned int idBegin=fileName.rfind ( "/" );
    if ( idBegin == std::string::npos ) {
        idBegin=0;
    }
    unsigned int idEnd=fileName.rfind ( ".lay" );
    if ( idEnd == std::string::npos ) {
        idEnd=fileName.rfind ( ".LAY" );
        if ( idEnd == std::string::npos ) {
            idEnd=fileName.size();
        }
    }
    id=fileName.substr ( idBegin+1, idEnd-idBegin-1 );

    pElem=hRoot.FirstChild ( "title" ).Element();
    if ( pElem && pElem->GetText() ) {
        title= pElem->GetTextStr();
    }

    pElem=hRoot.FirstChild ( "abstract" ).Element();
    if ( pElem && pElem->GetText() ) {
        abstract= pElem->GetTextStr();
    }

    pElem=hRoot.FirstChild ( "WMSAuthorized" ).Element();
    if ( pElem && pElem->GetText() && pElem->GetTextStr()=="false") {
        WMSauth= false;
    }

    pElem=hRoot.FirstChild ( "WMTSAuthorized" ).Element();
    if ( pElem && pElem->GetText() && pElem->GetTextStr()=="false") {
        WMTSauth= false;
    }

    pElem=hRoot.FirstChild("getFeatureInfoAvailability").Element();
    if ( pElem && pElem->GetText() && pElem->GetTextStr()=="true") {
        getFeatureInfoAvailability= true;

        pElem=hRoot.FirstChild("getFeatureInfoType").Element();
        if ( pElem && pElem->GetText()) {
            getFeatureInfoType = pElem->GetTextStr();
        }

        // en fonction du type : pas le meme schema xml
        if(getFeatureInfoType.compare("PYRAMID") == 0){
            // Donnee elle-meme
        }else if(getFeatureInfoType.compare("EXTERNALWMS") == 0){
            // WMS
            hDoc=hRoot.FirstChild("getFeatureInfoUrl");
            pElem=hDoc.FirstChild("getFeatureInfoBaseURL").Element();
            if ( pElem && pElem->GetText()) {
                getFeatureInfoBaseURL = pElem->GetTextStr();
		std::string a = getFeatureInfoBaseURL.substr(getFeatureInfoBaseURL.length()-1, 1);
		if ( a.compare("?") != 0 ) {
			getFeatureInfoBaseURL = getFeatureInfoBaseURL + "?";
		}
            }
            pElem=hDoc.FirstChild("layers").Element();
            if ( pElem && pElem->GetText()) {
                GFILayers = pElem->GetTextStr();
            }
            pElem=hDoc.FirstChild("queryLayers").Element();
            if ( pElem && pElem->GetText()) {
                GFIQueryLayers = pElem->GetTextStr();
            }
            pElem=hDoc.FirstChild("version").Element();
            if ( pElem && pElem->GetText()) {
                GFIVersion = pElem->GetTextStr();
            }
            pElem=hDoc.FirstChild("service").Element();
            if ( pElem && pElem->GetText()) {
                GFIService = pElem->GetTextStr();
            }
            pElem=hDoc.FirstChild("forceEPSG").Element();
            if ( pElem && pElem->GetText()=="false") {
                GFIForceEPSG = false;
            }
        }else if(getFeatureInfoType.compare("SQL") == 0){
            // SQL
        }else{
            LOGGER_ERROR ( fileName << _ ( "La source du GetFeatureInfo n'est pas autorisée." ) );
            return NULL;
        }
    }

    //

    for ( pElem=hRoot.FirstChild ( "keywordList" ).FirstChild ( "keyword" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "keyword" ) ) {
        if ( ! ( pElem->GetText() ) )
            continue;
        std::map<std::string,std::string> attributes;
        TiXmlAttribute* attrib = pElem->FirstAttribute();
        while ( attrib ) {
            attributes.insert ( attribute ( attrib->NameTStr(),attrib->ValueStr() ) );
            attrib = attrib->Next();
        }
        keyWords.push_back ( Keyword ( pElem->GetTextStr(),attributes ) );
    }
    std::string inspireStyleName = DEFAULT_STYLE_INSPIRE;
    for ( pElem=hRoot.FirstChild ( "style" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "style" ) ) {
        if ( !pElem || ! ( pElem->GetText() ) ) {
            LOGGER_ERROR ( _ ( "Pas de style => style = " ) << ( inspire?DEFAULT_STYLE_INSPIRE:DEFAULT_STYLE ) );
            styleName = ( inspire?DEFAULT_STYLE_INSPIRE:DEFAULT_STYLE );
        } else {
            styleName = pElem->GetTextStr();
        }
        std::map<std::string, Style*>::iterator styleIt= stylesList.find ( styleName );
        if ( styleIt == stylesList.end() ) {
            LOGGER_ERROR ( _ ( "Style " ) << styleName << _ ( "non defini" ) );
            continue;
        }

        if ( styleIt->second->getId().compare ( DEFAULT_STYLE_INSPIRE_ID ) ==0 ) {
            inspireStyleName = styleName;
        }
        styles.push_back ( styleIt->second );
        if ( inspire && ( styleName==inspireStyleName ) ) {
            styles.pop_back();
        }
    }
    if ( inspire ) {
        std::map<std::string, Style*>::iterator styleIt= stylesList.find ( inspireStyleName );
        if ( styleIt != stylesList.end() ) {
            styles.insert ( styles.begin(),styleIt->second );
        } else {
            LOGGER_ERROR ( _ ( "Style " ) << styleName << _ ( "non defini" ) );
            return NULL;
        }
    }
    if ( styles.size() ==0 ) {
        LOGGER_ERROR ( _ ( "Pas de Style defini, Layer non valide" ) );
        return NULL;
    }

    pElem = hRoot.FirstChild ( "minRes" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        minRes=0.;
    } else if ( !sscanf ( pElem->GetText(),"%lf",&minRes ) ) {
        LOGGER_ERROR ( _ ( "La resolution min est inexploitable:[" ) << pElem->GetTextStr() << "]" );
        return NULL;
    }

    pElem = hRoot.FirstChild ( "maxRes" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        maxRes=0.;
    } else if ( !sscanf ( pElem->GetText(),"%lf",&maxRes ) ) {
        LOGGER_ERROR ( _ ( "La resolution max est inexploitable:[" ) << pElem->GetTextStr() << "]" );
        return NULL;
    }
    // EX_GeographicBoundingBox
    pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).Element();
    if ( !pElem ) {
        LOGGER_ERROR ( _ ( "Pas de geographicBoundingBox = " ) );
        return NULL;
    } else {
        // westBoundLongitude
        pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).FirstChild ( "westBoundLongitude" ).Element();
        if ( !pElem  || ! ( pElem->GetText() ) ) {
            LOGGER_ERROR ( _ ( "Pas de westBoundLongitude" ) );
            return NULL;
        }
        if ( !sscanf ( pElem->GetText(),"%lf",&geographicBoundingBox.minx ) ) {
            LOGGER_ERROR ( _ ( "Le westBoundLongitude est inexploitable:[" ) << pElem->GetTextStr() << "]" );
            return NULL;
        }
        // southBoundLatitude
        pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).FirstChild ( "southBoundLatitude" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            LOGGER_ERROR ( _ ( "Pas de southBoundLatitude" ) );
            return NULL;
        }
        if ( !sscanf ( pElem->GetText(),"%lf",&geographicBoundingBox.miny ) ) {
            LOGGER_ERROR ( _ ( "Le southBoundLatitude est inexploitable:[" ) << pElem->GetTextStr() << "]" );
            return NULL;
        }
        // eastBoundLongitude
        pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).FirstChild ( "eastBoundLongitude" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            LOGGER_ERROR ( _ ( "Pas de eastBoundLongitude" ) );
            return NULL;
        }
        if ( !sscanf ( pElem->GetText(),"%lf",&geographicBoundingBox.maxx ) ) {
            LOGGER_ERROR ( _ ( "Le eastBoundLongitude est inexploitable:[" ) << pElem->GetTextStr() << "]" );
            return NULL;
        }
        // northBoundLatitude
        pElem = hRoot.FirstChild ( "EX_GeographicBoundingBox" ).FirstChild ( "northBoundLatitude" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            LOGGER_ERROR ( _ ( "Pas de northBoundLatitude" ) );
            return NULL;
        }
        if ( !sscanf ( pElem->GetText(),"%lf",&geographicBoundingBox.maxy ) ) {
            LOGGER_ERROR ( _ ( "Le northBoundLatitude est inexploitable:[" ) << pElem->GetTextStr() << "]" );
            return NULL;
        }
    }

    pElem = hRoot.FirstChild ( "boundingBox" ).Element();
    if ( !pElem ) {
        LOGGER_ERROR ( _ ( "Pas de BoundingBox" ) );
    } else {
        if ( ! ( pElem->Attribute ( "CRS" ) ) ) {
            LOGGER_ERROR ( _ ( "Le CRS est inexploitable:[" ) << pElem->GetTextStr() << "]" );
            return NULL;
        }
        boundingBox.srs=pElem->Attribute ( "CRS" );
        if ( ! ( pElem->Attribute ( "minx" ) ) ) {
            LOGGER_ERROR ( _ ( "minx attribute is missing" ) );
            return NULL;
        }
        if ( !sscanf ( pElem->Attribute ( "minx" ),"%lf",&boundingBox.minx ) ) {
            LOGGER_ERROR ( _ ( "Le minx est inexploitable:[" ) << pElem->Attribute ( "minx" ) << "]" );
            return NULL;
        }
        if ( ! ( pElem->Attribute ( "miny" ) ) ) {
            LOGGER_ERROR ( _ ( "miny attribute is missing" ) );
            return NULL;
        }
        if ( !sscanf ( pElem->Attribute ( "miny" ),"%lf",&boundingBox.miny ) ) {
            LOGGER_ERROR ( _ ( "Le miny est inexploitable:[" ) << pElem->Attribute ( "miny" ) << "]" );
            return NULL;
        }
        if ( ! ( pElem->Attribute ( "maxx" ) ) ) {
            LOGGER_ERROR ( _ ( "maxx attribute is missing" ) );
            return NULL;
        }
        if ( !sscanf ( pElem->Attribute ( "maxx" ),"%lf",&boundingBox.maxx ) ) {
            LOGGER_ERROR ( _ ( "Le maxx est inexploitable:[" ) << pElem->Attribute ( "maxx" ) << "]" );
            return NULL;
        }
        if ( ! ( pElem->Attribute ( "maxy" ) ) ) {
            LOGGER_ERROR ( _ ( "maxy attribute is missing" ) );
            return NULL;
        }
        if ( !sscanf ( pElem->Attribute ( "maxy" ),"%lf",&boundingBox.maxy ) ) {
            LOGGER_ERROR ( _ ( "Le maxy est inexploitable:[" ) << pElem->Attribute ( "maxy" ) << "]" );
            return NULL;
        }
    }

    if ( reprojectionCapability==true ) {
        for ( pElem=hRoot.FirstChild ( "WMSCRSList" ).FirstChild ( "WMSCRS" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "WMSCRS" ) ) {
            if ( ! ( pElem->GetText() ) )
                continue;
            std::string str_crs ( pElem->GetTextStr() );
            // On verifie que la CRS figure dans la liste des CRS de proj4 (sinon, le serveur n est pas capable de la gerer)
            CRS crs ( str_crs );
            bool crsOk=true;
            if ( !crs.isProj4Compatible() ) {
                LOGGER_WARN ( _ ( "Le CRS " ) <<str_crs<<_ ( " n est pas reconnu par Proj4 et n est donc par ajoute aux CRS de la couche" ) );
                crsOk = false;
            } else {
                //Test if already define in Global CRS

                for ( unsigned int k=0; k<servicesConf->getGlobalCRSList()->size(); k++ )
                    if ( crs.cmpRequestCode ( servicesConf->getGlobalCRSList()->at ( k ).getRequestCode() ) ) {
                        crsOk = false;
                        LOGGER_INFO ( _ ( "         CRS " ) <<str_crs << _ ( " already present in global CRS list" ) );
                        break;
                    }
                // Test if the current layer bounding box is compatible with the current CRS
                if ( inspire && !crs.validateBBoxGeographic ( geographicBoundingBox.minx,geographicBoundingBox.miny,geographicBoundingBox.maxx,geographicBoundingBox.maxy ) ) {
                    BoundingBox<double> cropBBox = crs.cropBBoxGeographic ( geographicBoundingBox.minx,geographicBoundingBox.miny,geographicBoundingBox.maxx,geographicBoundingBox.maxy );
                    // Test if the remaining bbox contain useful data
                    if ( cropBBox.xmax - cropBBox.xmin <= 0 || cropBBox.ymax - cropBBox.ymin <= 0 ) {
                        LOGGER_WARN ( _ ( "         Le CRS " ) <<str_crs<<_ ( " n est pas compatible avec l'emprise de la couche" ) );
                        crsOk = false;
                    }
                }
                
                if ( crsOk ){
                    bool allowedCRS = true;
                    std::vector<CRS> tmpEquilist;
                    if (servicesConf->getAddEqualsCRS()){
                        tmpEquilist = getEqualsCRS(servicesConf->getListOfEqualsCRS(), str_crs );
                    }
                    if ( servicesConf->getDoWeRestrictCRSList() ){
                        allowedCRS = isCRSAllowed(servicesConf->getRestrictedCRSList(), str_crs, tmpEquilist);
                    }
                    if (!allowedCRS){
                        LOGGER_WARN ( _ ( "         Forbiden CRS " ) << str_crs  );
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
                        LOGGER_INFO ( _ ( "         Adding CRS " ) <<str_crs );
                        WMSCRSList.push_back ( crs );
                    } else {
                        LOGGER_WARN ( _ ( "         Already present CRS " ) << str_crs  );
                    }
                    std::vector<CRS> tmpEquilist = getEqualsCRS(servicesConf->getListOfEqualsCRS() , str_crs );
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
                            LOGGER_INFO ( _ ( "         Adding Equivalent CRS " ) << tmpEquilist.at( l ).getRequestCode() );
                        } else {
                            LOGGER_WARN ( _ ( "         Already present CRS " ) << tmpEquilist.at( l ).getRequestCode()  );
                        }
                    }
                }
            }
        }
    }
    if ( WMSCRSList.size() ==0 ) {
        LOGGER_INFO ( fileName <<_ ( ": Aucun CRS specifique autorise pour la couche" ) );
    }

    //DEPRECATED
    pElem=hRoot.FirstChild ( "opaque" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_DEBUG ( _ ( "Pas de opaque => opaque = " ) << DEFAULT_OPAQUE );
        opaque = DEFAULT_OPAQUE;
    } else {
        std::string opaStr= pElem->GetTextStr();
        if ( opaStr=="true" ) {
            opaque = true;
        } else if ( opaStr=="false" ) {
            opaque = false;
        } else {
            LOGGER_ERROR ( _ ( "le param opaque n'est pas exploitable:[" ) << opaStr <<"]" );
            return NULL;
        }
    }

    pElem=hRoot.FirstChild ( "authority" ).Element();
    if ( pElem && pElem->GetText() ) {
        authority= pElem->GetTextStr();
    }

    pElem=hRoot.FirstChild ( "resampling" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "Pas de resampling => resampling = " ) << DEFAULT_RESAMPLING );
        resamplingStr = DEFAULT_RESAMPLING;
    } else {
        resamplingStr = pElem->GetTextStr();
    }

    resampling = Interpolation::fromString ( resamplingStr );

    pElem=hRoot.FirstChild ( "pyramid" ).Element();
    if ( pElem && pElem->GetText() ) {

        std::string pyramidFilePath ( pElem->GetTextStr() );
        //Relative Path
        if ( pyramidFilePath.compare ( 0,2,"./" ) ==0 ) {
            pyramidFilePath.replace ( 0,1,parentDir );
        } else if ( pyramidFilePath.compare ( 0,1,"/" ) !=0 ) {
            pyramidFilePath.insert ( 0,"/" );
            pyramidFilePath.insert ( 0,parentDir );
        }
        pyramid = buildPyramid ( pyramidFilePath, tmsList, times, stylesList, proxy);
        if ( !pyramid ) {
            LOGGER_ERROR ( _ ( "La pyramide " ) << pyramidFilePath << _ ( " ne peut etre chargee" ) );
            return NULL;
        }
    } else {
        // FIXME: pas forcement critique si on a un cache d'une autre nature (jpeg2000 par exemple).
        LOGGER_ERROR ( _ ( "Aucune pyramide associee au layer " ) << fileName );
        return NULL;
    }

    //MetadataURL Elements , mandatory in INSPIRE
    for ( pElem=hRoot.FirstChild ( "MetadataURL" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "MetadataURL" ) ) {
        std::string format;
        std::string href;
        std::string type;

        if ( pElem->QueryStringAttribute ( "type",&type ) != TIXML_SUCCESS ) {
            LOGGER_ERROR ( fileName << _ ( "MetadataURL type missing" ) );
            continue;
        }

        TiXmlHandle hMetadata ( pElem );
        TiXmlElement *pElemMTD = hMetadata.FirstChild ( "Format" ).Element();
        if ( !pElemMTD || !pElemMTD->GetText() ) {
            LOGGER_ERROR ( fileName << _ ( "MetadataURL Format missing" ) );
            continue;
        }
        format = pElemMTD->GetText();
        pElemMTD = hMetadata.FirstChild ( "OnlineResource" ).Element();
        if ( !pElemMTD || pElemMTD->QueryStringAttribute ( "xlink:href",&href ) != TIXML_SUCCESS ) {
            LOGGER_ERROR ( fileName << _ ( "MetadataURL HRef missing" ) );
            continue;
        }

        metadataURLs.push_back ( MetadataURL ( format,href,type ) );
    }

    if ( metadataURLs.size() == 0 && inspire ) {
        LOGGER_ERROR ( _ ( "No MetadataURL found in the layer " ) << fileName <<_ ( " : not compatible with INSPIRE!!" ) );
        return NULL;
    }



    Layer *layer;

    layer = new Layer ( id, title, abstract, WMSauth, WMTSauth,keyWords, pyramid, styles, minRes, maxRes,
                        WMSCRSList, opaque, authority, resampling,geographicBoundingBox,boundingBox,metadataURLs,
                        getFeatureInfoAvailability, getFeatureInfoType, getFeatureInfoBaseURL, GFIVersion,
                        GFIService, GFIQueryLayers, GFILayers, GFIForceEPSG);

    //Si une pyramide est à la demande, on n'authorize pas le WMS car c'est un cas non gérer dans les processus de reponse du serveur
    if (layer->getDataPyramid()->getOnDemand()) {
        layer->setWMSAuthorized(false);
    }

    return layer;
}//buildLayer

Layer * ConfLoader::buildLayer ( std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList,std::map<std::string,Style*> stylesList, bool reprojectionCapability, ServicesConf* servicesConf, Proxy proxy ) {
    TiXmlDocument doc ( fileName.c_str() );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "Ne peut pas charger le fichier " ) << fileName );
        return NULL;
    }
    return parseLayer ( &doc,fileName,tmsList,stylesList,reprojectionCapability,servicesConf, proxy );
}

// Load the server configuration (default is server.conf file) during server initialization
bool ConfLoader::parseTechnicalParam ( TiXmlDocument* doc,std::string serverConfigFile, LogOutput& logOutput, std::string& logFilePrefix, int& logFilePeriod, LogLevel& logLevel, int& nbThread, bool& supportWMTS, bool& supportWMS, bool& reprojectionCapability, std::string& servicesConfigFile, std::string &layerDir, std::string &tmsDir, std::string &styleDir, std::string& socket, int& backlog, int& nbProcess, Proxy &proxy ) {
    TiXmlHandle hDoc ( doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        std::cerr<<serverConfigFile <<_ ( " impossible de recuperer la racine." ) <<std::endl;
        return false;
    }
    if ( strcmp ( pElem->Value(), "serverConf" ) ) {
        std::cerr<<serverConfigFile <<_ ( " La racine n'est pas un serverConf." ) <<std::endl;
        return false;
    }
    hRoot=TiXmlHandle ( pElem );

    pElem=hRoot.FirstChild ( "logOutput" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de logOutput => logOutput = " ) << DEFAULT_LOG_OUTPUT;
        logOutput = DEFAULT_LOG_OUTPUT;
    } else {
        std::string strLogOutput= ( pElem->GetTextStr() );
        if ( strLogOutput=="rolling_file" ) {
            logOutput=ROLLING_FILE;
        } else if ( strLogOutput=="standard_output_stream_for_errors" ) {
            logOutput=STANDARD_OUTPUT_STREAM_FOR_ERRORS;
        } else if ( strLogOutput=="static_file" ) {
            logOutput=STATIC_FILE;
        } else {
            std::cerr<<_ ( "Le logOutput [" ) << pElem->GetTextStr() <<_ ( "]  est inconnu." ) <<std::endl;
            return false;
        }
    }

    pElem=hRoot.FirstChild ( "logFilePrefix" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de logFilePrefix => logFilePrefix = " ) << DEFAULT_LOG_FILE_PREFIX;
        logFilePrefix = DEFAULT_LOG_FILE_PREFIX;
    } else {
        logFilePrefix= pElem->GetTextStr();
    }
    pElem=hRoot.FirstChild ( "logFilePeriod" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de logFilePeriod => logFilePeriod = " ) << DEFAULT_LOG_FILE_PERIOD;
        logFilePeriod = DEFAULT_LOG_FILE_PERIOD;
    } else if ( !sscanf ( pElem->GetText(),"%d",&logFilePeriod ) )  {
        std::cerr<<_ ( "Le logFilePeriod [" ) << pElem->GetTextStr() <<_ ( "]  is not an integer." ) <<std::endl;
        return false;
    }

    pElem=hRoot.FirstChild ( "logLevel" ).Element();

    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de logLevel => logLevel = " ) << DEFAULT_LOG_LEVEL;
        logLevel = DEFAULT_LOG_LEVEL;
    } else {
        std::string strLogLevel ( pElem->GetText() );
        if ( strLogLevel=="fatal" ) logLevel=FATAL;
        else if ( strLogLevel=="error" ) logLevel=ERROR;
        else if ( strLogLevel=="warn" ) logLevel=WARN;
        else if ( strLogLevel=="info" ) logLevel=INFO;
        else if ( strLogLevel=="debug" ) logLevel=DEBUG;
        else {
            std::cerr<<_ ( "Le logLevel [" ) << pElem->GetTextStr() <<_ ( "]  est inconnu." ) <<std::endl;
            return false;
        }
    }

    pElem=hRoot.FirstChild ( "nbThread" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de nbThread => nbThread = " ) << DEFAULT_NB_THREAD<<std::endl;
        nbThread = DEFAULT_NB_THREAD;
    } else if ( !sscanf ( pElem->GetText(),"%d",&nbThread ) ) {
        std::cerr<<_ ( "Le nbThread [" ) << pElem->GetTextStr() <<_ ( "] is not an integer." ) <<std::endl;
        return false;
    }

    pElem=hRoot.FirstChild ( "nbProcess" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de nbProcess=> nbProcess = " ) << DEFAULT_NB_PROCESS<<std::endl;
        nbProcess = DEFAULT_NB_PROCESS;
    } else if ( !sscanf ( pElem->GetText(),"%d",&nbProcess ) ) {
        std::cerr<<_ ( "Le nbProcess [" ) << pElem->GetTextStr() <<_ ( "] is not an integer." ) <<std::endl;
        std::cerr<<_ ( "=> nbProcess = " ) << DEFAULT_NB_PROCESS<<std::endl;
        nbProcess = DEFAULT_NB_PROCESS;
    }
    if (nbProcess > MAX_NB_PROCESS) {
        std::cerr<<_ ( "Le nbProcess [" ) << pElem->GetTextStr() <<_ ( "] is bigger than " ) << MAX_NB_PROCESS <<std::endl;
        std::cerr<<_ ( "=> nbProcess = " ) << MAX_NB_PROCESS<<std::endl;
        nbProcess = MAX_NB_PROCESS;
    }

    pElem=hRoot.FirstChild ( "WMTSSupport" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de WMTSSupport => supportWMTS = true" ) <<std::endl;
        supportWMTS = true;
    } else {
        std::string strReprojection ( pElem->GetText() );
        if ( strReprojection=="true" ) supportWMTS=true;
        else if ( strReprojection=="false" ) supportWMTS=false;
        else {
            std::cerr<<_ ( "Le WMTSSupport [" ) << pElem->GetTextStr() <<_ ( "] n'est pas un booleen." ) <<std::endl;
            return false;
        }
    }

    pElem=hRoot.FirstChild ( "WMSSupport" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de WMSSupport => supportWMS = true" ) <<std::endl;
        supportWMS = true;
    } else {
        std::string strReprojection ( pElem->GetText() );
        if ( strReprojection=="true" ) supportWMS=true;
        else if ( strReprojection=="false" ) supportWMS=false;
        else {
            std::cerr<<_ ( "Le WMSSupport [" ) << pElem->GetTextStr() <<_ ( "] n'est pas un booleen." ) <<std::endl;
            return false;
        }
    }

    pElem=hRoot.FirstChild ( "proxy" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        proxy.proxyName = "";
    } else {
        proxy.proxyName = pElem->GetTextStr();
    }

    pElem=hRoot.FirstChild ( "noProxy" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        proxy.noProxy = "";
    } else {
        proxy.noProxy = pElem->GetTextStr();
    }

    if ( !supportWMS && !supportWMTS ) {
        std::cerr<<_ ( "WMTS et WMS desactives, extinction du serveur" ) <<std::endl;
        return false;
    }

    if ( supportWMS ) {
        pElem=hRoot.FirstChild ( "reprojectionCapability" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            std::cerr<<_ ( "Pas de reprojectionCapability => reprojectionCapability = true" ) <<std::endl;
            reprojectionCapability = true;
        } else {
            std::string strReprojection ( pElem->GetText() );
            if ( strReprojection=="true" ) reprojectionCapability=true;
            else if ( strReprojection=="false" ) reprojectionCapability=false;
            else {
                std::cerr<<_ ( "Le reprojectionCapability [" ) << pElem->GetTextStr() <<_ ( "] n'est pas un booleen." ) <<std::endl;
                return false;
            }
        }
    } else {
        reprojectionCapability = false;
    }

    pElem=hRoot.FirstChild ( "servicesConfigFile" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de servicesConfigFile => servicesConfigFile = " ) << DEFAULT_SERVICES_CONF_PATH <<std::endl;
        servicesConfigFile = DEFAULT_SERVICES_CONF_PATH;
    } else {
        servicesConfigFile= pElem->GetTextStr();
    }

    pElem=hRoot.FirstChild ( "layerDir" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de layerDir => layerDir = " ) << DEFAULT_LAYER_DIR<<std::endl;
        layerDir = DEFAULT_LAYER_DIR;
    } else {
        layerDir= pElem->GetTextStr();
    }

    pElem=hRoot.FirstChild ( "tileMatrixSetDir" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de tileMatrixSetDir => tileMatrixSetDir = " ) << DEFAULT_TMS_DIR<<std::endl;
        tmsDir = DEFAULT_TMS_DIR;
    } else {
        tmsDir= pElem->GetTextStr();
    }

    pElem=hRoot.FirstChild ( "styleDir" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de styleDir => styleDir = " ) << DEFAULT_STYLE_DIR<<std::endl;
        styleDir = DEFAULT_STYLE_DIR;
    } else {
        styleDir = pElem->GetTextStr();
    }
    // Definition de la variable PROJ_LIB à partir de la configuration
    std::string projDir;

    bool absolut=true;
    pElem=hRoot.FirstChild ( "projConfigDir" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas de projConfigDir => projConfigDir = " ) << DEFAULT_PROJ_DIR<<std::endl;
        char* pwdBuff = ( char* ) malloc ( PATH_MAX );
        getcwd ( pwdBuff,PATH_MAX );
        projDir = std::string ( pwdBuff );
        projDir.append ( "/" ).append ( DEFAULT_PROJ_DIR );
        free ( pwdBuff );
    } else {
        projDir= pElem->GetTextStr();
        //Gestion des chemins relatif
        if ( projDir.compare ( 0,1,"/" ) != 0 ) {
            absolut=false;
            char* pwdBuff = ( char* ) malloc ( PATH_MAX );
            getcwd ( pwdBuff,PATH_MAX );
            std::string pwdBuffStr = std::string ( pwdBuff );
            pwdBuffStr.append ( "/" );
            projDir.insert ( 0,pwdBuffStr );
            free ( pwdBuff );
        }
    }

    if ( setenv ( "PROJ_LIB",projDir.c_str(),1 ) !=0 ) {
        std::cerr<<_ ( "ERREUR FATALE : Impossible de definir le chemin pour proj " ) << projDir<<std::endl;
        return false;
    }
    std::clog << _ ( "Env : PROJ_LIB = " ) << getenv ( "PROJ_LIB" ) << std::endl;


    pElem=hRoot.FirstChild ( "serverPort" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::cerr<<_ ( "Pas d'element <serverPort> fonctionnement autonome impossible" ) <<std::endl;
        socket = "";
    } else {
        std::cerr<<_ ( "Element <serverPort> : Lancement interne impossible (Apache, spawn-fcgi)" ) <<std::endl;
        socket = pElem->GetTextStr();
    }

    pElem=hRoot.FirstChild ( "serverBackLog" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        std::clog<<_ ( "Pas d'element <serverBackLog> valeur par defaut : 0" ) <<std::endl;
        backlog = 0;
    } else if ( !sscanf ( pElem->GetText(),"%d",&backlog ) )  {
        std::cerr<<_ ( "Le logFilePeriod [" ) << pElem->GetTextStr() <<_ ( "]  is not an integer." ) <<std::endl;
        backlog = 0;
    }

    return true;
}//parseTechnicalParam

// Load the services configuration (default is services.conf file) during server initialization
ServicesConf * ConfLoader::parseServicesConf ( TiXmlDocument* doc,std::string servicesConfigFile ) {
    TiXmlHandle hDoc ( doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( " impossible de recuperer la racine." ) );
        return NULL;
    }
    if ( pElem->ValueStr() != "servicesConf" ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( " La racine n'est pas un servicesConf." ) );
        return NULL;
    }
    hRoot=TiXmlHandle ( pElem );

    std::string name="";
    std::string title="";
    std::string abstract="";
    std::vector<Keyword> keyWords;
    std::string serviceProvider="";
    std::string fee="";
    std::string accessConstraint="";
    unsigned int layerLimit;
    unsigned int maxWidth;
    unsigned int maxHeight;
    unsigned int maxTileX;
    unsigned int maxTileY;
    bool postMode = false;
    //Contact Info
    std::string providerSite="";
    std::string individualName="";
    std::string individualPosition="";
    std::string voice="";
    std::string facsimile="";
    std::string addressType="";
    std::string deliveryPoint="";
    std::string city="";
    std::string administrativeArea="";
    std::string postCode="";
    std::string country="";
    std::string electronicMailAddress="";
    //WMS
    std::vector<std::string> formatList;
    std::vector<CRS> globalCRSList;
    bool fullStyling = false;
    //WMTS
    std::string serviceType="";
    std::string serviceTypeVersion="";
    //INSPIRE
    bool inspire = false;
    std::vector<std::string> applicationProfileList;
    std::string metadataUrlWMS;
    std::string metadataMediaTypeWMS;
    std::string metadataUrlWMTS;
    std::string metadataMediaTypeWMTS;
    // CRS
    bool doweuselistofequalsCRS = false; // Option to avoid reprojecting data in equivalent CRS
    bool addEqualsCRS = false; // Option to take in count equivalents CRS of a CRS
    bool dowerestrictCRSList = false; // limits proj4 CRS list
    std::vector<std::string> listofequalsCRS; // If the option is true, load the list of equals CRS
    std::vector<std::string> restrictedCRSList;

    pElem=hRoot.FirstChild ( "name" ).Element();
    if ( pElem && pElem->GetText() ) name = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "title" ).Element();
    if ( pElem && pElem->GetText() ) title = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "abstract" ).Element();
    if ( pElem && pElem->GetText() ) abstract = pElem->GetTextStr();


    for ( pElem=hRoot.FirstChild ( "keywordList" ).FirstChild ( "keyword" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "keyword" ) ) {
        if ( ! ( pElem->GetText() ) )
            continue;
        std::map<std::string,std::string> attributes;
        TiXmlAttribute* attrib = pElem->FirstAttribute();
        while ( attrib ) {
            attributes.insert ( attribute ( attrib->NameTStr(),attrib->ValueStr() ) );
            attrib = attrib->Next();
        }
        keyWords.push_back ( Keyword ( pElem->GetTextStr(),attributes ) );
    }

    pElem=hRoot.FirstChild ( "postMode" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string postStr = pElem->GetTextStr();
        if ( postStr.compare ( "true" ) ==0 || postStr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Requete POST autorisee" ) );
            postMode = true;
        }
    }

    pElem=hRoot.FirstChild ( "serviceProvider" ).Element();
    if ( pElem && pElem->GetText() ) serviceProvider = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "providerSite" ).Element();
    if ( pElem && pElem->GetText() )
        providerSite = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "fee" ).Element();
    if ( pElem && pElem->GetText() ) fee = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "accessConstraint" ).Element();
    if ( pElem && pElem->GetText() ) accessConstraint = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "individualName" ).Element();
    if ( pElem && pElem->GetText() )
        individualName= pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "individualPosition" ).Element();
    if ( pElem && pElem->GetText() )
        individualPosition= pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "voice" ).Element();
    if ( pElem && pElem->GetText() )
        voice= pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "facsimile" ).Element();
    if ( pElem && pElem->GetText() )
        facsimile= pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "addressType" ).Element();
    if ( pElem && pElem->GetText() )
        addressType = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "deliveryPoint" ).Element();
    if ( pElem && pElem->GetText() )
        deliveryPoint= pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "city" ).Element();
    if ( pElem && pElem->GetText() )
        city = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "administrativeArea" ).Element();
    if ( pElem && pElem->GetText() )
        administrativeArea = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "postCode" ).Element();
    if ( pElem && pElem->GetText() )
        postCode = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "country" ).Element();
    if ( pElem && pElem->GetText() )
        country = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "electronicMailAddress" ).Element();
    if ( pElem && pElem->GetText() )
        electronicMailAddress= pElem->GetTextStr();

    pElem = hRoot.FirstChild ( "layerLimit" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        layerLimit=DEFAULT_LAYER_LIMIT;
    } else if ( !sscanf ( pElem->GetText(),"%d",&layerLimit ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le layerLimit est inexploitable:[" ) << pElem->GetTextStr() << "]" );
        return NULL;
    }

    pElem = hRoot.FirstChild ( "maxWidth" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        maxWidth=MAX_IMAGE_WIDTH;
    } else if ( !sscanf ( pElem->GetText(),"%d",&maxWidth ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le maxWidth est inexploitable:[" ) << pElem->GetTextStr() << "]" );
        return NULL;
    }

    pElem = hRoot.FirstChild ( "maxHeight" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        maxHeight=MAX_IMAGE_HEIGHT;
    } else if ( !sscanf ( pElem->GetText(),"%d",&maxHeight ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le maxHeight est inexploitable:[" ) << pElem->GetTextStr() << "]" );
        return NULL;
    }

    pElem = hRoot.FirstChild ( "maxTileX" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        maxTileX=MAX_TILE_X;
    } else if ( !sscanf ( pElem->GetText(),"%d",&maxTileX ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le maxTileX est inexploitable:[" ) << pElem->GetTextStr() << "]" );
        return NULL;
    }

    pElem = hRoot.FirstChild ( "maxTileY" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        maxTileY=MAX_TILE_Y;
    } else if ( !sscanf ( pElem->GetText(),"%d",&maxTileY ) ) {
        LOGGER_ERROR ( servicesConfigFile << _ ( "Le maxTileY est inexploitable:[" ) << pElem->GetTextStr() << "]" );
        return NULL;
    }

    for ( pElem=hRoot.FirstChild ( "formatList" ).FirstChild ( "format" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "format" ) ) {
        if ( ! ( pElem->GetText() ) )
            continue;
        std::string format ( pElem->GetText() );
        if ( format != "image/jpeg" &&
                format != "image/png"  &&
                format != "image/tiff" &&
                format != "image/geotiff" &&
                format != "image/x-bil;bits=32" &&
                format != "image/gif" ) {
            LOGGER_ERROR ( servicesConfigFile << _ ( "le format d'image [" ) << format << _ ( "] n'est pas un type MIME" ) );
        } else {
            formatList.push_back ( format );
        }
    }
    
    pElem=hRoot.FirstChild ( "avoidEqualsCRSReprojection" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string doweuselistofequalsCRSstr = pElem->GetTextStr();
        if ( doweuselistofequalsCRSstr.compare ( "true" ) ==0 || doweuselistofequalsCRSstr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Pas de reprojection pour les CRS equivalents" ) );
            doweuselistofequalsCRS = true;
            listofequalsCRS=loadListEqualsCRS();
        }
    }
    
    pElem=hRoot.FirstChild ( "addEqualsCRS" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string addEqualsCRSstr = pElem->GetTextStr();
        if ( addEqualsCRSstr.compare ( "true" ) ==0 || addEqualsCRSstr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Ajout automatique des CRS equivalents" ) );
            addEqualsCRS = true;
            if (!doweuselistofequalsCRS){
                listofequalsCRS=loadListEqualsCRS();
            }
            
        }
    }
    
    pElem=hRoot.FirstChild ( "restrictedCRSList" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string restritedCRSListfile = pElem->GetTextStr();
        if ( restritedCRSListfile.compare ( "" ) != 0 )  {
            LOGGER_INFO ( _ ( "Liste restreinte de CRS à partir du fichier " ) << restritedCRSListfile );
            dowerestrictCRSList = true;
            restrictedCRSList = loadStringVectorFromFile(restritedCRSListfile);
            
        }
        
    }

    //Global CRS List
    for ( pElem=hRoot.FirstChild ( "globalCRSList" ).FirstChild ( "crs" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "crs" ) ) {
        if ( ! ( pElem->GetText() ) )
            continue;
        std::string crsStr ( pElem->GetTextStr() );
        CRS crs ( crsStr );
        if ( !crs.isProj4Compatible() ) {
            LOGGER_ERROR ( servicesConfigFile << _ ( "The CRS [" ) << crsStr << _ ( "] is not present in Proj4" ) );
        } else {
            std::vector<CRS> tmpEquilist;
            if (addEqualsCRS){
               tmpEquilist = getEqualsCRS(listofequalsCRS, crsStr );
            }
            bool allowedCRS = true;
            if ( dowerestrictCRSList ){
                allowedCRS = isCRSAllowed(restrictedCRSList, crsStr, tmpEquilist);
            }
            if (allowedCRS) {
                bool found = false;
                for ( int i = 0; i<globalCRSList.size() ; i++ ){
                    if (globalCRSList.at( i ).getRequestCode().compare( crs.getRequestCode() ) == 0 ){
                        found = true;
                        break;
                    }
                }
                if (!found){
                    globalCRSList.push_back ( crs );
                    LOGGER_INFO ( _ ( "Adding global CRS " ) << crsStr  );
                } else {
                    LOGGER_WARN ( _ ( "Already present in global CRS list " ) << crsStr  );
                }
                for (unsigned int l = 0; l< tmpEquilist.size();l++){
                    found = false;
                    for ( int i = 0; i<globalCRSList.size() ; i++ ){
                        if ( globalCRSList.at( i ) == tmpEquilist.at( l ) ){
                            found = true;
                            break;
                        }
                    }
                    if (!found){
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
            std::vector<CRS> tmpEquilist = getEqualsCRS(listofequalsCRS, "CRS:84" );
            for (unsigned int l = 0; l< tmpEquilist.size();l++){
                globalCRSList.push_back( tmpEquilist.at( l ) );
                LOGGER_INFO ( _ ( "Adding equivalent global CRS [" ) << tmpEquilist.at( l ).getRequestCode() << _ ( "] of [CRS:84]") );
            }
        }
    }

    pElem=hRoot.FirstChild ( "fullStylingCapability" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string styleStr = pElem->GetTextStr();
        if ( styleStr.compare ( "true" ) ==0 || styleStr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Utilisation des styles pour tous les formats" ) );
            fullStyling = true;
        }
    }


    pElem=hRoot.FirstChild ( "serviceType" ).Element();
    if ( pElem && pElem->GetText() ) serviceType = pElem->GetTextStr();
    pElem=hRoot.FirstChild ( "serviceTypeVersion" ).Element();
    if ( pElem && pElem->GetText() ) serviceTypeVersion = pElem->GetTextStr();

    pElem=hRoot.FirstChild ( "inspire" ).Element();
    if ( pElem && pElem->GetText() ) {
        std::string inspirestr = pElem->GetTextStr();
        if ( inspirestr.compare ( "true" ) ==0 || inspirestr.compare ( "1" ) ==0 ) {
            LOGGER_INFO ( _ ( "Utilisation du mode Inspire" ) );
            inspire = true;
        }
    }

    pElem=hRoot.FirstChild ( "metadataWMS" ).Element();
    if ( pElem ) {
        pElem = pElem->FirstChildElement ( "url" );
        if ( pElem &&  pElem->GetText() ) {
            metadataUrlWMS = pElem->GetTextStr();
            pElem = pElem->NextSiblingElement ( "mediaType" );
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return NULL;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
        if ( pElem &&  pElem->GetText() ) {
            metadataMediaTypeWMS = pElem->GetTextStr();
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return NULL;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
    }

    pElem=hRoot.FirstChild ( "metadataWMTS" ).Element();
    if ( pElem ) {
        pElem = pElem->FirstChildElement ( "url" );
        if ( pElem &&  pElem->GetText() ) {
            metadataUrlWMTS = pElem->GetTextStr();
            pElem = pElem->NextSiblingElement ( "mediaType" );
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return NULL;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
        if ( pElem &&  pElem->GetText() ) {
            metadataMediaTypeWMTS = pElem->GetTextStr();
        } else {
            if ( inspire ) {
                LOGGER_ERROR ( _ ( "Metadata element incorrect" ) );
                return NULL;
            } else {
                LOGGER_INFO ( _ ( "Metadata element incorrect" ) );
            }
        }
    }

    

    

    MetadataURL mtdMWS = MetadataURL ( "simple",metadataUrlWMS,metadataMediaTypeWMS );
    MetadataURL mtdWMTS = MetadataURL ( "simple",metadataUrlWMTS,metadataMediaTypeWMTS );
    ServicesConf * servicesConf;
    servicesConf = new ServicesConf ( name, title, abstract, keyWords,serviceProvider, fee,
                                      accessConstraint, layerLimit, maxWidth, maxHeight, maxTileX, maxTileY, formatList, globalCRSList , serviceType, serviceTypeVersion,
                                      providerSite, individualName, individualPosition, voice, facsimile,
                                      addressType, deliveryPoint, city, administrativeArea, postCode, country,
                                      electronicMailAddress, mtdMWS, mtdWMTS, listofequalsCRS, restrictedCRSList, postMode, fullStyling, inspire, doweuselistofequalsCRS, addEqualsCRS, dowerestrictCRSList);
    return servicesConf;
}

std::vector<std::string> ConfLoader::loadListEqualsCRS(){
    // Build the list (vector of string) of equals CRS from a file given in parameter
    char * fileCRS = "/listofequalscrs.txt";
    char * dirCRS = getenv ( "PROJ_LIB" ); // Get path from config
    char namebuffer[100];
    strcpy(namebuffer, dirCRS);
    strcat(namebuffer, fileCRS);
    LOGGER_INFO ( _ ( "Construction de la liste des CRS equivalents depuis " ) << namebuffer );
    std::vector<std::string> rawStrVector = loadStringVectorFromFile(std::string(namebuffer));
    std::vector<std::string> strVector;
    // rawStrVector can cointains some unknowned CRS => filtering using Proj4
    for ( unsigned int l=0; l<rawStrVector.size(); l++ ){
        std::string line = rawStrVector.at( l );
        std::string crsstr = "";
        std::string targetLine;
        //split
        size_t start_index = 0;
        size_t len = 0;
        size_t found_space = 0;
        while ( found_space != std::string::npos ){
            found_space = line.find(" ", start_index );
            if ( found_space == std::string::npos ) {
                len = line.size() - start_index ; //-1 pour le retour chariot
            } else {
                len = found_space - start_index;
            }
            crsstr = line.substr( start_index, len );
            
            //is the new CRS compatible with Proj4 ?
            CRS crs ( crsstr );
            if ( !crs.isProj4Compatible() ) {
                LOGGER_WARN ( _ ( "The Equivalent CRS [" ) << crsstr << _ ( "] is not present in Proj4" ) );
            } else {
                targetLine.append( crsstr );
                targetLine.append( " " );
            }
            
            start_index = found_space + 1;
        }
        if (targetLine.length() != 0) {
           strVector.push_back( targetLine.substr(0, targetLine.length()) );
        }
    }
    return strVector;
}


std::vector<std::string> ConfLoader::loadStringVectorFromFile(std::string file){
    std::vector<std::string> strVector;
    std::ifstream input ( file.c_str() );
    // We test if the stream is empty
    //   This can happen when the file can't be loaded or when the file is empty
    if ( input.peek() == std::ifstream::traits_type::eof() ) {
        LOGGER_ERROR ( _ ("Ne peut pas charger le fichier ") << file << _ (" ou fichier vide")  );
    }
    
    for( std::string line; getline(input, line); ) {
        if (line[0] != '#' ){
            strVector.push_back( line );
        }
    }
    return strVector;
}

std::vector<CRS> ConfLoader::getEqualsCRS(std::vector<std::string> listofequalsCRS, std::string basecrs)
{
    std::vector<CRS> returnCRS;
    for ( unsigned int l=0; l<listofequalsCRS.size(); l++ ){
        std::string workingbasecrs(basecrs);
        workingbasecrs.append(" ");
        size_t found = listofequalsCRS.at( l ).find ( workingbasecrs );
        if (found == std::string::npos){
            size_t found = listofequalsCRS.at( l ).find ( basecrs );
            if ( found != ( listofequalsCRS.at( l ).size() - basecrs.size()) ){
                found = std::string::npos;
            }
        }
        if (found != std::string::npos) {
            //Global CRS found !
            std::string line = listofequalsCRS.at( l );
            std::string crsstr = "";
            //split
            size_t start_index = 0;
            size_t len = 0;
            size_t found_space = 0;
            while ( found_space != std::string::npos ){
                found_space = line.find(" ", start_index );
                if ( found_space == std::string::npos ) {
                    len = line.size() - start_index ; //-1 pour le retour chariot
                } else {
                    len = found_space - start_index;
                }
                crsstr = line.substr( start_index, len );
                
                if ( crsstr.compare(basecrs) != 0 ){
                    //is the new CRS compatible with Proj4 ?
                    CRS crs ( crsstr );
                    if ( !crs.isProj4Compatible() ) {
                        LOGGER_DEBUG ( _ ( "The Equivalent CRS [" ) << crsstr << _ ( "] of [" ) << basecrs << _ ( "] is not present in Proj4" ) );
                    } else {
                        returnCRS.push_back( crs );
                    }
                }
                start_index = found_space + 1;
            }
        }
    }
    return returnCRS;
}

bool ConfLoader::isCRSAllowed(std::vector<std::string> restrictedCRSList, std::string crs, std::vector<CRS> equiCRSList){
    bool allowedCRS = false;
    //Is the CRS allowed ?
    for (unsigned int l = 0 ; l<restrictedCRSList.size() ; l++){
        if ( crs.compare( restrictedCRSList.at( l ) ) == 0 ){
            allowedCRS = true;
            break;
        }
    }
    if (allowedCRS){
        return true;
    }
    //Is an equivalent of this CRS allowed ?
    for (unsigned int k = 0 ; k < equiCRSList.size(); k++ ){
        std::string equicrsstr = equiCRSList.at( k ).getRequestCode();
        for (unsigned int l = 0 ; l<restrictedCRSList.size() ; l++){
            if ( equicrsstr.compare( restrictedCRSList.at( l ) ) == 0 ){
                allowedCRS = true;
                break;
            }
        }
        if (allowedCRS){
            break;
        }
    }
    return allowedCRS;
}


bool ConfLoader::getTechnicalParam ( std::string serverConfigFile, LogOutput& logOutput, std::string& logFilePrefix,
                                     int& logFilePeriod, LogLevel& logLevel, int& nbThread, bool& supportWMTS, bool& supportWMS,
                                     bool& reprojectionCapability, std::string& servicesConfigFile, std::string &layerDir,
                                     std::string &tmsDir, std::string &styleDir, std::string& socket, int& backlog, int& nbProcess, Proxy &proxy ) {
    std::cout<<_ ( "Chargement des parametres techniques depuis " ) <<serverConfigFile<<std::endl;
    TiXmlDocument doc ( serverConfigFile );
    if ( !doc.LoadFile() ) {
        std::cerr<<_ ( "Ne peut pas charger le fichier " ) << serverConfigFile<<std::endl;
        return false;
    }
    return parseTechnicalParam ( &doc,serverConfigFile,logOutput,logFilePrefix,logFilePeriod,logLevel,nbThread,supportWMTS,supportWMS,reprojectionCapability,servicesConfigFile,layerDir,tmsDir,styleDir, socket, backlog, nbProcess, proxy );
}

bool ConfLoader::buildStylesList ( std::string styleDir, std::map< std::string, Style* >& stylesList, bool inspire ) {
    LOGGER_INFO ( _ ( "CHARGEMENT DES STYLES" ) );

    // lister les fichier du repertoire styleDir
    std::vector<std::string> styleFiles;
    std::vector<std::string> styleName;
    std::string styleFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( styleDir.c_str() ) ) == NULL ) {
        LOGGER_FATAL ( _ ( "Le repertoire des Styles " ) << styleDir << _ ( " n'est pas accessible." ) );
        return false;
    }
    while ( ( fileEntry = readdir ( dir ) ) ) {
        styleFileName = fileEntry->d_name;
        if ( styleFileName.rfind ( ".stl" ) ==styleFileName.size()-4 ) {
            styleFiles.push_back ( styleDir+"/"+styleFileName );
            styleName.push_back ( styleFileName.substr ( 0,styleFileName.size()-4 ) );
        }
    }
    closedir ( dir );

    if ( styleFiles.empty() ) {
        // FIXME:
        // Aucun Style presents.
        LOGGER_FATAL ( _ ( "Aucun fichier *.stl dans le repertoire " ) << styleDir );
        return false;
    }

    // generer les styles decrits par les fichiers.
    for ( unsigned int i=0; i<styleFiles.size(); i++ ) {
        Style * style;
        style = buildStyle ( styleFiles[i],inspire );
        if ( style ) {
            stylesList.insert ( std::pair<std::string, Style *> ( styleName[i], style ) );
        } else {
            LOGGER_ERROR ( _ ( "Ne peut charger le style: " ) << styleFiles[i] );
        }
    }

    if ( stylesList.size() ==0 ) {
        LOGGER_FATAL ( _ ( "Aucun Style n'a pu etre charge!" ) );
        return false;
    }

    LOGGER_INFO ( _ ( "NOMBRE DE STYLES CHARGES : " ) <<stylesList.size() );

    return true;
}

bool ConfLoader::buildTMSList ( std::string tmsDir,std::map<std::string, TileMatrixSet*> &tmsList ) {
    LOGGER_INFO ( _ ( "CHARGEMENT DES TMS" ) );

    // lister les fichier du repertoire tmsDir
    std::vector<std::string> tmsFiles;
    std::string tmsFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( tmsDir.c_str() ) ) == NULL ) {
        LOGGER_FATAL ( _ ( "Le repertoire des TMS " ) << tmsDir << _ ( " n'est pas accessible." ) );
        return false;
    }
    while ( ( fileEntry = readdir ( dir ) ) ) {
        tmsFileName = fileEntry->d_name;
        if ( tmsFileName.rfind ( ".tms" ) ==tmsFileName.size()-4 ) {
            tmsFiles.push_back ( tmsDir+"/"+tmsFileName );
        }
    }
    closedir ( dir );

    if ( tmsFiles.empty() ) {
        // FIXME:
        // Aucun TMS presents. Ce n'est pas necessairement grave si le serveur
        // ne sert pas pour le WMTS et qu'on exploite pas de cache tuile.
        // Cependant pour le moment (07/2010) on ne gere que des caches tuiles
        LOGGER_FATAL ( _ ( "Aucun fichier *.tms dans le repertoire " ) << tmsDir );
        return false;
    }

    // generer les TMS decrits par les fichiers.
    for ( unsigned int i=0; i<tmsFiles.size(); i++ ) {
        TileMatrixSet * tms;
        tms = buildTileMatrixSet ( tmsFiles[i] );
        if ( tms ) {
            tmsList.insert ( std::pair<std::string, TileMatrixSet *> ( tms->getId(), tms ) );
        } else {
            LOGGER_ERROR ( _ ( "Ne peut charger le tms: " ) << tmsFiles[i] );
        }
    }

    if ( tmsList.size() ==0 ) {
        LOGGER_FATAL ( _ ( "Aucun TMS n'a pu etre charge!" ) );
        return false;
    }

    LOGGER_INFO ( _ ( "NOMBRE DE TMS CHARGES : " ) <<tmsList.size() );

    return true;
}

bool ConfLoader::buildLayersList ( std::string layerDir, std::map< std::string, TileMatrixSet* >& tmsList, std::map< std::string, Style* >& stylesList, std::map< std::string, Layer* >& layers, bool reprojectionCapability, ServicesConf* servicesConf, Proxy proxy ) {
    LOGGER_INFO ( _ ( "CHARGEMENT DES LAYERS" ) );
    // lister les fichier du repertoire layerDir
    std::vector<std::string> layerFiles;
    std::string layerFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ( ( dir = opendir ( layerDir.c_str() ) ) == NULL ) {
        LOGGER_FATAL ( _ ( "Le repertoire " ) << layerDir << _ ( " n'est pas accessible." ) );
        return false;
    }
    while ( ( fileEntry = readdir ( dir ) ) ) {
        layerFileName = fileEntry->d_name;
        if ( layerFileName.rfind ( ".lay" ) ==layerFileName.size()-4 ) {
            layerFiles.push_back ( layerDir+"/"+layerFileName );
        }
    }
    closedir ( dir );

    if ( layerFiles.empty() ) {
        LOGGER_ERROR ( _ ( "Aucun fichier *.lay dans le repertoire " ) << layerDir );
        LOGGER_ERROR ( _ ( "Le serveur n'a aucune donnees à servir. Dommage..." ) );
        //return false;
    }

    // generer les Layers decrits par les fichiers.
    for ( unsigned int i=0; i<layerFiles.size(); i++ ) {
        Layer * layer;
        layer = buildLayer ( layerFiles[i], tmsList, stylesList , reprojectionCapability, servicesConf, proxy );
        if ( layer ) {
            layers.insert ( std::pair<std::string, Layer *> ( layer->getId(), layer ) );
        } else {
            LOGGER_ERROR ( _ ( "Ne peut charger le layer: " ) << layerFiles[i] );
        }
    }

    if ( layers.size() ==0 ) {
        LOGGER_ERROR ( _ ( "Aucun layer n'a pu etre charge!" ) );
        //return false;
    }

    LOGGER_INFO ( _ ( "NOMBRE DE LAYERS CHARGES : " ) <<layers.size() );
    return true;
}

ServicesConf * ConfLoader::buildServicesConf ( std::string servicesConfigFile ) {
    LOGGER_INFO ( _ ( "Construction de la configuration des services depuis " ) <<servicesConfigFile );
    TiXmlDocument doc ( servicesConfigFile );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "Ne peut pas charger le fichier " ) << servicesConfigFile );
        return NULL;
    }
    return parseServicesConf ( &doc,servicesConfigFile );
}

