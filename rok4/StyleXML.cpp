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

#include "StylesXML.h"


StylesXML::StylesXML(std::string styleFilepath, ServicesXML* servicesXML)
{
    ok = false;

    TiXmlDocument doc ( fileName.c_str() );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "                Ne peut pas charger le fichier " )  << fileName );
        return;
    }

    LOGGER_INFO ( _ ( "     Ajout du Style " ) << fileName );

    /********************** Default values */

    bool inspire = servicesXML->isInspire();
    id ="";
    rgbContinuous = false;
    alphaContinuous = false;
    noAlpha = false;
    angle = -1;
    exaggeration = 1;
    center = 0;

    int errorCode;

    /********************** Parse */

    TiXmlHandle hDoc ( &doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( fileName << _ ( "            Impossible de recuperer la racine." ) );
        return;
    }
    if ( strcmp ( pElem->Value(),"style" ) ) {
        LOGGER_ERROR ( fileName << _ ( "            La racine n'est pas un style." ) );
        return;
    }
    hRoot=TiXmlHandle ( pElem );

    pElem=hRoot.FirstChild ( "Identifier" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "Style " ) << fileName <<_ ( " pas de d'identifiant!!" ) );
        return;
    }
    id = pElem->GetTextStr();

    for ( pElem=hRoot.FirstChild ( "Title" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "Title" ) ) {
        
        if ( ! ( pElem->GetText() ) ) continue;

        std::string curtitle = pElem->GetTextStr();
        titles.push_back ( curtitle );
    }

    if ( titles.size() ==0 ) {
        LOGGER_ERROR ( _ ( "Aucun Title trouve dans le Style" ) << id <<_ ( " : il est invalide!!" ) );
        return;
    }

    for ( pElem=hRoot.FirstChild ( "Abstract" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "Abstract" ) ) {
        
        if ( ! ( pElem->GetText() ) ) continue;

        std::string curAbstract = pElem->GetTextStr();
        abstracts.push_back ( curAbstract );
    }

    if ( abstracts.size() == 0 && inspire ) {
        LOGGER_ERROR ( _ ( "Aucun Abstract trouve dans le Style" ) << id <<_ ( " : il est invalide au sens INSPIRE!!" ) );
        return;
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
        keywords.push_back ( Keyword ( pElem->GetTextStr(),attributes ) );
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
        return;
    }

    pElem = hRoot.FirstChild ( "palette" ).Element();

    if ( pElem ) {
        double maxValue=0.0;

        std::string continuousStr;

        errorCode = pElem->QueryStringAttribute ( "rgbContinuous",&continuousStr );
        if ( errorCode != TIXML_SUCCESS ) {
            LOGGER_DEBUG ( _ ( "L'attribut rgbContinuous n'a pas ete trouve dans la palette du Style " ) << id <<_ ( " : Faux par defaut" ) );
        } else {
            if ( continuousStr.compare ( "true" ) ==0 ) rgbContinuous=true;
        }

        errorCode = pElem->QueryStringAttribute ( "alphaContinuous",&continuousStr );
        if ( errorCode != TIXML_SUCCESS ) {
            LOGGER_DEBUG ( _ ( "L'attribut alphaContinuous n'a pas ete trouve dans la palette du Style " ) << id <<_ ( " : Faux par defaut" ) );
        } else {
            if ( continuousStr.compare ( "true" ) ==0 ) alphaContinuous=true;
        }
        
        errorCode = pElem->QueryStringAttribute ( "noAlpha",&continuousStr );
        if ( errorCode != TIXML_SUCCESS ) {
            LOGGER_DEBUG ( _ ( "L'attribut noAlpha n'a pas ete trouve dans la palette du Style " ) << id <<_ ( " : Faux par defaut" ) );
        } else {
            if ( continuousStr.compare ( "true" ) ==0 ) noAlpha=true;
        }

        errorCode = pElem->QueryDoubleAttribute ( "maxValue",&maxValue );
        if ( errorCode != TIXML_SUCCESS ) {
            LOGGER_ERROR ( _ ( "L'attribut maxValue n'a pas ete trouve dans la palette du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return;
        } else {
            LOGGER_DEBUG ( _ ( "MaxValue " ) << maxValue );

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
                return;
            }

        }
    }

    palette = Palette( colourMap, rgbContinuous, alphaContinuous, noAlpha );

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

    ok = true;
}


~StyleXML::StyleXML(){ }

std::string StyleXML::getId() { return id; }

bool StyleXML::isOk() { return ok; }