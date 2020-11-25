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

#include "StyleXML.h"
#include "ConfLoader.h"


StyleXML::StyleXML(std::string path, bool inspire) : DocumentXML ( path )
{
    ok = false;

    TiXmlDocument doc ( filePath.c_str() );
    if ( !doc.LoadFile() ) {
        LOGGER_ERROR ( _ ( "                Ne peut pas charger le fichier " )  << filePath );
        return;
    }

    LOGGER_INFO ( _ ( "     Ajout du Style " ) << filePath );


    /********************** ID = nom du fichier sans extension */

    id = ConfLoader::getFileName(filePath, ".stl");

    /********************** Default values */

    rgbContinuous = false;
    alphaContinuous = false;
    noAlpha = false;
    float zenith,azimuth,zFactor;
    std::string algo = "";
    std::string unit = "";
    std::string inter = "";
    std::string interOfEst = "";
    int ndslope,mxSlope;
    float ndimg;
    float minSlope = 5.0;

    int errorCode;

    /********************** Parse */

    TiXmlHandle hDoc ( &doc );
    TiXmlElement* pElem;
    TiXmlHandle hRoot ( 0 );

    pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
    if ( !pElem ) {
        LOGGER_ERROR ( filePath << _ ( "            Impossible de recuperer la racine." ) );
        return;
    }
    if ( strcmp ( pElem->Value(),"style" ) ) {
        LOGGER_ERROR ( filePath << _ ( "            La racine n'est pas un style." ) );
        return;
    }
    hRoot=TiXmlHandle ( pElem );

    pElem=hRoot.FirstChild ( "Identifier" ).Element();
    if ( !pElem || ! ( pElem->GetText() ) ) {
        LOGGER_ERROR ( _ ( "Style " ) << filePath <<_ ( " pas de d'identifiant!!" ) );
        return;
    }
    identifier = DocumentXML::getTextStrFromElem(pElem);
    if ( Request::containForbiddenChars(identifier) ) {
        LOGGER_ERROR ( _ ( "Style " ) << filePath <<_ ( " : l'identifiant contient des caracteres interdits" ) );
        return;
    }

    for ( pElem=hRoot.FirstChild ( "Title" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "Title" ) ) {
        
        if ( ! ( pElem->GetText() ) ) continue;

        std::string curtitle = DocumentXML::getTextStrFromElem(pElem);
        titles.push_back ( curtitle );
    }

    if ( titles.size() ==0 ) {
        LOGGER_ERROR ( _ ( "Aucun Title trouve dans le Style" ) << id <<_ ( " : il est invalide!!" ) );
        return;
    }

    for ( pElem=hRoot.FirstChild ( "Abstract" ).Element(); pElem; pElem=pElem->NextSiblingElement ( "Abstract" ) ) {
        
        if ( ! ( pElem->GetText() ) ) continue;

        std::string curAbstract = DocumentXML::getTextStrFromElem(pElem);
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
        keywords.push_back ( Keyword ( DocumentXML::getTextStrFromElem(pElem),attributes ) );
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

        errorCode = pElem->QueryFloatAttribute ( "zenith",&zenith );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut zenith invalide a ete trouve dans l'estompage du Style " ) << id <<_ ( " : il est invalide!!" ) );
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas de zenith defini, 45 par defaut");
        } else {
            estompage.setZenith(zenith);
        }
        errorCode = pElem->QueryFloatAttribute ( "azimuth",&azimuth );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut azimuth invalide a ete trouve dans l'estompage du Style " ) << id <<_ ( " : il est invalide!!" ) );
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas de azimuth defini, 315 par defaut");
        } else {
            estompage.setAzimuth(azimuth);
        }

        errorCode = pElem->QueryFloatAttribute ( "zFactor",&zFactor );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut zFactor invalide a ete trouve dans l'estompage du Style " ) << id <<_ ( " : il est invalide!!" ) );
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas de zFactor defini, 1 par defaut");
        } else {
            estompage.setZFactor(zFactor);
        }

        errorCode = pElem->QueryStringAttribute("interpolation", &interOfEst);
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut interpolation invalide a ete trouve dans l'estompage du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return;
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas d'interpolation defini, 'linear' par defaut");
        } else {
             if (interOfEst != "linear" && interOfEst != "cubic" && interOfEst != "nn" && interOfEst != "lanczos") {
                LOGGER_ERROR ("Un attribut interpolation invalide a ete trouve dans l'estompage du Style " ) << id << ( ", les valeurs possibles sont 'nn','linear','cubic' et 'lanczos'");
                return;
            } else {
                 estompage.setInterpolation(interOfEst);
             }
        }
    } else {
        estompage.setEstompage(false);
    }
    
    //recuperation des informations pour le calcul des pentes
    pElem = hRoot.FirstChild ( "pente" ).Element();
    
    if ( pElem && !estompage.isEstompage()) {

        pente.setPente(true);
        
        errorCode = pElem->QueryStringAttribute("algo", &algo);
        
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut algo invalide a ete trouve dans la pente du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return;
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas d'algo defini, H par defaut");
        } else {
            if (algo != "H" && algo != "Z") {
                LOGGER_ERROR ("Un attribut algo invalide a ete trouve dans la pente du Style " ) << id << ( ", la valeur possible est H");
                return;
            }
            pente.setAlgo(algo);
        }


        errorCode = pElem->QueryStringAttribute("unit", &unit);
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut unit invalide a ete trouve dans la pente du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return;
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas d'unit defini, 'degree' par defaut");
        } else {
             if (unit != "degree" && unit != "pourcent") {
                LOGGER_ERROR ("Un attribut unit invalide a ete trouve dans la pente du Style " ) << id << ( ", la valeur possible est 'degree' or 'pourcent'");
                return;
            }
            pente.setUnit(unit);
        }


        errorCode = pElem->QueryStringAttribute("interpolation", &inter);
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut interpolation invalide a ete trouve dans la pente du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return;
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas d'interpolation defini, 'linear' par defaut");
        } else {
             if (inter != "linear" && inter != "cubic" && inter != "nn" && inter != "lanczos") {
                LOGGER_ERROR ("Un attribut interpolation invalide a ete trouve dans la pente du Style " ) << id << ( ", les valeurs possibles sont 'nn','linear','cubic' et 'lanczos'");
                return;
            }
            pente.setInterpolation(inter);
        }


        errorCode = pElem->QueryIntAttribute("slopeNoData", &ndslope);
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut slopeNoData invalide a ete trouve dans la pente du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return;
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas de slopeNoData defini, '0' par defaut");
        } else {
             if (ndslope < 0 || ndslope > 255) {
                LOGGER_ERROR ("Un attribut ndslope invalide a ete trouve dans la pente du Style " ) << id << ( ", les valeurs possibles sont entre 0 et 255");
                return;
            }
            pente.setSlopeNoData(ndslope);
        }


        errorCode = pElem->QueryFloatAttribute("imageNoData", &ndimg);
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut imageNoData invalide a ete trouve dans la pente du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return;
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas de imageNoData defini, '-99999' par defaut");
        } else {
            pente.setImgNoData(ndimg);
        }

        errorCode = pElem->QueryIntAttribute("maxSlope", &mxSlope);
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut maxSlope invalide a ete trouve dans la pente du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return;
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            if (unit == "degree") {
                LOGGER_INFO("Pas de maxSlope defini, '90' par defaut");
            } else {
                LOGGER_INFO("Pas de maxSlope defini, '255' par defaut");
                mxSlope = 255;
                pente.setMaxSlope(mxSlope);
            }
        } else {
             if (mxSlope < 0 || mxSlope > 255) {
                LOGGER_ERROR ("Un attribut maxSlope invalide a ete trouve dans la pente du Style " ) << id << ( ", les valeurs possibles sont entre 0 et 255");
                return;
            }
            pente.setMaxSlope(mxSlope);
        }

    }

    //recuperation des informations pour le calcul des expostiions des  pentes
    pElem = hRoot.FirstChild ( "exposition" ).Element();

    if ( pElem && !estompage.isEstompage() && !pente.getPente()) {

        aspect.setAspect(true);
        errorCode = pElem->QueryStringAttribute("algo", &algo);
        
         if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( _ ( "Un attribut algo invalide a ete trouve dans l'exposition du Style " ) << id <<_ ( " : il est invalide!!" ) );
            return;
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO("Pas d'algo defini, 'H' par defaut");
            algo = "H";
        } else {
            if (algo != "H") {
                LOGGER_ERROR ("Un attribut algo invalide a ete trouve dans l'exposition du Style " ) << id << ( ", la valeur possible est H");
                return;
            }
        }
        aspect.setAlgo(algo);

        errorCode = pElem->QueryFloatAttribute ( "minSlope",&minSlope );
        if ( errorCode == TIXML_WRONG_TYPE ) {
            LOGGER_ERROR ( ( "Un attribut minSlope invalide a ete trouve dans l'exposition du Style " ) << id << ( " : mauvais type, float attendu" ) );
        } else if ( errorCode == TIXML_NO_ATTRIBUTE ) {
            LOGGER_INFO ( ( "Un attribut est manquant dans l'exposition du Style " ) << id << ( " : minSlope" ) );
        }
        if (minSlope < 0.0 || minSlope > 90.0) {
            LOGGER_ERROR ( ( "Un attribut minSlope invalide a ete trouve dans l'exposition du Style " ) << id << ( " : mauvaise valeur, entre 0 et 90 attendue" ) );
            minSlope = 1.0;
        }
        aspect.setMinSlope(minSlope);

    }

    ok = true;
}


StyleXML::~StyleXML(){ }

std::string StyleXML::getId() { return id; }

bool StyleXML::isOk() { return ok; }
