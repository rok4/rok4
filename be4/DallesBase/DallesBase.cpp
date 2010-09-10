//============================================================================
// Name        : DallesBase.cpp
// Author      : 
// Version     :
// Copyright   : 
// Description : Creation d une dalle georeference a partir de n dalles source
//============================================================================

/*
Ce programme est destine a etre utilise dans la chaine de generation de cache be4
Il est appele pour calculer le niveau minimum d une pyramide, pour chaque nouvelle dalle

Les dalles source ne sont pas necessairement entierement recouvrantes
Parametres d'entree :
1. Un fichier texte contenant les dalles source et la dalle finale avec leur georeferencement (resolution, emprise)
2. Un mode d interpolation

En sortie, un fichier TIFF au format dit de travail brut non compressé entrelace
Ou, erreurs :


Contrainte:
Toutes les dalles sont dans le meme SRS (pas de reprojection)
Autres
*/

using namespace std;
#include <iostream>

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <fstream>
#include "tiffio.h"

#include "Logger.h"
#include "config.h"
#include "LibtiffImage.h"
#include "ResampledImage.h"
#include "ExtendedCompoundImage.h"

/* Usage de la ligne de commande */

void usage() {
	LOGGER_INFO(" expl : MergeDalles -f textfile -i [lanczos/ppv/bicubique] -n couleur_hexa -t [img/mtd] -s [1/3] -b [8/32] -p[min_is_black/rgb/mask]");
}

/* Lecture des parametres de la ligne de commande */

int parseCommandLine(int argc, char** argv, char* liste_dalles_filename, Kernel::KernelType& interpolation, char* nodata, int& type, uint16_t& sampleperpixel, uint16_t& bitspersample, uint16_t& photometric) {
	if (argc != 15) {
		LOGGER_ERROR(" Nombre de parametres incorrect : !!");
		usage();
		return -1;
	}

	for(int i = 1; i < argc; i++) {
		LOGGER_DEBUG(argv[i] << " " << argv[i+1]);
		if(argv[i][0] == '-') {
			switch(argv[i][1]) {
			case 'f': // fichier de dalles
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -f"); return -1;}
				strcpy(liste_dalles_filename,argv[i]);
				break;
			case 'i': // interpolation
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -i"); return -1;}
				if(strncmp(argv[i], "lanczos",7) == 0) interpolation = Kernel::LANCZOS_3;
				else if(strncmp(argv[i], "ppv",3) == 0) interpolation = Kernel::NEAREST_NEIGHBOUR;
				else if(strncmp(argv[i], "bicubique",9) == 0) interpolation = Kernel::CUBIC;
				else {LOGGER_ERROR("Erreur sur l'option -i "); return -1;}
				break;
			case 'n': // nodata
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -n"); return -1;}
				strcpy(nodata,argv[i]);
				if (strlen(nodata)!=6) {LOGGER_ERROR("Couleur nodata invalide "); return -1;}
				break;
			case 't': // type
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -t"); return -1;}
				if(strncmp(argv[i], "img",3) == 0) type = 1 ;
				else if(strncmp(argv[i], "mtd",3) == 0) type = 0 ;
				else {LOGGER_ERROR("Erreur sur l'option -t"); return -1;}
				break;
			case 's': // sampleperpixel
                                if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -s"); return -1;}
                                if(strncmp(argv[i], "1",1) == 0) sampleperpixel = 1 ;
                                else if(strncmp(argv[i], "3",1) == 0) sampleperpixel = 3 ;
                                else {LOGGER_ERROR("Erreur sur l'option -s"); return -1;}
                                break;
			case 'b': // bitspersample
                                if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -s"); return -1;}
                                if(strncmp(argv[i], "8",1) == 0) bitspersample = 8 ;
                                else if(strncmp(argv[i], "32",2) == 0) bitspersample = 32 ;
                                else {LOGGER_ERROR("Erreur sur l'option -s"); return -1;}
                                break;
			case 'p': // photometric
                                if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -s"); return -1;}
                                if(strncmp(argv[i], "min_is_black",12) == 0) photometric = PHOTOMETRIC_MINISBLACK;
                                else if(strncmp(argv[i], "rgb",3) == 0) photometric = PHOTOMETRIC_RGB;
				else if(strncmp(argv[i], "mask",4) == 0) photometric = PHOTOMETRIC_MASK;
                                else {LOGGER_ERROR("Erreur sur l'option -s"); return -1;}
                                break;
			default: usage(); return -1;
			}
		}
	}
	return 0;
}

/* Lecture d une ligne du fichier de dalles */

