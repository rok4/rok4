//============================================================================
// Name        : DallesBase.cpp
// Author      : AM
// Version     :
// Copyright   : 
// Description : Creation d une dalle georeference a partir de n dalles source
//============================================================================

/*
Ce programme est destine a etre utilise dans la chaine de generation de cache be4.
Il est appele pour calculer le niveau minimum d'une pyramide, pour chaque nouvelle dalle.

Les dalles source ne sont pas necessairement entierement recouvrantes.

Parametres d'entree :
1. Un fichier texte contenant les dalles source et la dalle finale avec leur georeferencement (resolution, emprise)
2. Un mode d'interpolation, une couleur de NoData, un type...

En sortie, un fichier TIFF au format dit de travail brut non compressé entrelace
Ou, erreurs (voir dans le main)

Contrainte:
Toutes les dalles sont dans le meme SRS (pas de reprojection)
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
#include "config.h"

#include "Logger.h"
#include "LibtiffImage.h"
#include "ResampledImage.h"
#include "ExtendedCompoundImage.h"


string version = "version 0.12 ";

/* Usage de la ligne de commande */

void usage() {
	LOGGER_INFO(" Parametres : dalles_base -f (fichier de dalles) -i (interpolation) -n (couleur NoData) -t (type) -s (nb de sample par pixel) -b (nb de bit par sample) -p(photometric) ");
	LOGGER_INFO(" Exemple : dalles_base -f myfile.txt -i [lanczos/ppv/bicubique] -n CC00CC -t [img/mtd] -s [1/3] -b [8/32] -p[min_is_black/rgb/mask] ");
}

double getPhasex(Image * img) {
	double intpart;
	return ( modf( img->getxmin()/img->getresx(), &intpart)) ;
}
double getPhasey(Image * img) {
	double intpart;
	return ( modf( img->getymax()/img->getresy(), &intpart)) ;
}


/* Hexadecimal -> int  */

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

int saveImage(Image *pImage, char* pName, int sampleperpixel, uint16_t bitspersample, uint16_t photometric, char* nodata) {
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

	// calcul du buffer contenant nodata:
	unsigned char * buf_nodata = (unsigned char *)_TIFFmalloc(pImage->width*pImage->channels*bitspersample/8 );
	unsigned char r=h2i(nodata[0])*16 + h2i(nodata[1]), g=h2i(nodata[2])*16 + h2i(nodata[3]), b=h2i(nodata[4])*16 + h2i(nodata[5]);

	if (pImage->channels == 3 ) {
		for (long k=0; k<(pImage->width)*3*bitspersample/8; k+=3){
			buf_nodata[k]=r; buf_nodata[k+1]=g; buf_nodata[k+2]=b;
		}
	} else {
		for (long k=0; k<(pImage->width)*1*bitspersample/8; k+=1)
			buf_nodata[k]=r;
	}

	for( int ligne = 0; ligne < pImage->height; ligne++) {
		// on initialise le buffer avec le buffer_no_data
		_TIFFmemcpy(buf_ligne, buf_nodata, pImage->width*pImage->channels*bitspersample/8);
		// on va lire l'image pImage. NB: une resampledImage renvoie systematiquement toute la width alors
		// qu'une ECImage ne renvoie que la zone comportant des données
		pImage->getline(buf_ligne,ligne) ;
		// on ecrit la zone lue dans l'image à produire
		TIFFWriteScanline(output, buf_ligne, ligne, 0); // en contigu (entrelace) on prend chanel=0 (!)
	}

	_TIFFfree(buf_ligne); _TIFFfree(buf_nodata); TIFFClose(output);
	return 0;
}

/* Lecture des parametres de la ligne de commande */

int parseCommandLine(int argc, char** argv, char* liste_dalles_filename, Kernel::KernelType interpolation, char* nodata, int& type, uint16_t& sampleperpixel, uint16_t& bitspersample, uint16_t& photometric) {

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
				if(strncmp(argv[i], "lanczos",7) == 0) interpolation = Kernel::LANCZOS_3; // =4
				else if(strncmp(argv[i], "ppv",3) == 0) interpolation = Kernel::NEAREST_NEIGHBOUR; // =0
				else if(strncmp(argv[i], "bicubique",9) == 0) interpolation = Kernel::CUBIC; // =2
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
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -b"); return -1;}
				if(strncmp(argv[i], "8",1) == 0) bitspersample = 8 ;
				else if(strncmp(argv[i], "32",2) == 0) bitspersample = 32 ;
				else {LOGGER_ERROR("Erreur sur l'option -s"); return -1;}
				break;
			case 'p': // photometric
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -p"); return -1;}
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
	int ret;
	double resx=0., resy=0.;
	ret=sscanf (str.c_str(), "%s %lf %lf %lf %lf %lf %lf",filename, &bbox->xmin, &bbox->ymax, &bbox->xmax, &bbox->ymin, &resx, &resy);

	if (ret<0) // le fichier est fini
		return ret;

	if (ret>=0 && ret != 7) { // le fichier comporte une ligne fausse
		LOGGER_DEBUG(" Ligne erronnee: " << str.c_str() );
		return ret;
	}

	*width = (int) (bbox->xmax - bbox->xmin)/resx;	
	*height = (int) (bbox->ymax - bbox->ymin)/resy;

	return ret;
}

