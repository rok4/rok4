/*
 * Copyright © (2011) Institut national de l'information
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

#include "GetFeatureInfoEncoder.h"
#include <sstream>

GetFeatureInfoEncoder::GetFeatureInfoEncoder(std::vector<std::string> data, std::string info_format): data( data ), info_format( info_format ){
  
}
GetFeatureInfoEncoder::~GetFeatureInfoEncoder(){
  
}

DataStream* GetFeatureInfoEncoder::getDataStream() {
    if (this->info_format.compare("text/plain") == 0){
	return plainDataStream();
    } else if (this->info_format.compare("text/html") == 0){
	return htmlDataStream();
    } else if (this->info_format.compare("text/xml") == 0 || this->info_format.compare("application/xml") == 0){
	return xmlDataStream();
    } else if (this->info_format.compare("application/json") == 0){
	return jsonDataStream();
    } else {
	return NULL;
    }
}


DataStream* GetFeatureInfoEncoder::plainDataStream(){
  std::stringstream ss;
  for ( int i = 0 ; i < this->data.size(); i ++ ) {
    ss << this->data.at(i);
    if (i != (this->data.size()-1)){
      ss << " ";
    }
  }
  return new MessageDataStream(ss.str(), this->info_format);
}
DataStream* GetFeatureInfoEncoder::htmlDataStream(){
  std::stringstream ss;
  ss << "<html><body><b>FeatureInfo :</b><br><ul>";
  for ( int i = 0 ; i < this->data.size(); i ++ ) {
    ss << "<ul>" << this->data.at(i) << "</ul>";
  }
  ss << "</ul></body></html>";
  return new MessageDataStream(ss.str(), this->info_format);
}
DataStream* GetFeatureInfoEncoder::jsonDataStream(){
  std::stringstream ss;
  ss << "{\"featureInfo\":[";
  for ( int i = 0 ; i < this->data.size(); i ++ ) {
    ss << this->data.at(i);
    if (i != (this->data.size()-1)){
      ss << ", ";
    }
  }
  ss << "]}";
  return new MessageDataStream(ss.str(), this->info_format);
}
DataStream* GetFeatureInfoEncoder::xmlDataStream(){
  std::stringstream ss;
  ss << "<FeatureInfo>";
  for ( int i = 0 ; i < this->data.size(); i ++ ) {
    ss << "<value>" << this->data.at(i) << "</value>";
  }
  ss << "</FeatureInfo>";
  return new MessageDataStream(ss.str(), this->info_format);
}