int readFileLine(ifstream& file, char* filename, BoundingBox<double>* bbox, int* width, int* height)
{
	string str;
        std::getline(file,str);
        int n;
	double resx, resy;
        n=sscanf (str.c_str(), "%s %lf %lf %lf %lf %lf %lf",filename, &bbox->xmin, &bbox->xmax, &bbox->ymin, &bbox->ymax, &resx, &resy);
	if (resx*resy==0.)
	{
		LOGGER_ERROR("Ligne : " << str.c_str() << " resolution nulle");
		return -1;
	}
	*width = (int) (bbox->xmax - bbox->xmin)/resx;	
	*height = (int) (bbox->ymax - bbox->ymin)/resy;

	return n;
}

/* Chargement des images depuis le fichier texte donné en parametre */

int loadDalles(char* liste_dalles_filename, LibtiffImage** ppImageOut, vector<LibtiffImage*>* pImageIn, int sampleperpixel, uint16_t bitspersample, uint16_t photometric)
{
	char filename[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
	BoundingBox<double> bbox(0.,0.,0.,0.);
	int width, height;
	libtiffImageFactory factory;

	// Ouverture du fichier texte listant les dalles
	ifstream file;
	file.open(liste_dalles_filename);
	if (!file) {
                LOGGER_ERROR("Impossible d'ouvrir le fichier " << liste_dalles_filename);
                return -1;
        }

	// Lecture et creation de la dalle de sortie
	if (readFileLine(file,filename,&bbox,&width,&height)<0){
		LOGGER_ERROR("Erreur fichier " << liste_dalles_filename << " ligne 0");
		return -1;
	}
	else {
		*ppImageOut=factory.createLibtiffImage(filename, bbox, width, height, sampleperpixel, bitspersample, photometric,COMPRESSION_NONE);
		if (*ppImageOut==NULL){
			LOGGER_ERROR("Impossible de creer " << filename);
			return -1;
		}
	}

	// Lecture et creation des dalles d'entree	
	int i=1,n;
	while ((n=readFileLine(file,filename,&bbox,&width,&height))==7)
	{
		LibtiffImage* pImage=factory.createLibtiffImage(filename, bbox);
		if (pImage==NULL){
                        LOGGER_ERROR("Impossible de creer une image a partir de " << filename);
                        return -1;
                }
		pImageIn->push_back(pImage);
		i++;
	}
	if (n>0&&n!=7)
	{
                LOGGER_ERROR("Erreur fichier " << liste_dalles_filename << " ligne " << i);
                return -1;
	}

	// Fermeture du fichier
	file.close();

	return (pImageIn->size()+1);	
}

/* Controle des dalles d 'entree
Elles doivent :
1. Etre au format TIFF
2. Ne pas etre compressees
3. Avoir la meme resolution en x et y que la dalle de sortie
4. Avoir les memes valeurs de champ TIFFTAG_PHOTOMETRIC, TIFFTAG_BITSPERSAMPLE et TIFFTAG_SAMPLESPERPIXEL
*/

int checkDalles(LibtiffImage* pImageOut, vector<LibtiffImage*>& ImageIn)
{
	if (pImageOut->getresx()*pImageOut->getresy()==0.){
		LOGGER_ERROR("Resolution de la dalle de sortie egale a 0 " << pImageOut->getfilename());
                return -1;
	}
	for (unsigned int i=0;i<ImageIn.size();i++)
	{
LOGGER_DEBUG(ImageIn.at(i)->width);
		TIFF* tiff = TIFFOpen( ImageIn.at(i)->getfilename(), "r");
		if (!tiff) {
			LOGGER_ERROR("Impossible d 'ouvrir le fichier " << ImageIn.at(i)->getfilename());
			return -1;
		}
		TIFFClose(tiff);
                if (ImageIn.at(i)->getcompression() != COMPRESSION_NONE){
                        LOGGER_ERROR(" Dalle " << ImageIn.at(i)->getfilename() << " compressee Code = " << ImageIn.at(i)->getcompression());
			return -1;
		}
		if (ImageIn.at(0)->getphotometric()!=ImageIn.at(i)->getphotometric()){
			LOGGER_ERROR("Dalle " << ImageIn.at(i)->getfilename() << " photometrie incoherente avec la dalle " << ImageIn.at(0)->getfilename());
			return -1;
		}
		if (ImageIn.at(0)->getbitspersample()!=ImageIn.at(i)->getbitspersample()){
                        LOGGER_ERROR("Dalle " << ImageIn.at(i)->getfilename() << " bitspersample incoherent avec la dalle " << ImageIn.at(0)->getfilename());
                	return -1;
                }
                if (ImageIn.at(0)->channels!=ImageIn.at(i)->channels){
                        LOGGER_ERROR("Dalle "<< ImageIn.at(i)->getfilename() << " sampleperpixel incoherent avec la dalle " << ImageIn.at(0)->getfilename());
                        return -1;
                }
		if (ImageIn.at(i)->getresx()!=pImageOut->getresx() || ImageIn.at(i)->getresy()!=pImageOut->getresy()){
			LOGGER_ERROR("La resolution de la dalle " << ImageIn.at(0)->getfilename() << " diffrente de la resolution de sortie");
                        return -1;
		}
	}
LOGGER_DEBUG("ttttt");
	return 0;
}

/*
	// TODO gerer le type mtd
*/

/* Fusion de dalles */

int mergeDalles(LibtiffImage* pImageOut, vector<LibtiffImage*>& ImageIn,  ExtendedCompoundImage** ppImage) 
{
	// Fabrication de la ExtendedCompoundImage
	extendedCompoundImageFactory ECImgfactory ;
	if ( (*ppImage = ECImgfactory.createExtendedCompoundImage(pImageOut->width,pImageOut->height,pImageOut->channels,pImageOut->getbbox(),ImageIn))==NULL) {
		LOGGER_ERROR("Erreur lors de la fabrication de l image fusionnee");
		return -1;
	}

	return 0;
}

// Hexadecimal -> int

int h2i(char s)
{
	if('0' <= s && s <= '9')
		return (s - '0');
	if('a' <= s && s <= 'f')
		return (s - 'a' + 10);
	if('A' <= s && s <= 'F')
		return (10 + s - 'A');
	else
		return -1; /* invalid input! */
}

/* Enregistrement d'une image TIFF */

int saveImage(Image *pImage, char* pName, int sampleperpixel, uint16_t& bitspersample, uint16_t& photometric, char* nodata) {
        TIFF* output=TIFFOpen(pName,"w");
        if (!output) {
                LOGGER_ERROR( " Impossible d'ouvrir le fichier en ecriture !" );
                return -1;
        }
        TIFFSetField(output, TIFFTAG_IMAGEWIDTH, pImage->width);
        TIFFSetField(output, TIFFTAG_IMAGELENGTH, pImage->height);
        TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);
        TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, bitspersample);
        TIFFSetField(output, TIFFTAG_PHOTOMETRIC, photometric);
        TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, 1);
        TIFFSetField(output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
        unsigned char * buf_ligne = (unsigned char *)_TIFFmalloc(pImage->width*pImage->channels*bitspersample/8 );
	unsigned char r=h2i(nodata[0])*16 + h2i(nodata[1]), g=h2i(nodata[2])*16 + h2i(nodata[3]),b=h2i(nodata[0])*16 + h2i(nodata[1]);
	for (long k=0;k<(pImage->width)*pImage->channels*bitspersample/8;k+=3){
		buf_ligne[k]=r;buf_ligne[k+1]=g;buf_ligne[k+2]=b;
	}
        for(int ligne = 0; ligne < pImage->height; ligne++) {
                pImage->getline(buf_ligne,ligne);
                TIFFWriteScanline(output, buf_ligne, ligne, 0); // en contigu (entrelace) on prend chanel=0 (!)
        }
        _TIFFfree(buf_ligne);
        TIFFClose(output);
        return 0;
}


