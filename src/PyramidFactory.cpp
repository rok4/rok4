#include "PyramidFactory.h"
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/xmlschemas.h>

enum {
    ERROR_OCCURED = -1, // Une erreur est survenue pendant la validation: pas d'info sur le document
    NOT_VALID = 0,      // Le document n'est pas valide
    VALID = 1           // Le document est valide
};


/**
 * Fonction de validation d'un arbre DOM à l'aide d'un XML Schema
 **/
int PyramidFactory::validation_schema(xmlDocPtr doc, const char *xml_schema, bool afficher_erreurs) {
    int ret;
    xmlSchemaPtr schema;
    xmlSchemaValidCtxtPtr vctxt;
    xmlSchemaParserCtxtPtr pctxt;

    // Ouverture du fichier XML Schema
    if ((pctxt = xmlSchemaNewParserCtxt(xml_schema)) == NULL) {
    	LOGGER_ERROR("Impossible d'ouvrir le schema du fichier de conf de pyramide: " << xml_schema);
        return ERROR_OCCURED;
    }
    // Chargement du contenu du XML Schema
    schema = xmlSchemaParse(pctxt);
    xmlSchemaFreeParserCtxt(pctxt);
    if (schema == NULL) {
    	LOGGER_ERROR("Impossible de charger le schema xml du fichier de conf de pyramide: " << xml_schema);
        return ERROR_OCCURED;
    }
    // Création du contexte de validation
    if ((vctxt = xmlSchemaNewValidCtxt(schema)) == NULL) {
    	LOGGER_ERROR("Pb a la creation du contexte de validation: " << xml_schema);
        xmlSchemaFree(schema);
        return ERROR_OCCURED;
    }
    // Traitement des erreurs de validation
    if (afficher_erreurs) {
        xmlSchemaSetValidErrors(vctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
    }
    // Validation
    ret = (xmlSchemaValidateDoc(vctxt, doc) == 0 ? VALID : NOT_VALID);
    // Libération de la mémoire
    xmlSchemaFree(schema);
    xmlSchemaFreeValidCtxt(vctxt);

    return ret;
}

/** Construit une Pyramid avec tous ses Level à partir du fichier de config.
 *  retourne NULL en cas d'erreur.
 */
Pyramid* PyramidFactory::make(std::string configFileName) {
	xmlDocPtr doc;
	doc = xmlParseFile(configFileName.c_str());
	if (doc == NULL) {
        LOGGER_ERROR("Impossible de charger le fichier de conf de pyramide:" << configFileName);
        return NULL;
    }

	// validation du fichier de conf (conformité au schéma)
	switch (validation_schema(doc, "../config/pyramid.xsd", true)){
	case ERROR_OCCURED:
		// on a pas pu faire le contrôle (xsd perdu?)
		LOGGER_WARN("Le fichier de config " << configFileName <<" n'a pas pu être contrôlé. On suppose qu'il est bon.");
		break;
	case NOT_VALID:
		LOGGER_ERROR("Le fichier de configuration de pyramide " << configFileName << " n'est pas valide" );
		LOGGER_ERROR("Impossible d'exploiter la pyramide décrite par "<< configFileName );
		xmlFreeDoc(doc);
		return NULL;
	case VALID:
	default:
		LOGGER_DEBUG("Le fichier " << configFileName << " semble valide (au sens du schema)" );
		break;
	}

    // Récupération de la racine
	xmlNodePtr racine;
    racine = xmlDocGetRootElement(doc);

    std::vector<Level*> levelList;

    xmlNodePtr srsNode=racine->children;
    while (strcmp ((char*)srsNode->name, "srs")){
    	srsNode=srsNode->next;
    }
	std::string srs = (char *) srsNode->children->content;

    for ( xmlNodePtr levelNode = srsNode->next; levelNode; levelNode=levelNode->next){
    	if (!strcmp ((char*)levelNode->name, "level")){
    		std::string id;
    		std::string dir_name;
    		std::string format;
    		int channels=-1;
    		int image_width=-1, image_height=-1;
    		double resolution_x=-1.0, resolution_y=-1.0;
    		int tile_width=-1, tile_height=-1;
    		double x_origin=-1.0, y_origin=-1.0;
    		int path_depth=-1;
    		for ( xmlNodePtr node2=levelNode->children; node2; node2=node2->next ){
    	    	if (!strcmp ((char*)node2->name, "id")){
    	    		id = (char *) node2->children->content;
    	    	} else if (!strcmp ((char*)node2->name, "dir_name")){
    	    		dir_name = (char *) node2->children->content;
    	    	} else if (!strcmp ((char*)node2->name, "format")){
    	    		format = (char *) node2->children->content;
    	    	} else if (!strcmp ((char*)node2->name, "channels")){
    	    		channels = atoi((char *) node2->children->content);
    	    	} else if (!strcmp ((char*)node2->name, "image_width")){
    	    		image_width = atoi((char *) node2->children->content);
    	    	} else if (!strcmp ((char*)node2->name, "image_height")){
    	    		image_height = atoi((char *) node2->children->content);
    	    	} else if (!strcmp ((char*)node2->name, "resolution_x")){
    	    		resolution_x = atof((char *) node2->children->content);
    	    	} else if (!strcmp ((char*)node2->name, "resolution_y")){
    	    		resolution_y = atof((char *) node2->children->content);
    	    	} else if (!strcmp ((char*)node2->name, "tile_width")){
    	    		tile_width = atof((char *) node2->children->content);
    	    	} else if (!strcmp ((char*)node2->name, "tile_height")){
    	    		tile_height = atof((char *) node2->children->content);
    	    	} else if (!strcmp ((char*)node2->name, "x_origin")){
    	    		x_origin = atof((char *) node2->children->content);
    	    	} else if (!strcmp ((char*)node2->name, "y_origin")){
    	    		y_origin = atof((char *) node2->children->content);
    	    	} else if (!strcmp ((char*)node2->name, "path_depth")){
    	    		path_depth = atoi((char *) node2->children->content);
    	    	}
    	    	// TODO: ici on traitera le cas des metadonnees décrites dans le xml

    		}
    		// TODO: controle de cohérences entre les parametres.
    		// o verifier de la taille des tuiles est compatible avec la taille des images.
    		// o vérifier que le nombre de canal est compatible avec le format...
    		// FIXME: la relation entre le format et le nombre de cannaux doit être clarifiée.
    		if (format.find("_INT8",0) != std::string::npos){
    			// TODO: On a 8bits par cannaux, mais pour le moment on en fait rien
    			//       Parce qu'on gere que ca.
    		}else{
    			LOGGER_ERROR(configFileName << ": level: " << id << "Seul les formats d'images *_INT8 sont actuellement gérés");
    			LOGGER_ERROR(configFileName << ": le level " << id << " n'est pas pris en compte");
    			continue;
    		}

	    	Level *TL = new TiledLevel<RawDecoder>(srs.c_str(),
	    			                               tile_width, tile_height,
	    			                               channels,
	    			                               resolution_x, resolution_y,
	    			                               x_origin, y_origin,
	    			                               dir_name,
	    			                               image_width/tile_width, image_height/tile_height,
	    			                               path_depth);
	    	levelList.push_back(TL);
    	}

    }// boucle sur les levels

    // On a terminé l'analyse du fichier de config,
    // on libère la mémoire allouée pour le document
    xmlFreeDoc(doc);

    // on crée enfin la Pyramid
    if (levelList.size()==0){
    	LOGGER_ERROR("La config "<< configFileName << " ne contient aucun level valide, la pyramide n'est pas prise en compte");
    	return NULL;
    }
    Level** LL = new Level*[levelList.size()];
    LOGGER_DEBUG("Nombre de levels valides dans cette pyramide: " << levelList.size());
    for(int i = 0; i < levelList.size(); i++){
    	LL[i] = levelList[i];
    }
    Pyramid* Pyr = new Pyramid(LL, levelList.size());

    return Pyr;

}

