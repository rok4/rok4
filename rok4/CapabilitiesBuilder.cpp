/*
 * CapabilitiesBuilder.cpp
 *
 *  Created on: 3 sept. 2010
 *      Author: nico
 */

#include "WMSServer.h"
#include "tinyxml.h"
#include "tinystr.h"
#include <iostream>

std::string intToStr(int i){
	std::ostringstream strstr;
	strstr << i;
	return strstr.str();
}

void WMSServer::buildWMSCapabilities(){
	TiXmlDocument doc;
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", "" );
	doc.LinkEndChild( decl );


	TiXmlElement * capabilitiesEl = new TiXmlElement( "WMS_Capabilities" );
	capabilitiesEl->SetAttribute("version","1.3.0");
	capabilitiesEl->SetAttribute("xmlns","http://www.opengis.net/wms");
	capabilitiesEl->SetAttribute("xmlns:xlink","http://www.w3.org/1999/xlink");
	capabilitiesEl->SetAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
	capabilitiesEl->SetAttribute("xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd");


	// traitement de la partie service
	//----------------------------------
	TiXmlElement * serviceEl = new TiXmlElement( "Service" );
	//Name
	TiXmlElement * elem = new TiXmlElement( "Name" );
	TiXmlText * text = new TiXmlText( servicesConf.getName());
	elem->LinkEndChild(text);
	serviceEl->LinkEndChild(elem);
	//Title
	elem = new TiXmlElement( "Title" );
	text = new TiXmlText( servicesConf.getTitle());
	elem->LinkEndChild(text);
	serviceEl->LinkEndChild(elem);
	//Abstract
	elem = new TiXmlElement( "Abstract" );
	text = new TiXmlText( servicesConf.getAbstract());
	elem->LinkEndChild(text);
	serviceEl->LinkEndChild(elem);
	//KeywordList
	if (servicesConf.getKeyWords().size() != 0){
		TiXmlElement * kwlEl = new TiXmlElement( "KeywordList" );
		for (unsigned int i=0; i < servicesConf.getKeyWords().size(); i++){
			elem = new TiXmlElement( "Keyword" );
			text = new TiXmlText( servicesConf.getKeyWords()[i]);
			elem->LinkEndChild(text);
			kwlEl->LinkEndChild(elem);
		}
		serviceEl->LinkEndChild(kwlEl);
	}
	//OnlineResource
	TiXmlElement * onlineResourceEl = new TiXmlElement( "OnlineResource" );
	onlineResourceEl->SetAttribute("xmlns:xlink","http://www.w3.org/1999/xlink");
	onlineResourceEl->SetAttribute("xlink:href","http://hostname/");
	serviceEl->LinkEndChild(onlineResourceEl);
	// Pas de ContactInformation (facultatif).
	//Fees
	elem = new TiXmlElement( "Fees" );
	text = new TiXmlText( servicesConf.getFee());
	elem->LinkEndChild(text);
	serviceEl->LinkEndChild(elem);
	//AccessConstraints
	elem = new TiXmlElement( "AccessConstraints" );
	text = new TiXmlText( servicesConf.getAccessConstraint());
	elem->LinkEndChild(text);
	serviceEl->LinkEndChild(elem);
	//LayerLimit
	elem = new TiXmlElement( "LayerLimit" );
	text = new TiXmlText("1");
	elem->LinkEndChild(text);
	serviceEl->LinkEndChild(elem);
	//MaxWidth
	elem = new TiXmlElement( "MaxWidth" );
	text = new TiXmlText(intToStr(servicesConf.getMaxWidth()));
	elem->LinkEndChild(text);
	serviceEl->LinkEndChild(elem);
	//MaxHeight
	elem = new TiXmlElement( "MaxHeight" );
	text = new TiXmlText(intToStr(servicesConf.getMaxHeight()));
	elem->LinkEndChild(text);
	serviceEl->LinkEndChild(elem);

	capabilitiesEl->LinkEndChild( serviceEl );



	// Traitement de la partie Capability
	//-----------------------------------
	TiXmlElement * capabilityEl = new TiXmlElement( "Capability" );
	TiXmlElement * requestEl = new TiXmlElement( "Request" );
	TiXmlElement * getCapabilitiestEl = new TiXmlElement( "GetCapabilities" );
	elem = new TiXmlElement( "Format" );
	text = new TiXmlText("text/xml");
	elem->LinkEndChild(text);
	getCapabilitiestEl->LinkEndChild(elem);
	//DCPType
	TiXmlElement * DCPTypeEl = new TiXmlElement( "DCFPType" );
	TiXmlElement * HTTPEl = new TiXmlElement( "HTTP" );
	TiXmlElement * GetEl = new TiXmlElement( "Get" );
	//OnlineResource
	onlineResourceEl = new TiXmlElement( "OnlineResource" );
	onlineResourceEl->SetAttribute("xmlns:xlink","http://www.w3.org/1999/xlink");
	onlineResourceEl->SetAttribute("xlink:href","http://hostname/path?");
	onlineResourceEl->SetAttribute("xlink:type","simple");
	GetEl->LinkEndChild(onlineResourceEl);
	HTTPEl->LinkEndChild(GetEl);
	DCPTypeEl->LinkEndChild(HTTPEl);
	getCapabilitiestEl->LinkEndChild(DCPTypeEl);
	requestEl->LinkEndChild(getCapabilitiestEl);

	TiXmlElement * getMapEl = new TiXmlElement( "GetMap" );
	for (unsigned int i=0; i<servicesConf.getFormatList().size(); i++){
		elem = new TiXmlElement( "Format" );
		text = new TiXmlText(servicesConf.getFormatList()[i]);
		elem->LinkEndChild(text);
		getMapEl->LinkEndChild(elem);
	}
	DCPTypeEl = new TiXmlElement( "DCFPType" );
	HTTPEl = new TiXmlElement( "HTTP" );
	GetEl = new TiXmlElement( "Get" );
	onlineResourceEl = new TiXmlElement( "OnlineResource" );
	onlineResourceEl->SetAttribute("xmlns:xlink","http://www.w3.org/1999/xlink");
	onlineResourceEl->SetAttribute("xlink:href","http://hostname/path?");
	onlineResourceEl->SetAttribute("xlink:type","simple");
	GetEl->LinkEndChild(onlineResourceEl);
	HTTPEl->LinkEndChild(GetEl);
	DCPTypeEl->LinkEndChild(HTTPEl);
	getMapEl->LinkEndChild(DCPTypeEl);

	requestEl->LinkEndChild(getMapEl);

	capabilityEl->LinkEndChild(requestEl);

	//exception
	TiXmlElement * exceptionEl = new TiXmlElement( "Exception" );
	elem = new TiXmlElement( "Format" );
	text = new TiXmlText("XML");
	elem->LinkEndChild(text);
	exceptionEl->LinkEndChild(elem);
	capabilityEl->LinkEndChild(exceptionEl);

	// Layer
	std::map<std::string, Layer*>::iterator it(layers.begin()), itend(layers.end());
	for (;it!=itend;++it){
		TiXmlElement * layerEl = new TiXmlElement( "Layer" );
		Layer* layer = it->second;
		//Name
		TiXmlElement * elem = new TiXmlElement( "Name" );
		TiXmlText * text = new TiXmlText(layer->getId());
		elem->LinkEndChild(text);
		layerEl->LinkEndChild(elem);
		//Title
		elem = new TiXmlElement( "Title" );
		text = new TiXmlText(layer->getTitle());
		elem->LinkEndChild(text);
		layerEl->LinkEndChild(elem);
		//Abstract
		elem = new TiXmlElement( "Abstract" );
		text = new TiXmlText( layer->getAbstract());
		elem->LinkEndChild(text);
		layerEl->LinkEndChild(elem);
		//KeywordList
		if (layer->getKeyWords().size() != 0){
			TiXmlElement * kwlEl = new TiXmlElement( "KeywordList" );
			for (unsigned int i=0; i < layer->getKeyWords().size(); i++){
				elem = new TiXmlElement( "Keyword" );
				text = new TiXmlText( layer->getKeyWords()[i]);
				elem->LinkEndChild(text);
				kwlEl->LinkEndChild(elem);
			}
			layerEl->LinkEndChild(kwlEl);
		}
		//CRS
		for (unsigned int i=0; i < layer->getWMSCRSList().size(); i++){
			elem = new TiXmlElement( "CRS" );
			text = new TiXmlText(layer->getWMSCRSList()[i]);
			elem->LinkEndChild(text);
			layerEl->LinkEndChild(elem);
		}

/*		layer->getAuthority();
		layer->getMaxRes();
		layer->getMinRes();
		layer->getOpaque();
		layer->getStyles();
*/
		capabilityEl->LinkEndChild(layerEl);

	}// for layer

	capabilitiesEl->LinkEndChild(capabilityEl);
	doc.LinkEndChild( capabilitiesEl );

//	std::cout << doc; // ecriture non formatée dans la flux
//	doc.Print();  // affichage formaté sur stdout
	WMSCapabilities << doc;
/*	std::cout << ":"<<std::endl;
	std::cout << ":"<<std::endl;
	std::cout << ":"<<std::endl;
	std::cout << WMSCapabilities << std::endl;
*/
}


void WMSServer::buildWMTSCapabilities(){
	WMTSCapabilities="WMTS Capabilities";
}
