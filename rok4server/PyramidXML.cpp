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

#include "PyramidXML.h"

#include <tinyxml.h>
#include <libgen.h>

#include "TileMatrixXML.h"

PyramidXML::PyramidXML(std::string path, ServerXML* serverXML, ServicesXML* servicesXML, bool times) : DocumentXML (path)
{
    ok = false;

    TiXmlDocument doc ( filePath.c_str() );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "Ne peut pas charger le fichier " ) << filePath );
        return;
    }

    LOGGER_INFO ( _ ( "           Ajout de la pyramide : " ) << filePath );
    LOGGER_INFO ( _ ( "           BaseDir Relative to : " ) << parentDir );


    /********************** Default values */

    allowWMS = true;
    isBasedPyramid = ! times;
    containOdLevels = false;

    /********************** Parse */

    TiXmlHandle hDoc ( &doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( filePath << _ ( " impossible de recuperer la racine." ) );
        return;
    }
    if ( strcmp ( pElem->Value(),"Pyramid" ) ) {
        LOGGER_ERROR ( filePath << _ ( " La racine n'est pas une Pyramid." ) );
        return;
    }
    hRoot=TiXmlHandle ( pElem );
    //----

    //----TMS
    pElem=hRoot.FirstChild ( "tileMatrixSet" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << filePath <<_ ( "] n'a pas de TMS. C'est un probleme." ) );
        return;
    }
    std::string tmsName= DocumentXML::getTextStrFromElem(pElem);

    tms = serverXML->getTMS(tmsName);
    if ( tms == NULL ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << filePath <<_ ( "] reference un TMS [" ) << tmsName <<_ ( "] qui n'existe pas." ) );
        return;
    }
    //----

    //----FORMAT
    pElem=hRoot.FirstChild ( "format" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "La pyramide [" ) << filePath <<_ ( "] n'a pas de format." ) );
        return;
    }
    std::string formatStr= DocumentXML::getTextStrFromElem(pElem);

    //  to remove when TIFF_RAW_INT8 et TIFF_RAW_FLOAT32 only will be used
    if ( formatStr.compare ( "TIFF_INT8" ) == 0 ) formatStr = "TIFF_RAW_INT8";
    if ( formatStr.compare ( "TIFF_FLOAT32" ) == 0 ) formatStr = "TIFF_RAW_FLOAT32";

    format = Rok4Format::fromString ( formatStr );
    if ( ! ( format ) ) {
        LOGGER_ERROR ( _ ("Le format [" ) << formatStr <<_ ( "] n'est pas gere." ) );
        return;
    }
    //----


    /******************* PYRAMIDE RASTER *********************/
    
    if (Rok4Format::isRaster(format)) {

        //----PHOTOMETRIE
        pElem=hRoot.FirstChild ( "photometric" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            LOGGER_ERROR ( "La pyramide [" << filePath << "] n'a pas de photométrie." );
            return;
        }
        std::string photometricStr = DocumentXML::getTextStrFromElem(pElem);

        photo = Photometric::fromString ( photometricStr );
        if ( ! ( photo ) ) {
            LOGGER_ERROR ( filePath << "La photométrie [" << photometricStr << "] n'est pas gere." );
            return;
        }
        //----


        //----CHANNELS
        pElem=hRoot.FirstChild ( "channels" ).Element();
        if ( !pElem || ! ( pElem->GetText() ) ) {
            LOGGER_ERROR ( _ ( "La pyramide [" ) << filePath <<_ ( "] Pas de channels => channels = " ) << DEFAULT_CHANNELS );
            channels = DEFAULT_CHANNELS;
            return;
        } else if ( !sscanf ( pElem->GetText(),"%d",&channels ) ) {
            LOGGER_ERROR ( _ ( "La pyramide [" ) << filePath <<_ ( "] : channels=[" ) << DocumentXML::getTextStrFromElem(pElem) <<_ ( "] is not an integer." ) );
            return;
        }
        //----

        //----NODATAVALUE
        // on lit l'élément nodatavalues, il n'est pas obligatoire pour
        // une pyramide normale mais il le devient si la pyramide
        // est à la volée
        pElem=hRoot.FirstChild ( "nodataValue" ).Element();
        if ( pElem && pElem->GetText() ) {
            ndValuesStr = DocumentXML::getTextStrFromElem(pElem);

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
    }


    /******************* PARTIE COMMUNE *********************/

    //----LEVELS
    for ( pElem=hRoot.FirstChild ( "level" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "level" ) ) {
        LevelXML* levXML = new LevelXML(pElem, filePath, serverXML, servicesXML, this, times);
        if (! levXML->isOk()) {
            delete levXML;
            return;
        }

        //on va vérifier que le level qu'on vient de charger n'a pas déjà été chargé
        std::map<std::string, Level*>::iterator it= levels.find ( levXML->getId() );
        if ( it != levels.end() ) {
            LOGGER_ERROR ( _ ( "Level: " ) << levXML->getId() << _ ( " has already been loaded" ) );
            delete levXML;
            return ;
        }

        if (levXML->isOnDemand() || levXML->isOnFly()) containOdLevels = true;

        std::string lId = levXML->getId();
        Level* levObj = new Level(levXML, this);
        levels.insert ( std::pair<std::string, Level*> ( lId, levObj ) );

        delete levXML;
    } //if level

    if ( levels.size() == 0 ) {
        LOGGER_ERROR ( _ ( "Aucun level n'a pu etre charge pour la pyramide " ) << filePath );
        return;
    }

    ok = true;
}

PyramidXML::~PyramidXML(){

    if (! ok) {
        // Cette pyramide n'est pas valide, donc n'a pas été utilisé pour créer un objet Pyramid. Il faut donc nettoyer tout ce qui a été créé.
        std::map<std::string, Level*>::iterator iLevel;
        for ( iLevel=levels.begin(); iLevel!=levels.end(); iLevel++ )
            delete iLevel->second;

    }

}

bool PyramidXML::isOk() { return ok; }
int* PyramidXML::getNoDataValues() {
    int* nd = new int[channels];
    for (int i=0;i<channels;i++) {
        nd[i] = noDataValues.at(i);
    }
    return nd;
}
int PyramidXML::getChannels() { return channels; }
TileMatrixSet* PyramidXML::getTMS() { return tms; }
Rok4Format::eformat_data PyramidXML::getFormat() {return format; }
