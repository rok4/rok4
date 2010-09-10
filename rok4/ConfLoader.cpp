#include <dirent.h>
#include "ConfLoader.h"
#include "Pyramid.h"
#include "tinyxml.h"
#include "tinystr.h"
#include "config.h"

TileMatrixSet* buildTileMatrixSet(std::string fileName){
	LOGGER_DEBUG("=>buildTileMatrixSet");

	std::string id;
	std::string title="";
	std::string abstract="";
	std::vector<std::string> keyWords;
	std::string crs;
	std::map<std::string, TileMatrix> listTM;

	TiXmlDocument doc(fileName.c_str());
	if (!doc.LoadFile()){
		LOGGER_ERROR("Ne peut pas charger le fichier " << fileName);
		return NULL;
	}

	TiXmlHandle hDoc(&doc);
	TiXmlElement* pElem;
	TiXmlHandle hRoot(0);

	pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
	if (!pElem){
		LOGGER_ERROR(fileName << " impossible de recuperer la racine.");
		return NULL;
	}
	if (strcmp(pElem->Value(),"tileMatrixSet")){
		LOGGER_ERROR(fileName << " La racine n'est pas un tileMatrixSet.");
		return NULL;
	}
	hRoot=TiXmlHandle(pElem);

	unsigned int idBegin=fileName.rfind("/");
	if (idBegin == std::string::npos){
		idBegin=0;
	}
	unsigned int idEnd=fileName.rfind(".tms");
	if (idEnd == std::string::npos){
		idEnd=fileName.rfind(".TMS");
		if (idEnd == std::string::npos){
			idEnd=fileName.size();
		}
	}
	id=fileName.substr(idBegin+1, idEnd-idBegin-1);

	pElem=hRoot.FirstChild("crs").Element();
	if (!pElem){
		LOGGER_ERROR("TileMaxtrixSet " << id <<" pas de crs!!");
		return NULL;
	}
	crs = pElem->GetText();
	//FIXME: controle et normalisation du nom du CRS à faire

	pElem=hRoot.FirstChild("abstract").Element();
	if (pElem) abstract = pElem->GetText();

	pElem=hRoot.FirstChild("title").Element();
	if (pElem) title = pElem->GetText();


	for (pElem=hRoot.FirstChild("keywordList").FirstChild("keyword").Element(); pElem; pElem=pElem->NextSiblingElement("keyword")){
		std::string keyword(pElem->GetText());
		keyWords.push_back(keyword);
	}

	for( pElem=hRoot.FirstChild( "tileMatrix" ).Element(); pElem; pElem=pElem->NextSiblingElement( "tileMatrix")){
		std::string tmId;
		double res;
		double x0;
		double y0;
		int tileW;
		int tileH;
		long int matrixW;
		long int matrixH;

		TiXmlHandle hTM(pElem);
		TiXmlElement* pElemTM = hTM.FirstChild("id").Element();
		if (!pElemTM){LOGGER_ERROR("TileMaxtrixSet " << id <<", TileMatrix sans id!!"); return NULL; }
		tmId=pElemTM->GetText();

		pElemTM = hTM.FirstChild("resolution").Element();
		if (!pElemTM){LOGGER_ERROR("TileMaxtrixSet " << id <<" tileMatrix " << tmId <<" sans resolution!!"); return NULL; }
		if (!sscanf(pElemTM->GetText(),"%lf",&res)){
			LOGGER_ERROR("TileMaxtrixSet " << id <<", TileMaxtrix " << tmId <<": La resolution est inexploitable.");
			return NULL;
		}

		pElemTM = hTM.FirstChild("topLeftCornerX").Element();
		if (!pElemTM){LOGGER_ERROR("TileMaxtrixSet " << id <<" tileMatrix " << tmId <<" sans topLeftCornerX!!"); return NULL; }
		if (!sscanf(pElemTM->GetText(),"%lf",&x0)){
			LOGGER_ERROR("TileMaxtrixSet " << id <<", TileMaxtrix " << tmId <<": Le topLeftCornerX est inexploitable.");
			return NULL;
		}

		pElemTM = hTM.FirstChild("topLeftCornerY").Element();
		if (!pElemTM){LOGGER_ERROR("TileMaxtrixSet " << id <<" tileMatrix " << tmId <<" sans topLeftCornerY!!"); return NULL; }
		if (!sscanf(pElemTM->GetText(),"%lf",&y0)){
			LOGGER_ERROR("TileMaxtrixSet " << id <<", TileMaxtrix " << tmId <<": Le topLeftCornerY est inexploitable.");
			return NULL;
		}

		pElemTM = hTM.FirstChild("tileWidth").Element();
		if (!pElemTM){LOGGER_ERROR("TileMaxtrixSet " << id <<" tileMatrix " << tmId <<" sans tileWidth!!"); return NULL; }
		if (!sscanf(pElemTM->GetText(),"%d",&tileW)){
			LOGGER_ERROR("TileMaxtrixSet " << id <<", TileMaxtrix " << tmId <<": Le tileWidth est inexploitable.");
			return NULL;
		}

		pElemTM = hTM.FirstChild("tileHeight").Element();
		if (!pElemTM){LOGGER_ERROR("TileMaxtrixSet " << id <<" tileMatrix " << tmId <<" sans tileHeight!!"); return NULL; }
		if (!sscanf(pElemTM->GetText(),"%d",&tileH)){
			LOGGER_ERROR("TileMaxtrixSet " << id <<", TileMaxtrix " << tmId <<": Le tileHeight est inexploitable.");
			return NULL;
		}

		pElemTM = hTM.FirstChild("matrixWidth").Element();
		if (!pElemTM){LOGGER_ERROR("TileMaxtrixSet " << id <<" tileMatrix " << tmId <<" sans MatrixWidth!!"); return NULL; }
		if (!sscanf(pElemTM->GetText(),"%ld",&matrixW)){
			LOGGER_ERROR("TileMaxtrixSet " << id <<", TileMaxtrix " << tmId <<": Le MatrixWidth est inexploitable.");
			return NULL;
		}

		pElemTM = hTM.FirstChild("matrixHeight").Element();
		if (!pElemTM){LOGGER_ERROR("TileMaxtrixSet " << id <<" tileMatrix " << tmId <<" sans matrixHeight!!"); return NULL; }
		if (!sscanf(pElemTM->GetText(),"%ld",&matrixH)){
			LOGGER_ERROR("TileMaxtrixSet " << id <<", tileMaxtrix " << tmId <<": Le matrixHeight est inexploitable.");
			return NULL;
		}

		TileMatrix tm(tmId, res, x0, y0, tileW, tileH, matrixW, matrixH);
		listTM.insert(std::pair<std::string, TileMatrix> (tmId, tm));

	}// boucle sur le TileMatrix

	if (listTM.size()==0){
		LOGGER_ERROR("Aucun tileMatrix trouvé dans le tileMatrixSet" << id <<" : il est invalide!!");
		return NULL;
	}

	TileMatrixSet * tms = new TileMatrixSet(id,title,abstract,keyWords,crs,listTM);
	LOGGER_DEBUG("<=buildTileMatrixSet");
	return tms;

}//buildTileMatrixSet(std::string)

