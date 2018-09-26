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


#ifndef DOCUMENTXML_H
#define DOCUMENTXML_H

#include <string>
#include <libgen.h>
#include <tinyxml.h>

class DocumentXML
{
    
    public:

        static const std::string getTextStrFromElem(TiXmlElement* pElem) {
            const TiXmlNode* child = pElem->FirstChild();
            if ( child ) {
                const TiXmlText* childText = child->ToText();
                if ( childText ) {
                    return childText->ValueStr();
                }
            }
            return "";
        }

        /**
         * \~french Construit un noeud xml simple (de type text)
         * \~english Create a simple XML text node
         */
        static TiXmlElement* buildTextNode ( std::string elementName, std::string value ) {
            TiXmlElement * elem = new TiXmlElement ( elementName );
            TiXmlText * text = new TiXmlText ( value );
            elem->LinkEndChild ( text );
            return elem;
        }

        
        DocumentXML(std::string path) {
            filePath = path;

            char * fileNameChar = ( char * ) malloc ( strlen ( filePath.c_str() ) + 1 );
            strcpy ( fileNameChar, filePath.c_str() );
            char * parentDirChar = dirname ( fileNameChar );
            parentDir = std::string ( parentDirChar );
            free ( fileNameChar );
            fileNameChar=NULL;
            parentDirChar=NULL;
        }

        ~DocumentXML() {}
        
    protected:

        std::string filePath;
        std::string parentDir;

    private:

};

#endif // DOCUMENTXML_H