/* Chargement des images depuis le fichier texte donné en parametre */

int loadDalles(char* liste_dalles_filename, LibtiffImage** ppImageOut, vector<Image*>* pImageIn, int sampleperpixel, uint16_t bitspersample, uint16_t photometric)
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
		LOGGER_ERROR("Erreur lecture du fichier de parametres: " << liste_dalles_filename << " a la ligne 0");
		return -1;
	}

	// NB: le width et height ci-dessous sont les valeurs FINALES de la dalle à creer
	*ppImageOut=factory.createLibtiffImage(filename, bbox, width, height, sampleperpixel, bitspersample, photometric,COMPRESSION_NONE);
	if (*ppImageOut==NULL){
		LOGGER_ERROR("Impossible de creer " << filename);
		return -1;
	}
	//LOGGER_DEBUG( " width-height de l IMAGE FINALE demandee: " << width << "-" << height << ". bbox de l IMAGE FINALE demandee: ");
	//bbox.print() ;

	// Lecture et creation des dalles source
	int iligne=2, nb=0;
	while ((nb=readFileLine(file,filename,&bbox,&width,&height))==7)
	{
		LibtiffImage* pImage=factory.createLibtiffImage(filename, bbox);
		if (pImage==NULL){
			LOGGER_ERROR("Impossible de creer une image a partir de " << filename);
			return -1;
		}
		pImageIn->push_back(pImage);
		iligne++;
	}
	if (nb>=0 && nb!=7)
	{
		LOGGER_ERROR("Erreur lecture du fichier de parametres: " << liste_dalles_filename << " a la ligne " << iligne);
		return -1;
	}

	// Fermeture du fichier
	file.close();

	return (pImageIn->size() - 1);
}


/* Controle des dalles d'entree
Elles doivent :
1. Etre au format TIFF
2. Ne pas etre compressees
 */

int checkDalles(LibtiffImage* pImageOut, vector<Image*>& ImageIn)
{
	if (pImageOut->getresx()*pImageOut->getresy()==0.){
		LOGGER_ERROR("Resolution de la dalle de sortie egale a 0 " << pImageOut->getfilename());
		return -1;
	}

	// les controles sont desormais faits en aval
	//	for (unsigned int i=0;i<ImageIn.size();i++)  {
	//		//LOGGER_DEBUG( ImageIn.at(i)->width);
	//		TIFF* tiff = TIFFOpen( ImageIn.at(i)->getfilename(), "r");
	//		if (!tiff) {
	//			LOGGER_ERROR("Impossible d'ouvrir le fichier " << ImageIn.at(i)->getfilename());  return -1;
	//		}
	//		TIFFClose(tiff);
	//		if (ImageIn.at(i)->getcompression() != COMPRESSION_NONE){
	//			LOGGER_ERROR(" Dalle " << ImageIn.at(i)->getfilename() << " compressee! Code = " << ImageIn.at(i)->getcompression());  return -1;
	//		}
	//		if (ImageIn.at(0)->getphotometric()!=ImageIn.at(i)->getphotometric()){
	//			LOGGER_ERROR("Dalle " << ImageIn.at(i)->getfilename() << " photometrie incoherente avec la dalle " << ImageIn.at(0)->getfilename());  return -1;
	//		}
	//		if (ImageIn.at(0)->getbitspersample()!=ImageIn.at(i)->getbitspersample()){
	//			LOGGER_ERROR("Dalle " << ImageIn.at(i)->getfilename() << " bitspersample incoherent avec la dalle " << ImageIn.at(0)->getfilename());  return -1;
	//		}
	//		if (ImageIn.at(0)->channels!=ImageIn.at(i)->channels){
	//			LOGGER_ERROR("Dalle "<< ImageIn.at(i)->getfilename() << " sampleperpixel incoherent avec la dalle " << ImageIn.at(0)->getfilename());  return -1;
	//		}
	//	}
	return 0;
}

int rechercheIndice(const vector< double> tableau, double indice) {
	for (unsigned int i=0;i<tableau.size();i++)
	{
		if (indice==tableau.at(i))
			return i;
	}
	return (-1);
}