Pyramid * buildPyramid(std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList){
	LOGGER_DEBUG("=>buildPyramid");
	TileMatrixSet *tms;
	std::map<std::string, Level *> levels;

	TiXmlDocument doc(fileName.c_str());
	if (!doc.LoadFile()){
		LOGGER_ERROR("Ne peut pas charger le fichier " << fileName);
		return NULL;
	}

	TiXmlHandle hDoc(&doc);
	TiXmlElement* pElem;
	TiXmlHandle hRoot(0);

	pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
	if (!pElem){
		LOGGER_ERROR(fileName << " impossible de recuperer la racine.");
		return NULL;
	}
	if (strcmp(pElem->Value(),"Pyramid")){
		LOGGER_ERROR(fileName << " La racine n'est pas une Pyramid.");
		return NULL;
	}
	hRoot=TiXmlHandle(pElem);

	pElem=hRoot.FirstChild("tileMatrixSet").Element();
	if (!pElem){
		LOGGER_ERROR(fileName << "La pyramide n'a pas de TMS. C'est un problème.");
		return NULL;
	}
	std::string tmsName=pElem->GetText();
	std::map<std::string, TileMatrixSet *>::iterator it;
	it=tmsList.find(tmsName);
	if(it == tmsList.end()){
		LOGGER_ERROR("La Pyramide ["<< fileName <<"] reference un TMS ["<< tmsName <<"] qui n'existe pas.");
		return NULL;
	}
	tms=it->second;


	for( pElem=hRoot.FirstChild( "level" ).Element(); pElem; pElem=pElem->NextSiblingElement( "level")){
		TileMatrix *tm;
		std::string id;
		std::string format;
		std::string baseDir;
		int channels;
		int32_t minTileRow=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignée.
		int32_t maxTileRow=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignée.
		int32_t minTileCol=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignée.
		int32_t maxTileCol=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignée.
		int tilesPerWidth;
		int tilesPerHeight;
		int pathDepth;

		TiXmlHandle hLvl(pElem);
		TiXmlElement* pElemLvl = hLvl.FirstChild("tileMatrix").Element();
		if (!pElemLvl){LOGGER_ERROR(fileName <<" level "<<id<<" sans tileMatrix!!"); return NULL; }
		std::string tmName(pElemLvl->GetText());
		id=tmName;
		std::map<std::string, TileMatrix>::iterator it = tms->tmList.find(tmName);
		if(it == tms->tmList.end()){
			LOGGER_ERROR(fileName <<" Le level "<< id <<" ref. Le TM [" << tmName << "] qui n'appartient pas au TMS [" << tmsName << "]");
			return NULL;
		}
		tm = &(it->second);

		pElemLvl = hLvl.FirstChild("baseDir").Element();
   		if (!pElemLvl){LOGGER_ERROR(fileName <<" Level "<< id <<" sans baseDir!!"); return NULL; }
   		baseDir=pElemLvl->GetText();

		pElemLvl = hLvl.FirstChild("format").Element();
			if (!pElemLvl){LOGGER_ERROR(fileName <<" Level "<< id <<" sans format!!"); return NULL; }
			format=pElemLvl->GetText(); // FIXME: controle de la valeur a faire

		pElemLvl = hLvl.FirstChild("channels").Element();
   		if (!pElemLvl){
   			LOGGER_ERROR(fileName <<" Level "<< id << " Pas de channels => channels = " << DEFAULT_CHANNELS);
   			channels=DEFAULT_CHANNELS;
   		}else if (!sscanf(pElemLvl->GetText(),"%d",&channels)){
   			LOGGER_ERROR(fileName <<" Level "<< id <<": channels=[" << pElemLvl->GetText() <<"] n'est pas un entier.");
   			return NULL;
   		}

	    pElemLvl = hLvl.FirstChild("tilesPerWidth").Element();
   		if (!pElemLvl){
   			LOGGER_ERROR(fileName <<" Level "<< id << ": Pas de tilesPerWidth !!");
   			return NULL;
   		}
   		if (!sscanf(pElemLvl->GetText(),"%d",&tilesPerWidth)){
   			LOGGER_ERROR(fileName <<" Level "<< id <<": tilesPerWidth=[" << pElemLvl->GetText() <<"] n'est pas un entier.");
   			return NULL;
   		}

	    pElemLvl = hLvl.FirstChild("tilesPerHeight").Element();
   		if (!pElemLvl){
   			LOGGER_ERROR(fileName <<" Level "<< id << ": Pas de tilesPerHeight !!");
   			return NULL;
   		}
   		if (!sscanf(pElemLvl->GetText(),"%d",&tilesPerHeight)){
   			LOGGER_ERROR(fileName <<" Level "<< id <<": tilesPerHeight=[" << pElemLvl->GetText() <<"] n'est pas un entier.");
   			return NULL;
   		}

		pElemLvl = hLvl.FirstChild("pathDepth").Element();
		if (!pElemLvl){
			LOGGER_ERROR(fileName <<" Level "<< id << ": Pas de pathDepth !!");
			return NULL;
		}
		if (!sscanf(pElemLvl->GetText(),"%d",&pathDepth)){
			LOGGER_ERROR(fileName <<" Level "<< id <<": pathDepth=[" << pElemLvl->GetText() <<"] n'est pas un entier.");
			return NULL;
		}

		TiXmlElement* pElemTMSL=hRoot.FirstChild( "TMSLimits" ).Element();
		if (pElemTMSL){ // le bloc TMSLimits n'est pas obligatoire, mais s'il est là, il doit y avoir tous les champs.
			TiXmlHandle hTMSL(pElem);
			TiXmlElement* pElemTMSL = hTMSL.FirstChild("minTileRow").Element();
			if (!pElemTMSL){
				LOGGER_ERROR(fileName <<" Level "<< id << ": Pas de minTileRow dans le bloc TMSLimits !!");
				return NULL;
			}
			if (!sscanf(pElemTMSL->GetText(),"%d",&minTileRow)){
				LOGGER_ERROR(fileName <<" Level "<< id <<": minTileRow=[" << pElemTMSL->GetText() <<"] n'est pas un entier.");
				return NULL;
			}

			pElemTMSL = hTMSL.FirstChild("maxTileRow").Element();
			if (!pElemTMSL){
				LOGGER_ERROR(fileName <<" Level "<< id << ": Pas de maxTileRow dans le bloc TMSLimits !!");
				return NULL;
			}
			if (!sscanf(pElemTMSL->GetText(),"%d",&maxTileRow)){
				LOGGER_ERROR(fileName <<" Level "<< id <<": maxTileRow=[" << pElemTMSL->GetText() <<"] n'est pas un entier.");
				return NULL;
			}

			pElemTMSL = hTMSL.FirstChild("minTileCol").Element();
			if (!pElemTMSL){
				LOGGER_ERROR("Level "<< id << ": Pas de minTileCol dans le bloc TMSLimits !!");
				return NULL;
			}
			if (!sscanf(pElemTMSL->GetText(),"%d",&minTileCol)){
				LOGGER_ERROR(fileName <<" Level "<< id <<": minTileCol=[" << pElemTMSL->GetText() <<"] n'est pas un entier.");
				return NULL;
			}

			pElemTMSL = hTMSL.FirstChild("maxTileCol").Element();
			if (!pElemTMSL){
				LOGGER_ERROR(fileName <<" Level "<< id << ": Pas de maxTileCol dans le bloc TMSLimits !!");
				return NULL;
			}
			if (!sscanf(pElemTMSL->GetText(),"%d",&maxTileCol)){
				LOGGER_ERROR(fileName <<" Level "<< id <<": maxTileCol=[" << pElemTMSL->GetText() <<"] n'est pas un entier.");
				return NULL;
			}
		}

		Level *TL = new Level(*tm, channels, baseDir, tilesPerWidth, tilesPerHeight,
									maxTileRow,  minTileRow, maxTileCol, minTileCol, pathDepth,format);
		levels.insert(std::pair<std::string, Level *> (id, TL));
	}// boucle sur les levels

	if (levels.size()==0){
		LOGGER_ERROR("Aucun level n'a pu être chargé pour la pyramide "<< fileName);
		return NULL;
	}

	Pyramid *pyr = new Pyramid(levels, *tms);
	LOGGER_DEBUG("=>buildPyramid");
	return pyr;


}// buildPyramid()

