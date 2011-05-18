#include "Rok4Server.h"
#include "tinyxml.h"
#include "tinystr.h"
#include <iostream>
#include <vector>
#include <map>


/**
 * Conversion de int en std::string.
 */
std::string numToStr(int i){
	std::ostringstream strstr;
	strstr << i;
	return strstr.str();
}

/**
 * construit un noeud xml simple (de type text).
 */
TiXmlElement * buildTextNode(std::string elementName, std::string value){
	TiXmlElement * elem = new TiXmlElement( elementName );
	TiXmlText * text = new TiXmlText(value);
	elem->LinkEndChild(text);
	return elem;
}

/**
 * Construit les fragments invariants du getCapabilities WMS (wmsCapaFrag).
 */
void Rok4Server::buildWMSCapabilities(){
	std::string hostNameTag="]HOSTNAME[";   ///Tag a remplacer par le nom du serveur
	std::string pathTag="]HOSTNAME/PATH[";  ///Tag à remplacer par le chemin complet avant le ?.
	TiXmlDocument doc;
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", "" );
	doc.LinkEndChild( decl );


	TiXmlElement * capabilitiesEl = new TiXmlElement( "WMS_Capabilities" );
	capabilitiesEl->SetAttribute("version","1.3.0");
	capabilitiesEl->SetAttribute("xmlns","http://www.opengis.net/wms");
	// Pour Inspire. Cf. remarque plus bas.
	capabilitiesEl->SetAttribute("xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs/1.0");
	capabilitiesEl->SetAttribute("xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0");
	capabilitiesEl->SetAttribute("xmlns:xlink","http://www.w3.org/1999/xlink");
	capabilitiesEl->SetAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
	capabilitiesEl->SetAttribute("xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd  http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd http://inspire.ec.europa.eu/schemas/common/1.0 http://inspire.ec.europa.eu/schemas/common/1.0/common.xsd");


	// Traitement de la partie service
	//----------------------------------
	TiXmlElement * serviceEl = new TiXmlElement( "Service" );
	serviceEl->LinkEndChild(buildTextNode("Name",servicesConf.getName()));
	serviceEl->LinkEndChild(buildTextNode("Title",servicesConf.getTitle()));
	serviceEl->LinkEndChild(buildTextNode("Abstract",servicesConf.getAbstract()));
	//KeywordList
	if (servicesConf.getKeyWords().size() != 0){
		TiXmlElement * kwlEl = new TiXmlElement( "KeywordList" );
		for (unsigned int i=0; i < servicesConf.getKeyWords().size(); i++){
			kwlEl->LinkEndChild(buildTextNode("Keyword",servicesConf.getKeyWords()[i]));
		}
		serviceEl->LinkEndChild(kwlEl);
	}
	//OnlineResource
	TiXmlElement * onlineResourceEl = new TiXmlElement( "OnlineResource" );
	onlineResourceEl->SetAttribute("xmlns:xlink","http://www.w3.org/1999/xlink");
	onlineResourceEl->SetAttribute("xlink:href",hostNameTag);
	serviceEl->LinkEndChild(onlineResourceEl);
	// Pas de ContactInformation (facultatif).
	serviceEl->LinkEndChild(buildTextNode("Fees",servicesConf.getFee()));
	serviceEl->LinkEndChild(buildTextNode("AccessConstraints",servicesConf.getAccessConstraint()));
	serviceEl->LinkEndChild(buildTextNode("LayerLimit","1"));
	serviceEl->LinkEndChild(buildTextNode("MaxWidth",numToStr(servicesConf.getMaxWidth())));
	serviceEl->LinkEndChild(buildTextNode("MaxHeight",numToStr(servicesConf.getMaxHeight())));

	capabilitiesEl->LinkEndChild( serviceEl );



	// Traitement de la partie Capability
	//-----------------------------------
	TiXmlElement * capabilityEl = new TiXmlElement( "Capability" );
	TiXmlElement * requestEl = new TiXmlElement( "Request" );
	TiXmlElement * getCapabilitiestEl = new TiXmlElement( "GetCapabilities" );

	getCapabilitiestEl->LinkEndChild(buildTextNode("Format","text/xml"));
	//DCPType
	TiXmlElement * DCPTypeEl = new TiXmlElement( "DCPType" );
	TiXmlElement * HTTPEl = new TiXmlElement( "HTTP" );
	TiXmlElement * GetEl = new TiXmlElement( "Get" );
	//OnlineResource
	onlineResourceEl = new TiXmlElement( "OnlineResource" );
	onlineResourceEl->SetAttribute("xmlns:xlink","http://www.w3.org/1999/xlink");
	onlineResourceEl->SetAttribute("xlink:href",pathTag);
	onlineResourceEl->SetAttribute("xlink:type","simple");
	GetEl->LinkEndChild(onlineResourceEl);
	HTTPEl->LinkEndChild(GetEl);
	DCPTypeEl->LinkEndChild(HTTPEl);
	getCapabilitiestEl->LinkEndChild(DCPTypeEl);
	requestEl->LinkEndChild(getCapabilitiestEl);

	TiXmlElement * getMapEl = new TiXmlElement( "GetMap" );
	for (unsigned int i=0; i<servicesConf.getFormatList()->size(); i++){
		getMapEl->LinkEndChild(buildTextNode("Format",servicesConf.getFormatList()->at(i)));
	}
	DCPTypeEl = new TiXmlElement( "DCPType" );
	HTTPEl = new TiXmlElement( "HTTP" );
	GetEl = new TiXmlElement( "Get" );
	onlineResourceEl = new TiXmlElement( "OnlineResource" );
	onlineResourceEl->SetAttribute("xmlns:xlink","http://www.w3.org/1999/xlink");
	onlineResourceEl->SetAttribute("xlink:href",pathTag);
	onlineResourceEl->SetAttribute("xlink:type","simple");
	GetEl->LinkEndChild(onlineResourceEl);
	HTTPEl->LinkEndChild(GetEl);
	DCPTypeEl->LinkEndChild(HTTPEl);
	getMapEl->LinkEndChild(DCPTypeEl);

	requestEl->LinkEndChild(getMapEl);

	capabilityEl->LinkEndChild(requestEl);

	//Exception
	TiXmlElement * exceptionEl = new TiXmlElement( "Exception" );
	exceptionEl->LinkEndChild(buildTextNode("Format","XML"));
	capabilityEl->LinkEndChild(exceptionEl);

	// Inspire (extended Capability)
	// TODO : en dur. A mettre dans la configuration du service (prevoir differents profils d'application possibles)
	TiXmlElement * extendedCapabilititesEl = new TiXmlElement("inspire_vs:ExtendedCapabilities");

	// MetadataURL
	TiXmlElement * metadataUrlEl = new TiXmlElement("inspire_common:MetadataUrl");
	metadataUrlEl->LinkEndChild(buildTextNode("inspire_common:URL", "A specifier"));
        extendedCapabilititesEl->LinkEndChild(metadataUrlEl);

	// Languages
	TiXmlElement * supportedLanguagesEl = new TiXmlElement("inspire_common:SupportedLanguages");
	TiXmlElement * defaultLanguageEl = new TiXmlElement("inspire_common:DefaultLanguage");
	TiXmlElement * languageEl = new TiXmlElement("inspire_common:Language");
	TiXmlText * lfre = new TiXmlText("fre");
        languageEl->LinkEndChild(lfre);
	defaultLanguageEl->LinkEndChild(languageEl);
	supportedLanguagesEl->LinkEndChild(defaultLanguageEl);
	extendedCapabilititesEl->LinkEndChild(supportedLanguagesEl);
	// Responselanguage
	TiXmlElement * responseLanguageEl = new TiXmlElement("inspire_common:ResponseLanguage");
	responseLanguageEl->LinkEndChild(buildTextNode("inspire_common:Language","fre"));
	extendedCapabilititesEl->LinkEndChild(responseLanguageEl);
	
	capabilityEl->LinkEndChild(extendedCapabilititesEl);

	// Layer
	if (layerList.empty()){
		LOGGER_ERROR("Liste de layers vide");
		return;
	}
	// Parent layer
	TiXmlElement * parentLayerEl = new TiXmlElement( "Layer" );
	// Title
	parentLayerEl->LinkEndChild(buildTextNode("Title", "cache IGN"));
	// Abstract
	parentLayerEl->LinkEndChild(buildTextNode("Abstract", "Cache IGN"));
	// CRS
	parentLayerEl->LinkEndChild(buildTextNode("CRS", "CRS:84"));

	// Child layers
	std::map<std::string, Layer*>::iterator it;
	for (it=layerList.begin();it!=layerList.end();it++){
		TiXmlElement * childLayerEl = new TiXmlElement( "Layer" );
		Layer* childLayer = it->second;
		// Name
		childLayerEl->LinkEndChild(buildTextNode("Name", childLayer->getId()));
		// Title
		childLayerEl->LinkEndChild(buildTextNode("Title", childLayer->getTitle()));
		// Abstract
		childLayerEl->LinkEndChild(buildTextNode("Abstract", childLayer->getAbstract()));
		// KeywordList
		if (childLayer->getKeyWords().size() != 0){
			TiXmlElement * kwlEl = new TiXmlElement( "KeywordList" );
			for (unsigned int i=0; i < childLayer->getKeyWords().size(); i++){
				kwlEl->LinkEndChild(buildTextNode("Keyword", childLayer->getKeyWords()[i]));
			}
			childLayerEl->LinkEndChild(kwlEl);
		}
		// CRS
		for (unsigned int i=0; i < childLayer->getWMSCRSList().size(); i++){
			childLayerEl->LinkEndChild(buildTextNode("CRS", childLayer->getWMSCRSList()[i]));
		}
		// GeographicBoundingBox
		TiXmlElement * gbbEl = new TiXmlElement( "EX_GeographicBoundingBox");
		std::ostringstream os;
		os<<childLayer->getGeographicBoundingBox().minx;
		gbbEl->LinkEndChild(buildTextNode("westBoundLongitude", os.str()));
		os.str("");
		os<<childLayer->getGeographicBoundingBox().maxx;
		gbbEl->LinkEndChild(buildTextNode("eastBoundLongitude", os.str()));
		os.str("");
		os<<childLayer->getGeographicBoundingBox().miny;
		gbbEl->LinkEndChild(buildTextNode("southBoundLatitude", os.str()));
		os.str("");
		os<<childLayer->getGeographicBoundingBox().maxy;
		gbbEl->LinkEndChild(buildTextNode("northBoundLatitude", os.str()));
		childLayerEl->LinkEndChild(gbbEl);

		// BoundingBox
		TiXmlElement * bbEl = new TiXmlElement( "BoundingBox");
		bbEl->SetAttribute("CRS",childLayer->getBoundingBox().srs);
		bbEl->SetAttribute("minx",childLayer->getBoundingBox().minx);
		bbEl->SetAttribute("miny",childLayer->getBoundingBox().miny);
		bbEl->SetAttribute("maxx",childLayer->getBoundingBox().maxx);
		bbEl->SetAttribute("maxy",childLayer->getBoundingBox().maxy);
		childLayerEl->LinkEndChild(bbEl);
	
		// Scale denominators
		os.str("");
		os<<childLayer->getMinRes()*1000/0.28;
                childLayerEl->LinkEndChild(buildTextNode("MinScaleDenominator", os.str()));
		os.str("");
                os<<childLayer->getMaxRes()*1000/0.28;
                childLayerEl->LinkEndChild(buildTextNode("MaxScaleDenominator", os.str()));

		// TODO : gerer le cas des CRS avec des unites en degres
		
		/* TODO:
		 *
		 layer->getAuthority();
		 layer->getOpaque();
		 layer->getStyles();
		 */
		parentLayerEl->LinkEndChild(childLayerEl);

	}// for layer

	capabilityEl->LinkEndChild(parentLayerEl);


	capabilitiesEl->LinkEndChild(capabilityEl);
	doc.LinkEndChild( capabilitiesEl );

	// std::cout << doc; // ecriture non formatée dans le flux
	// doc.Print();      // affichage formaté sur stdout
	std::string wmsCapaTemplate;
	wmsCapaTemplate << doc;  // ecriture non formatée dans un std::string
	doc.Clear();

	// Découpage en fragments constants.
	size_t beginPos;
	size_t endPos;
	endPos=wmsCapaTemplate.find(hostNameTag);
	wmsCapaFrag.push_back(wmsCapaTemplate.substr(0,endPos));

	beginPos= endPos + hostNameTag.length();
	endPos  = wmsCapaTemplate.find(pathTag, beginPos);
	while(endPos != std::string::npos){
		wmsCapaFrag.push_back(wmsCapaTemplate.substr(beginPos,endPos-beginPos));
		beginPos = endPos + pathTag.length();
		endPos=wmsCapaTemplate.find(pathTag,beginPos);
	}
	wmsCapaFrag.push_back(wmsCapaTemplate.substr(beginPos));

}