/* Tri les dalles source en paquet de dalles ayant une meme resolution
 */

int sortDalles(vector<Image*> ImageIn, vector< vector<Image*> >* pTabImageIn)
{
	vector < double > tabResx, tabResy ;
	for (unsigned int i=0;i<ImageIn.size();i++)
	{
		int posx=-1, posy=-1 ;
		posx = rechercheIndice( tabResx, ImageIn.at(i)->getresx() );
		if (posx>=0) {
			posy = rechercheIndice( tabResy, ImageIn.at(i)->getresy() );
		}
		if ( posx==posy && posx!=(-1) ) {
			// on insere la dalle dans le tableau
			(pTabImageIn->at(posx)).push_back( ImageIn.at(i) );
		} else {
			tabResx.push_back( ImageIn.at(i)->getresx() );
			tabResy.push_back( ImageIn.at(i)->getresy() );
			vector< Image* > tmp ;
			tmp.push_back( ImageIn.at(i) ) ;
			pTabImageIn->push_back( tmp );
		}
	}

	for (unsigned int i=0; i<pTabImageIn->size(); i++)	{
		LOGGER_DEBUG( " Paquet " << i << " de " << pTabImageIn->at(i).size() << " dalle(s): res x: " << pTabImageIn->at(i).at(0)->getresx() << " - res y: " << pTabImageIn->at(i).at(0)->getresy() );
	}
	return 0;
}


/* Assemble les dalles source intermediaires ayant meme resolution
 */

ExtendedCompoundImage* CompoundDalles( vector< Image*> & TabImageIn )
{
	extendedCompoundImageFactory ECImgfactory ;

	double xmin=1E12, ymin=1E12, xmax=-1E12, ymax=-1E12 ;
	for (unsigned int j=0; j<TabImageIn.size(); j++) {
		if ( TabImageIn.at(j)->getxmin() < xmin )  xmin=TabImageIn.at(j)->getxmin();
		if ( TabImageIn.at(j)->getymin() < ymin )  ymin=TabImageIn.at(j)->getymin();
		if ( TabImageIn.at(j)->getxmax() > xmax )  xmax=TabImageIn.at(j)->getxmax();
		if ( TabImageIn.at(j)->getymax() > ymax )  ymax=TabImageIn.at(j)->getymax();
	}
	// syntaxe: BoundingBox(T xmin, T ymin, T xmax, T ymax)
	BoundingBox<double> bbox(xmin, ymin, xmax, ymax);

	double resx = TabImageIn.at(0)->getresx(), resy = TabImageIn.at(0)->getresy() ;
	int width = (xmax-xmin)/resx, height = (ymax-ymin)/resy ;

	ExtendedCompoundImage * pECI;
	if ( (pECI = ECImgfactory.createExtendedCompoundImage(width, height,
			TabImageIn.at(0)->channels, bbox, TabImageIn))==NULL) {
		return NULL;
	}
	LOGGER_DEBUG( " -------------------------------------- " );
	LOGGER_DEBUG( " ECImage creee: " << pECI->width << " " << pECI->getresx() << " " << pECI->height << " " << pECI->getresy() );
	pECI->getbbox().print();

	return pECI ;
}

/* Resample les dalles source intermediaires ayant meme resolution
 */

ResampledImage* resampleDalles(LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, Kernel::KernelType & interpolation )
{
	double xmin=pECI->getxmin(), ymin=pECI->getymin(), xmax=pECI->getxmax(), ymax=pECI->getymax() ;
	double resx = pECI->getresx(), resy = pECI->getresy() ;

	double ratiox = pImageOut->getresx() / resx ;
	double ratioy = pImageOut->getresy() / resy ;
	LOGGER_DEBUG( " ratios x: " << ratiox << " y: " << ratioy /*<< " ; interpolation: " << interpolation*/ );

	double decalx = ratiox * interpolation, decaly = ratioy * interpolation ;
	int newwidth = int( ((xmax - xmin)/resx - 2*decalx) / ratiox );
	int newheight = int( ((ymax - ymin)/resy - 2*decaly) / ratioy );
	BoundingBox<double> newbbox(xmin +decalx*resx, ymin +decaly*resy, xmax -decalx*resx, ymax -decaly*resy);
	//LOGGER_DEBUG( " RI à creer : " << newwidth << "-" << newheight << "-" << decalx << "-" << decaly );

	// creation de la RI en passant la newbbox sur le dernier parametre
	ResampledImage* pRImage = new ResampledImage( (Image*) pECI, newwidth, newheight, decalx, decaly, ratiox, ratioy,
			interpolation, newbbox );

	LOGGER_DEBUG( " ResampledImage creee: " << pRImage->width << " " << pRImage->getresx()
			<< " " << pRImage->height << " " << pRImage->getresy());
	pRImage->getbbox().print();
	return pRImage ;
}