Layer * buildLayer(std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList){
	LOGGER_DEBUG("=> buildLayer");
	std::string id;
	std::string title="";
	std::string abstract="";
	std::vector<std::string> keyWords;
	std::string style;
	std::vector<std::string> styles;
	double minRes;
	double maxRes;
	std::vector<std::string> WMSCRSList;
	bool opaque;
	std::string authority="";
	std::string resampling;
	std::vector<Pyramid*> pyramids;

	TiXmlDocument doc(fileName.c_str());
	if (!doc.LoadFile()){
		LOGGER_ERROR("Ne peut pas charger le fichier " << fileName);
		return NULL;
	}

	TiXmlHandle hDoc(&doc);
	TiXmlElement* pElem;
	TiXmlHandle hRoot(0);

	pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
	if (!pElem){
		LOGGER_ERROR(fileName << " impossible de recuperer la racine.");
		return NULL;
	}
	if (strcmp(pElem->Value(),"layer")){
		LOGGER_ERROR(fileName << " La racine n'est pas un layer.");
		return NULL;
	}
	hRoot=TiXmlHandle(pElem);

	unsigned int idBegin=fileName.rfind("/");
	if (idBegin == std::string::npos){
		idBegin=0;
	}
	unsigned int idEnd=fileName.rfind(".lay");
	if (idEnd == std::string::npos){
		idEnd=fileName.rfind(".LAY");
		if (idEnd == std::string::npos){
			idEnd=fileName.size();
		}
	}
	id=fileName.substr(idBegin+1, idEnd-idBegin-1);

	pElem=hRoot.FirstChild("title").Element();
	if (pElem) title=pElem->GetText();

	pElem=hRoot.FirstChild("abstract").Element();
	if (pElem) abstract=pElem->GetText();


	for (pElem=hRoot.FirstChild("keywordList").FirstChild("keyword").Element(); pElem; pElem=pElem->NextSiblingElement("keyword")){
		std::string keyword(pElem->GetText());
		keyWords.push_back(keyword);
	}

	pElem=hRoot.FirstChild("style").Element();
	if (!pElem){
		LOGGER_ERROR("Pas de style => style = " << DEFAULT_STYLE);
		style = DEFAULT_STYLE;
	}else{
		style = pElem->GetText();
	}
	styles.push_back(style);

	pElem = hRoot.FirstChild("minRes").Element();
	if (!pElem){
		minRes=0; //convention pour non contraint...
	}else if (!sscanf(pElem->GetText(),"%lf",&minRes)){
		LOGGER_ERROR("La resolution min est inexploitable:[" << pElem->GetText() << "]");
		return NULL;
	}

	pElem = hRoot.FirstChild("maxRes").Element();
	if (!pElem){
		maxRes=0; //convention pour non contraint...
	}else if (!sscanf(pElem->GetText(),"%lf",&maxRes)){
		LOGGER_ERROR("La resolution max est inexploitable:[" << pElem->GetText() << "]");
		return NULL;
	}


	for (pElem=hRoot.FirstChild("WMSCRSList").FirstChild("WMSCRS").Element(); pElem; pElem=pElem->NextSiblingElement("WMSCRS")){
		std::string crs(pElem->GetText());
		WMSCRSList.push_back(crs);
	}
	if (WMSCRSList.size()==0){
		LOGGER_WARN(fileName << ": Aucun CRS autorisé pour le WMS");
	}

    pElem=hRoot.FirstChild("opaque").Element();
    if (!pElem){
      LOGGER_ERROR("Pas de opaque => opaque = " << DEFAULT_OPAQUE);
      opaque = DEFAULT_OPAQUE;
    }else{
      std::string opaStr=pElem->GetText();
      if (opaStr=="true"){
        opaque = true;
      }else if(opaStr=="false"){
        opaque = false;
      }else{
        LOGGER_ERROR("le param opaque n'est pas exploitable:[" << opaStr <<"]");
        return NULL;
      }
    }

	pElem=hRoot.FirstChild("authority").Element();
	if (pElem) authority=pElem->GetText();

	pElem=hRoot.FirstChild("resampling").Element();
	if (!pElem){
		LOGGER_ERROR("Pas de resampling => resampling = " << DEFAULT_RESAMPLING);
		resampling = DEFAULT_RESAMPLING;
	}else{
		resampling = pElem->GetText();
	}


	for (pElem=hRoot.FirstChild("pyramidList").FirstChild("pyramid").Element(); pElem; pElem=pElem->NextSiblingElement("pyramid")){
		Pyramid * pyramid = buildPyramid(pElem->GetText(), tmsList);
		if (!pyramid){
			LOGGER_ERROR("La pyramide " << pElem->GetText() << " ne peut être chargée");
			//FIXME: que faut-il faire des pyramides déjà créées? Faut-il les detruire? ou vector s'en charge?
			return NULL;
		}
		pyramids.push_back(pyramid);
	}
	if (pyramids.size()==0){
		// FIXME: pas forcément critique si on a un cache d'une autre nature (jpeg2000 par exemple).
		LOGGER_ERROR("Aucune pyramide associé au layer "<< fileName);
		return NULL;
	}

    Layer *layer;

	layer = new Layer(id, title, abstract, keyWords, pyramids, styles, minRes, maxRes,
			         WMSCRSList, opaque, authority, resampling);

	LOGGER_DEBUG("<= buildLayer");

	return layer;
}//buildLayer