/* Fonction principale */

int main(int argc, char **argv) {
        char liste_dalles_filename[256], nodata[6];
        Kernel::KernelType interpolation = DEFAULT_INTERPOLATION;
	LibtiffImage* pImageOut;
	vector<LibtiffImage*> ImageIn;
	ExtendedCompoundImage* pECImage ;
        uint16_t sampleperpixel, bitspersample, photometric;
	int type=-1;

	// Initilisation du logger
	Logger::configure("LOG_CONF_PATH");

	// Lecture des parametres de la ligne de commande
	if (parseCommandLine(argc, argv,liste_dalles_filename,interpolation,nodata,type,sampleperpixel,bitspersample,photometric)<0){
		LOGGER_ERROR("Echec lecture ligne de commande");
		return -1;
	}

	// Chargement des dalles
	if (loadDalles(liste_dalles_filename,&pImageOut,&ImageIn,sampleperpixel,bitspersample,photometric)<0){
		LOGGER_ERROR("Echec chargement des dalles");
		return -1;
	}

	// Controle des dalles
	if (checkDalles(pImageOut,ImageIn)<0){
		LOGGER_ERROR("Echec controle des dalles");
		return -1;
	}

	// Fusion des dalles
	if (mergeDalles(pImageOut,ImageIn,&pECImage)<0){
		LOGGER_ERROR("Echec fusion des dalles");
		return -1;
	}

	// Enregistrement de la dalle fusionnee
	if (saveImage(pECImage,pImageOut->getfilename(),pImageOut->channels,bitspersample,photometric,nodata)<0){
		LOGGER_ERROR("Echec enregistrement dalle finale");
		return -1;
	};

	return 0;
}

