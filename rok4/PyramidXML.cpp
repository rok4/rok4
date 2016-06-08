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

#include "PyramidXML.h"

#include <tinyxml.h>
#include <tinystr.h>

#include "TileMatrixXML.h"

PyramidXML::PyramidXML(std::string fileName, ServerXML* serverXML, ServicesXML* servicesXML, std::map<std::string, TileMatrixSet*> &tmsList , std::map<std::string, Style *> stylesList, bool times)
{
    ok = false;

    TiXmlDocument doc ( fileName.c_str() );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "Ne peut pas charger le fichier " ) << fileName );
        return;
    }

    LOGGER_INFO ( _ ( "             Ajout de la pyramide : " ) << fileName );

    char * fileNameChar = ( char * ) malloc ( strlen ( fileName.c_str() ) + 1 );
    strcpy ( fileNameChar, fileName.c_str() );
    char * parentDirChar = dirname ( fileNameChar );
    std::string parentDir ( parentDirChar );
    free ( fileNameChar );
    fileNameChar=NULL;
    parentDirChar=NULL;
    LOGGER_INFO ( _ ( "           BaseDir Relative to : " ) << parentDir );


    /********************** Default values */

    formatStr="";
    onDemand = false;
    onDemandSpecific = false;
    nbSpecificLevel = 0;
    basedPyramid = NULL;
    ws = NULL;
    onFly = false;
    testOnFly = true;

    /********************** Parse */

    TiXmlHandle hDoc ( &doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( fileName << _ ( " impossible de recuperer la racine." ) );
        return;
    }
    if ( strcmp ( pElem->Value(),"Pyramid" ) ) {
        LOGGER_ERROR ( fileName << _ ( " La racine n'est pas une Pyramid." ) );
        return;
    }
    hRoot=TiXmlHandle ( pElem );
    //----

    //----TMS
    pElem=hRoot.FirstChild ( "tileMatrixSet" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << fileName <<_ ( "] n'a pas de TMS. C'est un probleme." ) );
        return;
    }
    std::string tmsName= pElem->GetTextStr();
    std::map<std::string, TileMatrixSet *>::iterator it;
    it=tmsList.find ( tmsName );
    if ( it == tmsList.end() ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << fileName <<_ ( "] reference un TMS [" ) << tmsName <<_ ( "] qui n'existe pas." ) );
        return;
    }
    tms=it->second;
    //----

    //----FORMAT
    pElem=hRoot.FirstChild ( "format" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << fileName <<_ ( "] n'a pas de format." ) );
        return;
    }
    formatStr= pElem->GetTextStr();

    //  to remove when TIFF_RAW_INT8 et TIFF_RAW_FLOAT32 only will be used
    if ( formatStr.compare ( "TIFF_INT8" ) == 0 ) formatStr = "TIFF_RAW_INT8";
    if ( formatStr.compare ( "TIFF_FLOAT32" ) == 0 ) formatStr = "TIFF_RAW_FLOAT32";

    format = Rok4Format::fromString ( formatStr );
    if ( ! ( format ) ) {
        LOGGER_ERROR ( fileName << _ ( "Le format [" ) << formatStr <<_ ( "] n'est pas gere." ) );
        return;
    }
    //----

    //----PHOTOMETRIE
    // on lit l'élément photometric, il n'est pas obligatoire pour
    // une pyramide normale mais il le devient si la pyramide
    // est à la volée
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
        channels = DEFAULT_CHANNELS;
        return;
    } else if ( !sscanf ( pElem->GetText(),"%d",&channels ) ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << fileName <<_ ( "] : channels=[" ) << pElem->GetTextStr() <<_ ( "] is not an integer." ) );
        return;
    }
    //----

    //----NODATAVALUE
    // on lit l'élément nodatavalues, il n'est pas obligatoire pour
    // une pyramide normale mais il le devient si la pyramide
    // est à la volée
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

    // on va vérifier que les levels sont spécifiés
    // si c'est une pyramide à la demande, ce n'est pas obligatoire
    if (hRoot.FirstChild ( "level" ).Element()) {

    for ( pElem=hRoot.FirstChild ( "level" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "level" ) ) {

        LevelXML levXML(pElem, ...);
        if (! levXML.isOk()) {
            return;
        }

        std::string lId = levXML.getId();
        Level* levObj = new Level(levXML);
        listTM.insert ( std::pair<std::string, Level*> ( lId, levObj ) );

    } //if level

    if ( levels.size() ==0 ) {
        LOGGER_ERROR ( _ ( "Aucun level n'a pu etre charge pour la pyramide " ) << fileName );
        return;
    }


    //----PYRAMID

    Pyramid* pyr;

    if (onFly) {
    pyr = new PyramidOnFly(levels, *tms, format, channels, onDemand, onFly, Photometric::fromString(photometricStr),noDataValues,specificSources);
    } else {
    if (onDemand) {
    pyr = new PyramidOnDemand(levels, *tms, format, channels, onDemand, onFly,specificSources);
    } else {
    pyr = new Pyramid ( levels, *tms, format, channels, onDemand, onFly );
    }
    }

    //----
    return pyr;

    ok = true;
}