bool ConfLoader::getTechnicalParam(int &nbThread, std::string &layerDir, std::string &tmsDir){
	LOGGER_DEBUG("=>getTechnicalParam");
	TiXmlDocument doc(SERVER_CONF_PATH);
	if (!doc.LoadFile()){
		LOGGER_ERROR("Ne peut pas charger le fichier " << SERVER_CONF_PATH);
		return false;
	}

	TiXmlHandle hDoc(&doc);
	TiXmlElement* pElem;
	TiXmlHandle hRoot(0);

	pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
	if (!pElem){
		LOGGER_ERROR(SERVER_CONF_PATH << " impossible de recuperer la racine.");
		return false;
	}
	if (strcmp(pElem->Value(), "serverConf")){
		LOGGER_ERROR(SERVER_CONF_PATH << " La racine n'est pas un serverConf.");
		return false;
	}
	hRoot=TiXmlHandle(pElem);

	pElem=hRoot.FirstChild("nbThread").Element();
	if (!pElem){
		LOGGER_ERROR("Pas de nbThread => nbThread = " << DEFAULT_NB_THREAD);
		nbThread = DEFAULT_NB_THREAD;
	}else if (!sscanf(pElem->GetText(),"%d",&nbThread)){
		LOGGER_ERROR("Le nbThread [" << pElem->GetText() <<"] n'est pas un entier.");
		return false;
	}

	pElem=hRoot.FirstChild("layerDir").Element();
	if (!pElem){
		LOGGER_ERROR("Pas de layerDir => layerDir = " << DEFAULT_LAYER_DIR);
		layerDir = DEFAULT_LAYER_DIR;
	}else{
		layerDir=pElem->GetText();
	}

	pElem=hRoot.FirstChild("tileMatrixSetDir").Element();
	if (!pElem){
		LOGGER_ERROR("Pas de tileMatrixSetDir => tileMatrixSetDir = " << DEFAULT_TMS_DIR);
		tmsDir = DEFAULT_TMS_DIR;
	}else{
		tmsDir=pElem->GetText();
	}

	LOGGER_DEBUG("<=getTechnicalParam");
    return true;
}//getTechnicalParam