void Rok4Server::buildWMTSCapabilities(){
	// std::string hostNameTag="]HOSTNAME[";   ///Tag a remplacer par le nom du serveur
	std::string pathTag="]HOSTNAME/PATH[";  ///Tag à remplacer par le chemin complet avant le ?.
	TiXmlDocument doc;
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", "" );
	doc.LinkEndChild( decl );

	TiXmlElement * capabilitiesEl = new TiXmlElement( "Capabilities" );
	capabilitiesEl->SetAttribute("version","1.0.0");
	// attribut UpdateSequence à ajouter quand on en aura besoin
	capabilitiesEl->SetAttribute("xmlns","http://www.opengis.net/wmts/1.0");
	capabilitiesEl->SetAttribute("xmlns:ows","http://www.opengis.net/ows/1.1");
	capabilitiesEl->SetAttribute("xmlns:xlink","http://www.w3.org/1999/xlink");
	capabilitiesEl->SetAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
	capabilitiesEl->SetAttribute("xmlns:gml","http://www.opengis.net/gml");
	capabilitiesEl->SetAttribute("xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd");

	//----------------------------------------------------------------------
	// ServiceIdentification
	//----------------------------------------------------------------------
	TiXmlElement * serviceEl = new TiXmlElement( "ows:ServiceIdentification" );

	serviceEl->LinkEndChild(buildTextNode("ows:Title", servicesConf.getTitle()));
	serviceEl->LinkEndChild(buildTextNode("ows:Abstract", servicesConf.getAbstract()));
	//KeywordList
	if (servicesConf.getKeyWords().size() != 0){
		TiXmlElement * kwlEl = new TiXmlElement( "ows:Keywords" );
		for (unsigned int i=0; i < servicesConf.getKeyWords().size(); i++){
			kwlEl->LinkEndChild(buildTextNode("ows:Keyword", servicesConf.getKeyWords()[i]));
		}
		serviceEl->LinkEndChild(kwlEl);
	}
	serviceEl->LinkEndChild(buildTextNode("ows:ServiceType", servicesConf.getServiceType()));
	serviceEl->LinkEndChild(buildTextNode("ows:ServiceTypeVersion", servicesConf.getServiceTypeVersion()));
	serviceEl->LinkEndChild(buildTextNode("ows:Fees", servicesConf.getFee()));
	serviceEl->LinkEndChild(buildTextNode("ows:AccessConstraints", servicesConf.getAccessConstraint()));

	capabilitiesEl->LinkEndChild(serviceEl);

	//----------------------------------------------------------------------
	// Le serviceProvider (facultatif) n'est pas implémenté pour le moment.
	//TiXmlElement * servProvEl = new TiXmlElement("ows:ServiceProvider");
	//----------------------------------------------------------------------


	//----------------------------------------------------------------------
	// OperationsMetadata
	//----------------------------------------------------------------------
	TiXmlElement * opMtdEl = new TiXmlElement("ows:OperationsMetadata");
	TiXmlElement * opEl = new TiXmlElement("ows:Operation");
	opEl->SetAttribute("name","GetCapabilities");
	TiXmlElement * dcpEl = new TiXmlElement("ows:DCP");
	TiXmlElement * httpEl = new TiXmlElement("ows:HTTP");
	TiXmlElement * getEl = new TiXmlElement("ows:Get");
	getEl->SetAttribute("xlink:href","]HOSTNAME/PATH[");
	httpEl->LinkEndChild(getEl);
	dcpEl->LinkEndChild(httpEl);
	opEl->LinkEndChild(dcpEl);

	opMtdEl->LinkEndChild(opEl);

	opEl = new TiXmlElement("ows:Operation");
	opEl->SetAttribute("name","GetTile");
	dcpEl = new TiXmlElement("ows:DCP");
	httpEl = new TiXmlElement("ows:HTTP");
	getEl = new TiXmlElement("ows:Get");
	getEl->SetAttribute("xlink:href","]HOSTNAME/PATH[");
	httpEl->LinkEndChild(getEl);
	dcpEl->LinkEndChild(httpEl);
	opEl->LinkEndChild(dcpEl);

	opMtdEl->LinkEndChild(opEl);

	capabilitiesEl->LinkEndChild(opMtdEl);
	//----------------------------------------------------------------------
	// Contents
	//----------------------------------------------------------------------
	TiXmlElement * contentsEl=new TiXmlElement("Contents");

	// Layer
	//------------------------------------------------------------------
	std::map<std::string, Layer*>::iterator itLay(layerList.begin()), itLayEnd(layerList.end());
	for (;itLay!=itLayEnd;++itLay){
		TiXmlElement * layerEl=new TiXmlElement("Layer");
		Layer* layer = itLay->second;

		layerEl->LinkEndChild(buildTextNode("ows:Title", layer->getTitle()));
		layerEl->LinkEndChild(buildTextNode("ows:Abstract", layer->getAbstract()));
		if (layer->getKeyWords().size() != 0){
			TiXmlElement * kwlEl = new TiXmlElement( "ows:Keywords" );
			for (unsigned int i=0; i < layer->getKeyWords().size(); i++){
				kwlEl->LinkEndChild(buildTextNode("ows:Keyword", layer->getKeyWords()[i]));
			}
			layerEl->LinkEndChild(kwlEl);
		}
		//TODO: ows:WGS84BoundingBox (0,n)
		layerEl->LinkEndChild(buildTextNode("ows:Identifier", layer->getId()));

		//FIXME: La gestion des styles reste à faire entièrement dans la conf.
		//       Je me contente d'un minimum pour que le capabilities soit valide.
		if (layer->getStyles().size() != 0){
			for (unsigned int i=0; i < layer->getStyles().size(); i++){
				TiXmlElement * styleEl= new TiXmlElement("Style");
				if (i==0) styleEl->SetAttribute("isDefault","true");
				styleEl->LinkEndChild(buildTextNode("ows:Identifier", layer->getStyles()[i]));
				layerEl->LinkEndChild(styleEl);
			}
		}

		// on pourrait avoir plusieurs formats différents par layer si on utilise plusieurs pyramides par layer.
		std::vector<std::string> formats = layer->getMimeFormats();
		for (unsigned int i=0; i < formats.size(); i++){
			layerEl->LinkEndChild(buildTextNode("Format",formats[i]));
		}

		/* on suppose qu'on a qu'un TMS par layer parce que si on admet avoir un TMS par pyramide
		 *  il faudra contrôler la cohérence entre le format, la projection et le TMS... */
		TiXmlElement * tmsLinkEl = new TiXmlElement("TileMatrixSetLink");
		tmsLinkEl->LinkEndChild(buildTextNode("TileMatrixSet",layer->getPyramids()[0]->getTms().getId()));
		layerEl->LinkEndChild(tmsLinkEl);

		contentsEl->LinkEndChild(layerEl);
	}

	// TileMatrixSet
	//--------------------------------------------------------
	std::map<std::string,TileMatrixSet*>::iterator itTms(tmsList.begin()), itTmsEnd(tmsList.end());
	for (;itTms!=itTmsEnd;++itTms){
		TiXmlElement * tmsEl=new TiXmlElement("TileMatrixSet");
		TileMatrixSet* tms = itTms->second;
		tmsEl->LinkEndChild(buildTextNode("ows:Identifier",tms->getId()));
		tmsEl->LinkEndChild(buildTextNode("ows:SupportedCRS",tms->getCrs().getRequestCode()));
		std::map<std::string, TileMatrix>* tmList = tms->getTmList();

		// TileMatrix
		std::map<std::string, TileMatrix>::iterator itTm(tmList->begin()), itTmEnd(tmList->end());
		for (;itTm!=itTmEnd;++itTm){
			TileMatrix tm =itTm->second;
			TiXmlElement * tmEl=new TiXmlElement("TileMatrix");
			tmEl->LinkEndChild(buildTextNode("ows:Identifier",tm.getId()));
			tmEl->LinkEndChild(buildTextNode("ScaleDenominator",numToStr(tm.getRes()/0.00028)));
			tmEl->LinkEndChild(buildTextNode("TopLeftCorner",numToStr(tm.getX0()) + " " + numToStr(tm.getY0())));
			tmEl->LinkEndChild(buildTextNode("TileWidth",numToStr(tm.getTileW())));
			tmEl->LinkEndChild(buildTextNode("TileHeight",numToStr(tm.getTileH())));
			tmEl->LinkEndChild(buildTextNode("MatrixWidth",numToStr(tm.getMatrixW())));
			tmEl->LinkEndChild(buildTextNode("MatrixHeight",numToStr(tm.getMatrixH())));
			tmsEl->LinkEndChild(tmEl);
		}
		contentsEl->LinkEndChild(tmsEl);
	}

	capabilitiesEl->LinkEndChild(contentsEl);

	doc.LinkEndChild( capabilitiesEl );

	//doc.Print();      // affichage formaté sur stdout

	std::string wmtsCapaTemplate;
	wmtsCapaTemplate << doc;  // ecriture non formatée dans un std::string
	doc.Clear();

	// Découpage en fragments constants.
	size_t beginPos;
	size_t endPos;
	beginPos= 0;
	endPos  = wmtsCapaTemplate.find(pathTag);
	while(endPos != std::string::npos){
		wmtsCapaFrag.push_back(wmtsCapaTemplate.substr(beginPos,endPos-beginPos));
		beginPos = endPos + pathTag.length();
		endPos=wmtsCapaTemplate.find(pathTag,beginPos);
	}
	wmtsCapaFrag.push_back(wmtsCapaTemplate.substr(beginPos));

	/*//debug: affichage des fragments.
		for (int i=0; i<wmtsCapaFrag.size();i++){
		std::cout << "(" << wmtsCapaFrag[i] << ")" << std::endl;
		}
		*/

}
