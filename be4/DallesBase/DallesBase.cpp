/**
 * \file DallesBase.cpp
 * \brief Creation d une dalle georeference a partir de n dalles source
 * \author IGN
*
* Ce programme est destine a etre utilise dans la chaine de generation de cache be4.
* Il est appele pour calculer le niveau minimum d'une pyramide, pour chaque nouvelle dalle.
*
* Les dalles source ne sont pas necessairement entierement recouvrantes.
*
* Parametres d'entree :
* 1. Un fichier texte contenant les dalles source et la dalle finale avec leur georeferencement (resolution, emprise)
* 2. Un mode d'interpolation
* 3. Une couleur de NoData
* 4. Un type de dalle (Data/Metadata)
* 5. Le nombre de canaux des images
* 6. Nombre d'octets par canal
* 7. La colorimetrie
*
* En sortie, un fichier TIFF au format dit de travail brut non compressé entrelace
* Ou, erreurs (voir dans le main)
*
* Contrainte:
* Toutes les dalles sont dans le meme SRS (pas de reprojection)
* FIXME : meme pixels (nombre de canaux, poids, couleurs) en entree et en sortie
*/

#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include "tiffio.h"
#include "config.h"
#include "Logger.h"
#include "LibtiffImage.h"
#include "ResampledImage.h"
#include "ExtendedCompoundImage.h"


/* Usage de la ligne de commande */

void usage() {
	LOGGER_INFO(" Parametres : dalles_base -f (fichier de dalles) -i (interpolation) -n (couleur NoData) -t (type) -s (nb de sample par pixel) -b (nb de bit par sample) -p(photometric) ");
	LOGGER_INFO(" Exemple : dalles_base -f myfile.txt -i [lanczos/ppv/bicubique] -n CC00CC -t [img/mtd] -s [1/3] -b [8/32] -p[min_is_black/rgb/mask] ");
}

/* Lecture des parametres de la ligne de commande */

int parseCommandLine(int argc, char** argv, char* liste_dalles_filename, Kernel::KernelType interpolation, char* nodata, int& type, uint16_t& sampleperpixel, uint16_t& bitspersample, uint16_t& photometric) {

	if (argc != 15) {
		LOGGER_ERROR(" Nombre de parametres incorrect : !!");
		usage();
		return -1;
	}

	for(int i = 1; i < argc; i++) {
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
				else {LOGGER_ERROR("Erreur sur l'option -b"); return -1;}
				break;
			case 'p': // photometric
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -p"); return -1;}
				if(strncmp(argv[i], "min_is_black",12) == 0) photometric = PHOTOMETRIC_MINISBLACK;
				else if(strncmp(argv[i], "rgb",3) == 0) photometric = PHOTOMETRIC_RGB;
				else if(strncmp(argv[i], "mask",4) == 0) photometric = PHOTOMETRIC_MASK;
				else {LOGGER_ERROR("Erreur sur l'option -p"); return -1;}
				break;
			default: usage(); return -1;
			}
		}
	}

	return 0;
}

/* Lecture d une ligne du fichier de dalles */

int readFileLine(std::ifstream& file, char* filename, BoundingBox<double>* bbox, int* width, int* height)
{
	std::string str;
	std::getline(file,str);
	double resx, resy;
	int nb;

	if ( (nb=sscanf(str.c_str(), "%s %lf %lf %lf %lf %lf %lf",filename, &bbox->xmin, &bbox->ymax, &bbox->xmax, &bbox->ymin, &resx, &resy)) ==7) {
		// Arrondi a la valeur entiere la plus proche
		*width = (int) ((bbox->xmax - bbox->xmin)/resx + 0.5);	
		*height = (int) ((bbox->ymax - bbox->ymin)/resy + 0.5);
	}

	return nb;
}

/* Chargement des images depuis le fichier texte donné en parametre */