bool ConfLoader::buildTMSList(std::string tmsDir,std::map<std::string, TileMatrixSet*> &tmsList){
	LOGGER_DEBUG("=>buildTMSList");

	// lister les fichier du répertoire tmsDir
	std::vector<std::string> tmsFiles;
	std::string tmsFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ((dir = opendir(tmsDir.c_str())) == NULL){
        LOGGER_FATAL("Le répertoire des TMS " << tmsDir << " n'est pas accessible.");
        return false;
    }
    while ((fileEntry = readdir(dir))){
       tmsFileName = fileEntry->d_name;
       if(tmsFileName.rfind(".tms")==tmsFileName.size()-4){
    	   tmsFiles.push_back(tmsDir+"/"+tmsFileName);
       }
    }
    closedir(dir);

    if (tmsFiles.empty()){
    	// FIXME:
    	// Aucun TMS présents. Ce n'est pas nécessairement grave si le serveur
    	// ne sert pas pour le WMTS et qu'on exploite pas de cache tuilé.
    	// Cependant pour le moment (07/2010) on ne gère que des caches tuilés
    	LOGGER_FATAL("Aucun fichier *.tms dans le répertoire " << tmsDir);
    	return false;
    }

    // générer les TMS décrits par les fichiers.
    for (unsigned int i=0; i<tmsFiles.size(); i++){
        TileMatrixSet * tms;
        tms = buildTileMatrixSet(tmsFiles[i]);
        if (tms){
     	    tmsList.insert( std::pair<std::string, TileMatrixSet *> (tms->getId(), tms));
        }else{
        	LOGGER_ERROR("Ne peut charger le tms: " << tmsFiles[i]);
        }
    }

    if (tmsList.size()==0){
        LOGGER_FATAL("Aucun TMS n'a pu être chargé!");
        return false;
    }

    LOGGER_DEBUG("<=buildTMSList");
    return true;
}

