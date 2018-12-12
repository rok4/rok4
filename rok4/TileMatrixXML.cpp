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

#include "TileMatrixXML.h"
#include "Request.h"


TileMatrixXML::TileMatrixXML(std::string tmsId, std::string path, TiXmlElement* levelElement) : DocumentXML(path)
{
    ok = false;

    TiXmlHandle hTM ( levelElement );
    TiXmlElement* pElemTM = hTM.FirstChild ( "id" ).Element();
    if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId << ", TileMatrix sans id!!" );
        return;
    }
    id = std::string(pElemTM->GetText());

    if ( Request::containForbiddenChars(id) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", un TileMatrix contient des caracteres interdits" ) );
        return;
    }

    pElemTM = hTM.FirstChild ( "resolution" ).Element();
    if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( " sans resolution!!" ) );
        return;
    }
    if ( !sscanf ( pElemTM->GetText(),"%lf",&res ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( ": La resolution est inexploitable." ) );
        return;
    }

    pElemTM = hTM.FirstChild ( "topLeftCornerX" ).Element();
    if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( " sans topLeftCornerX!!" ) );
        return;
    }
    if ( !sscanf ( pElemTM->GetText(),"%lf",&x0 ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( ": Le topLeftCornerX est inexploitable." ) );
        return;
    }

    pElemTM = hTM.FirstChild ( "topLeftCornerY" ).Element();
    if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( " sans topLeftCornerY!!" ) );
        return;
    }
    if ( !sscanf ( pElemTM->GetText(),"%lf",&y0 ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( ": Le topLeftCornerY est inexploitable." ) );
        return;
    }

    pElemTM = hTM.FirstChild ( "tileWidth" ).Element();
    if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( " sans tileWidth!!" ) );
        return;
    }
    if ( !sscanf ( pElemTM->GetText(),"%d",&tileW ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( ": Le tileWidth est inexploitable." ) );
        return;
    }

    pElemTM = hTM.FirstChild ( "tileHeight" ).Element();
    if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( " sans tileHeight!!" ) );
        return;
    }
    if ( !sscanf ( pElemTM->GetText(),"%d",&tileH ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( ": Le tileHeight est inexploitable." ) );
        return;
    }

    pElemTM = hTM.FirstChild ( "matrixWidth" ).Element();
    if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( " sans MatrixWidth!!" ) );
        return;
    }
    if ( !sscanf ( pElemTM->GetText(),"%ld",&matrixW ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( ": Le MatrixWidth est inexploitable." ) );
        return;
    }

    pElemTM = hTM.FirstChild ( "matrixHeight" ).Element();
    if ( !pElemTM || ! ( pElemTM->GetText() ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", TileMatrix " ) << id <<_ ( " sans matrixHeight!!" ) );
        return;
    }
    if ( !sscanf ( pElemTM->GetText(),"%ld",&matrixH ) ) {
        LOGGER_ERROR ( _ ( "TileMatrixSet " ) << tmsId <<_ ( ", tileMatrix " ) << id <<_ ( ": Le matrixHeight est inexploitable." ) );
        return;
    }

    ok = true;
}

TileMatrixXML::~TileMatrixXML(){ }

std::string TileMatrixXML::getId() { return id; }

bool TileMatrixXML::isOk() { return ok; }