int loadDalles(char* liste_dalles_filename, LibtiffImage** ppImageOut, std::vector<Image*>* pImageIn, int sampleperpixel, uint16_t bitspersample, uint16_t photometric)
{
	char filename[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
	BoundingBox<double> bbox(0.,0.,0.,0.);
	int width, height;
	libtiffImageFactory factory;

	// Ouverture du fichier texte listant les dalles
	std::ifstream file;
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
	*ppImageOut=factory.createLibtiffImage(filename, bbox, width, height, sampleperpixel, bitspersample, photometric,COMPRESSION_NONE);

	if (*ppImageOut==NULL){
		LOGGER_ERROR("Impossible de creer " << filename);
		return -1;
	}

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


/* Controle des dalles
* TODO : ajouter des controles
*/

int checkDalles(LibtiffImage* pImageOut, std::vector<Image*>& ImageIn)
{
	for (unsigned int i=0;i<ImageIn.size();i++) {
		if (ImageIn.at(i)->getresx()*ImageIn.at(i)->getresy()==0.) {	
			LOGGER_ERROR("Resolution de la dalle d entre " << i+1 << " sur " << ImageIn.size() << " egale a 0");
                	return -1;
		}
	}
	if (pImageOut->getresx()*pImageOut->getresy()==0.){
		LOGGER_ERROR("Resolution de la dalle de sortie egale a 0 " << pImageOut->getfilename());
		return -1;
	}

	return 0;
}

/* Calcul de la phase en X d'une image */

double getPhasex(Image* pImage) {
        double intpart;
        return ( modf( pImage->getxmin()/pImage->getresx(), &intpart)) ;
}

/* Calcul de la phase en Y d'une image */

double getPhasey(Image* pImage) {
        double intpart;
        return ( modf( pImage->getymax()/pImage->getresy(), &intpart)) ;
}

/* Teste si 2 images sont superposabbles */
bool areOverlayed(Image* pImage1, Image* pImage2)
{
	if (pImage1->getresx()!=pImage2->getresx()) return false;
        if (pImage1->getresy()!=pImage2->getresy()) return false;
	if (getPhasex(pImage1)!=getPhasex(pImage2)) return false;
        if (getPhasey(pImage1)!=getPhasey(pImage2)) return false;
	return true;
} 

/* Fonctions d'ordre */
bool InfResx(Image* pImage1, Image* pImage2) {return (pImage1->getresx()<pImage2->getresx());}
bool InfResy(Image* pImage1, Image* pImage2) {return (pImage1->getresy()<pImage2->getresy());}
bool InfPhasex(Image* pImage1, Image* pImage2) {return (getPhasex(pImage1)<getPhasex(pImage2));}
bool InfPhasey(Image* pImage1, Image* pImage2) {return (getPhasey(pImage1)<getPhasey(pImage2));}

/*
* @brief Tri des dalles source en paquets de dalles superposables (memes phases et resolutions en x et y)
* @param ImageIn : vecteur contenant les images non triees
* @param pTabImageIn : tableau de vecteurs conteant chacun des images superposables
* @return 0 en cas de succes, -1 sinon
*/

int sortDalles(std::vector<Image*> ImageIn, std::vector<std::vector<Image*> >* pTabImageIn)
{
	std::vector<Image*> vTmp;
	
	// Initilisation du tableau de sortie
	pTabImageIn->push_back(ImageIn);

	// Creation de vecteurs contenant des images avec une resolution en x homogene
	for (std::vector<std::vector<Image*> >::iterator it=pTabImageIn->begin();it<pTabImageIn->end();it++)
        {
                std::sort(it->begin(),it->end(),InfResx); 
                for (std::vector<Image*>::iterator it2 = it->begin();it2+1<it->end();it2++)
                        if ((*it2)->getresy()!=(*(it2+1))->getresy() && it2+2!=it->end())
                        {
				vTmp.assign(it2+1,it->end());
                                it->assign(it->begin(),it2+1);
                                pTabImageIn->push_back(vTmp);

				return 0;
                                it++;
                        }
        }

//TODO : A refaire proprement
/*
	// Creation de vecteurs contenant des images avec une resolution en x et en y homogenes
        for (std::vector<std::vector<Image*> >::iterator it=pTabImageIn->begin();it<pTabImageIn->end();it++)
        {
                std::sort(it->begin(),it->end(),InfResy); 
                for (std::vector<Image*>::iterator it2 = it->begin();it2+1<it->end();it2++)
                	if ((*it2)->getresy()!=(*(it2+1))->getresy() && it2+2!=it->end())
                	{
                        	it->assign(it->begin(),it2);
                        	vTmp.assign(it2+1,it->end());
                        	pTabImageIn->push_back(vTmp);
                        	it++;
                	}
        }

	// Creation de vecteurs contenant des images avec une resolution en x et en y, et une pihase en x homogenes
        for (std::vector<std::vector<Image*> >::iterator it=pTabImageIn->begin();it<pTabImageIn->end();it++)
        {
                std::sort(it->begin(),it->end(),InfPhasex); 
                for (std::vector<Image*>::iterator it2 = it->begin();it2+1<it->end();it2++)
                	if (getPhasex(*it2)!=getPhasex(*(it2+1)) && it2+2!=it->end())
                	{
                        	it->assign(it->begin(),it2);
	                        vTmp.assign(it2+1,it->end());
        	                pTabImageIn->push_back(vTmp);
                	        it++;
                	}
        }

	// Creation de vecteurs contenant des images superposables
        for (std::vector<std::vector<Image*> >::iterator it=pTabImageIn->begin();it<pTabImageIn->end();it++)
        {
                std::sort(it->begin(),it->end(),InfPhasey); 
                for (std::vector<Image*>::iterator it2 = it->begin();it2+1<it->end();it2++)
                	if (getPhasey(*it2)!=getPhasey(*(it2+1)) && it2+2!=it->end())
                	{
                        	it->assign(it->begin(),it2);
	                        vTmp.assign(it2+1,it->end());
        	                pTabImageIn->push_back(vTmp);
                	        it++;
                	}
        }
*/
	return 0;
}

/* 
* @brief Assemblage de dalles superposables
* @param TabImageIn : vecteur de dalles a assembler
* @return Image composee de type ExtendedCompoundImage
*/

ExtendedCompoundImage* compoundDalles(std::vector< Image*> & TabImageIn)
{
	if (TabImageIn.empty()) {
		LOGGER_ERROR("Assemblage d'un tableau de dalles de taille nulle");
		return NULL;
	}

	// Rectangle englobant des dalles d entree
	double xmin=1E12, ymin=1E12, xmax=-1E12, ymax=-1E12 ;
	for (unsigned int j=0;j<TabImageIn.size();j++) {
		if (TabImageIn.at(j)->getxmin()<xmin)  xmin=TabImageIn.at(j)->getxmin();
		if (TabImageIn.at(j)->getymin()<ymin)  ymin=TabImageIn.at(j)->getymin();
		if (TabImageIn.at(j)->getxmax()>xmax)  xmax=TabImageIn.at(j)->getxmax();
		if (TabImageIn.at(j)->getymax()>ymax)  ymax=TabImageIn.at(j)->getymax();
	}

	extendedCompoundImageFactory ECImgfactory ;
	int w=(int)((xmax-xmin)/(*TabImageIn.begin())->getresx()+0.5), h=(int)((ymax-ymin)/(*TabImageIn.begin())->getresy()+0.5);
	ExtendedCompoundImage* pECI = ECImgfactory.createExtendedCompoundImage(w,h,(*TabImageIn.begin())->channels, BoundingBox<double>(xmin,ymin,xmax,ymax), TabImageIn);

	return pECI ;
}

/*
* @brief Reechantillonnage d'une dalle de type ExtendedCompoundImage
* @brief Objectif : la rendre superposable a l'image finale
* @return Image reechantillonnee legerement plus petite
*/

ResampledImage* resampleDalles(LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, Kernel::KernelType& interpolation )
{
	double xmin=pECI->getxmin(), ymin=pECI->getymin(), xmax=pECI->getxmax(), ymax=pECI->getymax() ;
	double resx = pECI->getresx(), resy = pECI->getresy() ;
	double ratiox = pImageOut->getresx() / resx ;
	double ratioy = pImageOut->getresy() / resy ;
	double decalx = ratiox * interpolation, decaly = ratioy * interpolation ;

	int newwidth = int( ((xmax - xmin)/resx - 2*decalx) / ratiox + 0.5 );
	int newheight = int( ((ymax - ymin)/resy - 2*decaly) / ratioy + 0.5 );
	BoundingBox<double> newbbox(xmin +decalx*resx, ymin +decaly*resy, xmax -decalx*resx, ymax -decaly*resy);

	// creation de la RI en passant la newbbox sur le dernier parametre
	ResampledImage* pRImage = new ResampledImage( (Image*) pECI, newwidth, newheight, decalx, decaly, ratiox, ratioy,
			interpolation, newbbox );
	return pRImage ;
}

/*
* @brief Fusion des dalles
* @param pImageOut : dalle de sortie
* @param TabImageIn : tableau de vecteur d images superposables
* @param ppECImage : image composite creee
* @param interpolation : type d'interpolation utilise
* @return 0 en cas de succes, -1 sinon
*/

int mergeTabDalles(LibtiffImage* pImageOut, std::vector<std::vector<Image*> >& TabImageIn, ExtendedCompoundImage** ppECImage, Kernel::KernelType& interpolation)
{
	extendedCompoundImageFactory ECImgfactory ;
	std::vector<Image*> pOverlayedImage;

	for (unsigned int i=0; i<TabImageIn.size(); i++) {
		// Mise en superposition du paquet d'images en 2 etapes

	        // Etape 1 : Creation d'une image composite
        	ExtendedCompoundImage * pECI = compoundDalles(TabImageIn.at(i));
	        if (pECI==NULL) {
        	        LOGGER_ERROR("Impossible d'assembler les images");
                	return -1;
	        }
		if (areOverlayed(pImageOut,pECI))
			pOverlayedImage.push_back(pECI);
		else {
        		// Etape 2 : Reechantillonnage de l'image composite si necessaire
	        	ResampledImage* pRImage = resampleDalles(pImageOut, pECI, interpolation);
        		if (pRImage==NULL) {
                		LOGGER_ERROR("Impossible de reechantillonner les images");
	                	return -1;
			}
			pOverlayedImage.push_back( pRImage );
        	}
	}

	// Assemblage des paquets et decoupage aux dimensions de la dalle de sortie
	if ( (*ppECImage = ECImgfactory.createExtendedCompoundImage(pImageOut->width, pImageOut->height,
			pImageOut->channels, pImageOut->getbbox(), pOverlayedImage))==NULL) {
		LOGGER_ERROR("Erreur lors de la fabrication de l image finale");
		return -1;
	}

//	delete pTabResampledImage;

	return 0;
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


/*
* @brief Enregistrement d'une image TIFF
* @param Image : Image a enregistrer
* @param pName : nom du fichier TIFF
* @param sampleperpixel : nombre de canaux de l'image TIFF
* @parama bitspersample : nombre de bits par canal de l'image TIFF
* @param photometric : valeur du tag TIFFTAG_PHOTOMETRIC de l'image TIFF
* @param nodata : valeur du pixel representant la valeur NODATA (6 caractère hexadécimaux)
* TODO : gerer tous les types de couleur pour la valeur NODATA
* @return : 0 en cas de succes, -1 sinon
*/

int saveImage(Image *pImage, char* pName, int sampleperpixel, uint16_t bitspersample, uint16_t photometric, char* nodata) {
        // Ouverture du fichier
        TIFF* output=TIFFOpen(pName,"w");
        if (output==NULL) {
                LOGGER_ERROR("Impossible d'ouvrir le fichier " << pName << " en ecriture");
                return -1;
        }

        // Ecriture de l'en-tete
        TIFFSetField(output, TIFFTAG_IMAGEWIDTH, pImage->width);
        TIFFSetField(output, TIFFTAG_IMAGELENGTH, pImage->height);
        TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);
        TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, bitspersample);
        TIFFSetField(output, TIFFTAG_PHOTOMETRIC, photometric);
        TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, 1);
        TIFFSetField(output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

        // Initilisation des buffers
        unsigned char * buf_line = (unsigned char *)_TIFFmalloc(pImage->width*pImage->channels*bitspersample/8 );
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

        // Ecriture de l'image
        for( int line = 0; line < pImage->height; line++) {
                _TIFFmemcpy(buf_line, buf_nodata, pImage->width*pImage->channels*bitspersample/8);
                pImage->getline(buf_line,line);
                TIFFWriteScanline(output, buf_line, line, 0); // en contigu (entrelace) on prend chanel=0 (!)
        }

        // Liberation
        _TIFFfree(buf_line);
        _TIFFfree(buf_nodata);
        TIFFClose(output);
        return 0;
}

/**
* Fonction principale
*/

int main(int argc, char **argv) {
	char liste_dalles_filename[256], nodata[6];
	uint16_t sampleperpixel, bitspersample, photometric;
	int type=-1;
	Kernel::KernelType interpolation = DEFAULT_INTERPOLATION;

	LibtiffImage * pImageOut ;
	std::vector<Image*> ImageIn;
	std::vector<std::vector<Image*> > TabImageIn;
	ExtendedCompoundImage* pECImage;

	/* Initialisation des Loggers */
        Accumulator* acc = new RollingFileAccumulator("/var/tmp/be4"/*,86400,1024*/);
        Logger::setAccumulator(DEBUG, acc);
        Logger::setAccumulator(INFO , acc);
        Logger::setAccumulator(WARN , acc);
        Logger::setAccumulator(ERROR, acc);
        Logger::setAccumulator(FATAL, acc);

	std::ostream &log = LOGGER(DEBUG);
        log.precision(18);
	log.setf(std::ios::fixed,std::ios::floatfield);

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
	if (sortDalles(ImageIn, &TabImageIn)<0){
		LOGGER_ERROR("Echec tri des dalles"); return -1;
	}

	// Fusion des paquets de dalles

	if (mergeTabDalles(pImageOut, TabImageIn, &pECImage, interpolation) < 0){
		LOGGER_ERROR("Echec fusion des paquets de dalles"); return -1;
	}

	// Enregistrement de la dalle fusionnee
	if (saveImage(pECImage,pImageOut->getfilename(),pImageOut->channels,bitspersample,photometric,nodata)<0){
		LOGGER_ERROR("Echec enregistrement dalle finale"); return -1;
	}

	// TODO Nettoyage
/*	delete pImageOut ;
	delete pECImage ;
*/
	LOGGER_INFO( " dalles_base ; fin du programme " );

	return 0;
}