bool ConfLoader::buildLayersList(std::string layerDir,std::map<std::string, TileMatrixSet*> &tmsList, std::map<std::string,Layer*> &layers){
	LOGGER_DEBUG("=>buildLayersList");
	// lister les fichier du répertoire layerDir
	std::vector<std::string> layerFiles;
	std::string layerFileName;
    struct dirent *fileEntry;
    DIR *dir;
    if ((dir = opendir(layerDir.c_str())) == NULL){
        LOGGER_FATAL("Le répertoire " << layerDir << " n'est pas accessible.");
        return false;
    }
    while ((fileEntry = readdir(dir))){
       layerFileName = fileEntry->d_name;
       if(layerFileName.rfind(".lay")==layerFileName.size()-4){
    	   layerFiles.push_back(layerDir+"/"+layerFileName);
       }
    }
    closedir(dir);

    if (layerFiles.empty()){
    	LOGGER_FATAL("Aucun fichier *.lay dans le répertoire " << layerDir);
    	LOGGER_FATAL("Le serveur n'a aucune données à servir. Domage...");
    	return false;
    }

    // générer les Layers décrits par les fichiers.
    for (unsigned int i=0; i<layerFiles.size(); i++){
        Layer * layer;
        layer = buildLayer(layerFiles[i], tmsList);
        if (layer){
    	    layers.insert( std::pair<std::string, Layer *> (layer->getId(), layer));
        }else{
        	LOGGER_ERROR("Ne peut charger le layer: " << layerFiles[i]);
        }
    }

    if (layers.size()==0){
        LOGGER_FATAL("Aucun layer n'a pu être chargé!");
        return false;
    }

	LOGGER_DEBUG("<=buildLayersList");
    return true;
}