/* Assemble les paquets de dalles source ayant meme resolution
 */

int mergeTabDalles(LibtiffImage* pImageOut, vector< vector<Image*> > TabImageIn, ExtendedCompoundImage** ppECImage, Kernel::KernelType & interpolation)
{
	extendedCompoundImageFactory ECImgfactory ;
	vector< Image* >  *pTabResampledImage = new vector< Image* >() ;

	for (unsigned int i=0; i<TabImageIn.size(); i++)
	{
		// -----------------------------------------------------------------------------
		// 1/2  pour ce paquet de dalles: creation d'une image globale (ECI)
		ExtendedCompoundImage * pECI = CompoundDalles( TabImageIn.at(i) );
		if (pECI==NULL) {
			LOGGER_ERROR("Echec fusion du paquet de dalles : CompoundDalles: " << i);
			return -1;
		}

		// -----------------------------------------------------------------------------
		// 2/2  creation d'une ResampledImage RI à partir de la ECI :
		ResampledImage* pRImage = resampleDalles(pImageOut, pECI, interpolation );
		if (pRImage==NULL) {
			LOGGER_ERROR("Echec fusion du paquet de dalles : resampleDalles: " << i);
			return -1;
		}
		// on ajoute l'image produite dans le vector qui servira à produire l'ECI finale
		pTabResampledImage->push_back( pRImage );
	}

	LOGGER_DEBUG( " -------------------------------------- " );
	// merge final des différentes ResampleImage --> creation d'une ECI finale
	if ( (*ppECImage = ECImgfactory.createExtendedCompoundImage(pImageOut->width, pImageOut->height,
			pImageOut->channels, pImageOut->getbbox(), *pTabResampledImage))==NULL) {
		LOGGER_ERROR("Erreur lors de la fabrication de l image finale");		return -1;
	}
	LOGGER_DEBUG( " ECImage finale cree: " << (*ppECImage)->width << " " << (*ppECImage)->getresx() << " " <<
			(*ppECImage)->height << " " << (*ppECImage)->getresy() );
	(*ppECImage)->getbbox().print();

	delete pTabResampledImage ;

	return 0;
}

/* Fonction principale ------------------------------------------------------------ */

int main(int argc, char **argv) {

	// Initilisation du logger
	Logger::configure(LOG_CONF_PATH);
	LOGGER_INFO( " dalles_base ; " << version );

	char liste_dalles_filename[256], nodata[6];
	uint16_t sampleperpixel, bitspersample, photometric;
	int type=-1;
	Kernel::KernelType interpolation = DEFAULT_INTERPOLATION; // =4

	LibtiffImage * pImageOut ;
	vector<Image*> ImageIn;
	vector< vector<Image*> > TabImageIn ;
	ExtendedCompoundImage* pECImage ;

	// Lecture des parametres de la ligne de commande
	if (parseCommandLine(argc, argv,liste_dalles_filename,interpolation,nodata,type,sampleperpixel,bitspersample,photometric)<0){
		LOGGER_ERROR("Echec lecture ligne de commande"); return -1;
	}

	// TODO : gérer le type mtd !!
	if (type==0) {
		LOGGER_ERROR("Le type mtd n'est pas pris en compte"); return -1;
	}

	// Chargement des dalles
	if (loadDalles(liste_dalles_filename,&pImageOut,&ImageIn,sampleperpixel,bitspersample,photometric)<0){
		LOGGER_ERROR("Echec chargement des dalles"); return -1;
	}

	// Controle des dalles
	if (checkDalles(pImageOut,ImageIn)<0){
		LOGGER_ERROR("Echec controle des dalles"); return -1;
	}

	// Tri des dalles
	if (sortDalles(ImageIn, & TabImageIn)<0){
		LOGGER_ERROR("Echec tri des dalles"); return -1;
	}

	// Fusion des paquets de dalles
	if (mergeTabDalles(pImageOut, TabImageIn, & pECImage, interpolation) < 0){
		LOGGER_ERROR("Echec fusion des paquets de dalles"); return -1;
	}

	// Enregistrement de la dalle fusionnee
	if (saveImage(pECImage,pImageOut->getfilename(),pImageOut->channels,bitspersample,photometric,nodata)<0){
		LOGGER_ERROR("Echec enregistrement dalle finale"); return -1;
	}

	// Nettoyage
	delete pImageOut ;
	delete pECImage ;

	LOGGER_INFO( " dalles_base ; fin du programme " );

	return 0;
}
