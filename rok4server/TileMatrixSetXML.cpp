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

#include "TileMatrixSetXML.h"
#include "TileMatrixXML.h"
#include "Request.h"

TileMatrixSetXML::TileMatrixSetXML(std::string path ) : DocumentXML(path)
{
    ok = false;

    TiXmlDocument doc ( filePath.c_str() );
    if ( ! doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "                Ne peut pas charger le fichier " ) << filePath );
        return;
    }

    TiXmlHandle hDoc ( &doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( filePath << _ ( "            Impossible de recuperer la racine." ) );
        return;
    }
    if ( strcmp ( pElem->Value(),"tileMatrixSet" ) ) {
        LOGGER_ERROR ( filePath << _ ( "            La racine n'est pas un tileMatrixSet." ) );
        return;
    }
    hRoot=TiXmlHandle ( pElem );

    // Récupération de l'ID du TMS = nom du fichier sans extension

    unsigned int idBegin=filePath.rfind ( "/" );
    if ( idBegin == std::string::npos ) {
        idBegin=0;
    }
    unsigned int idEnd=filePath.rfind ( ".tms" );
    if ( idEnd == std::string::npos ) {
        idEnd=filePath.rfind ( ".TMS" );
        if ( idEnd == std::string::npos ) {
            idEnd=filePath.size();
        }
    }
    id = filePath.substr ( idBegin+1, idEnd-idBegin-1 );

    if ( Request::containForbiddenChars(id) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( " : l'identifiant du TMS contient des caracteres interdits" ) );
        return;
    }

    // Récupération du CRS

    pElem=hRoot.FirstChild ( "crs" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << id <<_ ( " pas de crs!!" ) );
        return;
    }
    crs = CRS( DocumentXML::getTextStrFromElem(pElem) );

    // Récupération du titre

    pElem=hRoot.FirstChild ( "title" ).Element();
    if ( pElem && pElem->GetText() ) {
        title = DocumentXML::getTextStrFromElem(pElem);
    }

    // Récupération du résumé

    pElem=hRoot.FirstChild ( "abstract" ).Element();
    if ( pElem && pElem->GetText() ) {
        abstract = DocumentXML::getTextStrFromElem(pElem);
    }

    // Récupération des mots clés

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


    for ( pElem=hRoot.FirstChild ( "tileMatrix" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "tileMatrix" ) ) {
        TileMatrixXML tmXML(id, filePath, pElem);
        if (! tmXML.isOk()) {
            return;
        }

        std::string tmId = tmXML.getId();
        TileMatrix* tmObj = new TileMatrix(tmXML);
        listTM.insert ( std::pair<std::string, TileMatrix*> ( tmId, tmObj ) );
    }

    if ( listTM.size() == 0 ) {
        LOGGER_ERROR ( _ ( "Aucun tileMatrix trouve dans le tileMatrixSet" ) << id <<_ ( " : il est invalide!!" ) );
        return;
    }

    ok = true;
}

TileMatrixSetXML::~TileMatrixSetXML(){

    if (! ok) {
        // Ce TMS n'est pas valide, donc n'a pas été utilisé pour créer un objet TileMatrixSet. Il faut donc nettoyer tout ce qui a été créé.
        std::map<std::string, TileMatrix*>::iterator itTM;
        for ( itTM=listTM.begin(); itTM != listTM.end(); itTM++ )
            delete itTM->second;

    }

}

std::string TileMatrixSetXML::getId() { return id; }

bool TileMatrixSetXML::isOk() { return ok; }