ServicesConf * ConfLoader::buildServicesConf(){
	LOGGER_DEBUG("=> buildServicesConf");
	TiXmlDocument doc(SERVICES_CONF_PATH);
	if (!doc.LoadFile()){
		LOGGER_ERROR("Ne peut pas charger le fichier " << SERVICES_CONF_PATH);
		return NULL;
	}

	TiXmlHandle hDoc(&doc);
	TiXmlElement* pElem;
	TiXmlHandle hRoot(0);

	pElem=hDoc.FirstChildElement().Element(); //recuperation de la racine.
	if (!pElem){
		LOGGER_ERROR(SERVICES_CONF_PATH << " impossible de recuperer la racine.");
		return NULL;
	}
	if (pElem->ValueStr() != "servicesConf"){
		LOGGER_ERROR(SERVICES_CONF_PATH << " La racine n'est pas un servicesConf.");
		return NULL;
	}
	hRoot=TiXmlHandle(pElem);

	std::string name="";
	std::string title="";
	std::string abstract="";
	std::vector<std::string> keyWords;
	std::string serviceProvider;
	std::string fee;
	std::string accessConstraint;
	unsigned int maxWidth;
	unsigned int maxHeight;
	std::vector<std::string> formatList;


	pElem=hRoot.FirstChild("name").Element();
	if (pElem) name = pElem->GetText();

	pElem=hRoot.FirstChild("title").Element();
	if (pElem) title = pElem->GetText();

	pElem=hRoot.FirstChild("abstract").Element();
	if (pElem) abstract = pElem->GetText();


	for (pElem=hRoot.FirstChild("keywordList").FirstChild("keyword").Element(); pElem; pElem=pElem->NextSiblingElement("keyword")){
		std::string keyword(pElem->GetText());
		keyWords.push_back(keyword);
	}

	pElem=hRoot.FirstChild("serviceProvider").Element();
	if (pElem) serviceProvider = pElem->GetText();

	pElem=hRoot.FirstChild("fee").Element();
	if (pElem) fee = pElem->GetText();

	pElem=hRoot.FirstChild("accessConstraint").Element();
	if (pElem) accessConstraint = pElem->GetText();

	pElem = hRoot.FirstChild("maxWidth").Element();
	if (!pElem){
		maxWidth=MAX_IMAGE_WIDTH;
	}else if (!sscanf(pElem->GetText(),"%d",&maxWidth)){
		LOGGER_ERROR(SERVICES_CONF_PATH << "Le maxWidth est inexploitable:[" << pElem->GetText() << "]");
		return NULL;
	}

	pElem = hRoot.FirstChild("maxHeight").Element();
	if (!pElem){
		maxHeight=MAX_IMAGE_HEIGHT;
	}else if (!sscanf(pElem->GetText(),"%d",&maxHeight)){
		LOGGER_ERROR(SERVICES_CONF_PATH << "Le maxHeight est inexploitable:[" << pElem->GetText() << "]");
		return false;
	}

	for (pElem=hRoot.FirstChild("formatList").FirstChild("format").Element(); pElem; pElem=pElem->NextSiblingElement("format")){
		std::string format(pElem->GetText());
		formatList.push_back(format);
	}


	ServicesConf * servicesConf;
	servicesConf = new ServicesConf(name, title, abstract, keyWords,serviceProvider, fee,
			                       accessConstraint, maxWidth, maxHeight, formatList);

	LOGGER_DEBUG("<= buildServicesConf");
    return servicesConf;
}
