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
* Pas de fichier TIFF tuile ou LUT en entree
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
* FIXME : meme type de pixels (nombre de canaux, poids, couleurs) en entree et en sortie
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
#include "Logger.h"
#include "LibtiffImage.h"
#include "ResampledImage.h"
#include "ExtendedCompoundImage.h"
#include "MirrorImage.h"

/* Usage de la ligne de commande */

void usage() {
	LOGGER_INFO(" Parametres : dalles_base -f (fichier de dalles) -a (sampleformat) -i (interpolation) -n (couleur NoData) -t (type) -s (nb de sample par pixel) -b (nb de bit par sample) -p(photometric) ");
	LOGGER_INFO(" Exemple : dalles_base -f myfile.txt -a [uint/float] -i [lanczos/ppv/linear/bicubique] -n CC00CC -t [img/mtd] -s [1/3] -b [8/32] -p[min_is_black/rgb/mask] ");
}

/* Lecture des parametres de la ligne de commande */

int parseCommandLine(int argc, char** argv, char* liste_dalles_filename, Kernel::KernelType& interpolation, char* nodata, int& type, uint16_t& sampleperpixel, uint16_t& bitspersample, uint16_t& sampleformat,  uint16_t& photometric) {

	if (argc != 17) {
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
				else if(strncmp(argv[i], "linear",6) == 0) interpolation = Kernel::LINEAR; // =2
				else {LOGGER_ERROR("Erreur sur l'option -i "); return -1;}
				break;
			case 'n': // nodata
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -n"); return -1;}
				strcpy(nodata,argv[i]);
				if (strlen(nodata)!=6) {LOGGER_ERROR("Couleur nodata invalide "); return -1;}
				break;
			case 't': // type
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -t"); return -1;}
				if(strncmp(argv[i], "image",5) == 0) type = 1 ;
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
			case 'a': // sampleformat
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -a"); return -1;}
				if(strncmp(argv[i],"uint",4) == 0) sampleformat = 1 ;
				else if(strncmp(argv[i],"float",5) == 0) sampleformat = 3 ;
				else {LOGGER_ERROR("Erreur sur l'option -a"); return -1;}
				break;
			case 'p': // photometric
				if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -p"); return -1;}
				if(strncmp(argv[i], "gray",4) == 0) photometric = PHOTOMETRIC_MINISBLACK;
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

/*
* @brief Enregistrement d'une image TIFF
* @param Image : Image a enregistrer
* @param pName : nom du fichier TIFF
* @param sampleperpixel : nombre de canaux de l'image TIFF
* @parama bitspersample : nombre de bits par canal de l'image TIFF
* @parama sampleformat : format des données binaires (uint ou float)
* @param photometric : valeur du tag TIFFTAG_PHOTOMETRIC de l'image TIFF
* @param nodata : valeur du pixel representant la valeur NODATA (6 caractère hexadécimaux)
* TODO : gerer tous les types de couleur pour la valeur NODATA
* @return : 0 en cas de succes, -1 sinon
*/

int saveImage(Image *pImage, char* pName, int sampleperpixel, uint16_t bitspersample, uint16_t sampleformat, uint16_t photometric) {
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
    TIFFSetField(output, TIFFTAG_SAMPLEFORMAT, sampleformat);
    TIFFSetField(output, TIFFTAG_PHOTOMETRIC, photometric);
    TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

        // Initialisation du buffer
	unsigned char* buf_u=0;
	float* buf_f=0;

	// Ecriture de l'image
	if (sampleformat==1){
		buf_u = (unsigned char*)_TIFFmalloc(pImage->width*pImage->channels*bitspersample/8);
		for( int line = 0; line < pImage->height; line++) {
                        pImage->getline(buf_u,line);
                        TIFFWriteScanline(output, buf_u, line, 0);
		}
	}
	else if(sampleformat==3){
		buf_f = (float*)_TIFFmalloc(pImage->width*pImage->channels*bitspersample/8);
                for( int line = 0; line < pImage->height; line++) {
                        pImage->getline(buf_f,line);
                        TIFFWriteScanline(output, buf_f, line, 0);
		}
	}

    // Liberation
	if (buf_u) _TIFFfree(buf_u);
	if (buf_f) _TIFFfree(buf_f);
    TIFFClose(output);
    return 0;
}

/* Lecture d une ligne du fichier de dalles */

int readFileLine(std::ifstream& file, char* filename, BoundingBox<double>* bbox, int* width, int* height)
{
	std::string str;
	std::getline(file,str);
	double resx, resy;
	int nb;

	if ( (nb=sscanf(str.c_str(),"%s %lf %lf %lf %lf %lf %lf",filename, &bbox->xmin, &bbox->ymax, &bbox->xmax, &bbox->ymin, &resx, &resy)) ==7) {
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

	*ppImageOut=factory.createLibtiffImage(filename, bbox, width, height, sampleperpixel, bitspersample, photometric,COMPRESSION_NONE,16);

	if (*ppImageOut==NULL){
		LOGGER_ERROR("Impossible de creer " << filename);
		return -1;
	}

	// Lecture et creation des dalles source
	int nb=0,i;
	while ((nb=readFileLine(file,filename,&bbox,&width,&height))==7)
	{
		LibtiffImage* pImage=factory.createLibtiffImage(filename, bbox);
		if (pImage==NULL){
			LOGGER_ERROR("Impossible de creer une image a partir de " << filename);
			return -1;
		}
		pImageIn->push_back(pImage);
		i++;
	}
	if (nb>=0 && nb!=7)
	{
		LOGGER_ERROR("Erreur lecture du fichier de parametres: " << liste_dalles_filename << " a la ligne " << i);
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
		if (ImageIn.at(i)->channels!=pImageOut->channels){
			LOGGER_ERROR("Nombre de canaux de la dalle d entree " << i+1 << " sur " << ImageIn.size() << " differente de la dalle de sortie");
                        return -1;
		}
	}
	if (pImageOut->getresx()*pImageOut->getresy()==0.){
		LOGGER_ERROR("Resolution de la dalle de sortie egale a 0 " << pImageOut->getfilename());
		return -1;
	}
	if (pImageOut->getbitspersample()!=8 && pImageOut->getbitspersample()!=32){
		LOGGER_ERROR("Nombre de bits par sample de la dalle de sortie " << pImageOut->getfilename() << " non gere");
                return -1;
	}

	return 0;
}

#define epsilon 0.001

/* Calcul de la phase en X d'une image */

double getPhasex(Image* pImage) {
        double intpart;
        double phi=modf( pImage->getxmin()/pImage->getresx(), &intpart);
	if (fabs(1-phi)<epsilon)
		phi=0.0000001;
	return phi;
}

/* Calcul de la phase en Y d'une image */

double getPhasey(Image* pImage) {
        double intpart;
        double phi=modf( pImage->getymax()/pImage->getresy(), &intpart);
	if (fabs(1-phi)<epsilon)
                phi=0.0000001;
        return phi;
}

/* Teste si 2 images sont superposabbles */
bool areOverlayed(Image* pImage1, Image* pImage2)
{
	if (fabs(pImage1->getresx()-pImage2->getresx())>epsilon) return false;
        if (fabs(pImage1->getresy()-pImage2->getresy())>epsilon) return false;
	if (fabs(getPhasex(pImage1)-getPhasex(pImage2))>epsilon) return false;
        if (fabs(getPhasey(pImage1)-getPhasey(pImage2))>epsilon) return false;
	return true;
} 

/* Fonctions d'ordre */
bool InfResx(Image* pImage1, Image* pImage2) {return (pImage1->getresx()<pImage2->getresx()-epsilon);}
bool InfResy(Image* pImage1, Image* pImage2) {return (pImage1->getresy()<pImage2->getresy()-epsilon);}
bool InfPhasex(Image* pImage1, Image* pImage2) {return (getPhasex(pImage1)<getPhasex(pImage2)-epsilon);}
bool InfPhasey(Image* pImage1, Image* pImage2) {return (getPhasey(pImage1)<getPhasey(pImage2)-epsilon);}

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
                std::stable_sort(it->begin(),it->end(),InfResx); 
                for (std::vector<Image*>::iterator it2 = it->begin();it2+1<it->end();it2++)
                        if ( fabs((*it2)->getresy()-(*(it2+1))->getresy())>epsilon)
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
* @brief Assemblage de dalles superposables
* @param TabImageIn : vecteur de dalles a assembler
* @return Image composee de type ExtendedCompoundImage
*/

ExtendedCompoundImage* compoundDalles(std::vector< Image*> & TabImageIn,char* nodata)
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
	uint8_t r=h2i(nodata[0])*16 + h2i(nodata[1]);
	ExtendedCompoundImage* pECI = ECImgfactory.createExtendedCompoundImage(w,h,(*TabImageIn.begin())->channels, BoundingBox<double>(xmin,ymin,xmax,ymax), TabImageIn,r);

	return pECI ;
}

/* 
* @brief Ajout de miroirs a une ExtendedCompoundImage
* L'image en entree doit etre composee d'un assemblage regulier d'images (de type CompoundImage)
* Objectif : mettre des miroirs la ou il n'y a pas d'images afin d'eviter des effets de bord en cas de reechantillonnage
* @param pECI : l'image à completer
*/

void addMirrors(ExtendedCompoundImage* pECI)
{
	int w=pECI->getimages()->at(0)->width;
	int h=pECI->getimages()->at(0)->height;
	double resx=pECI->getimages()->at(0)->getresx();
	double resy=pECI->getimages()->at(0)->getresy();

	unsigned int i,j;
	double intpart;
	for (i=0;i<pECI->getimages()->size();i++){	
		if (pECI->getimages()->at(i)->getresx()!=resx
		|| pECI->getimages()->at(i)->getresy()!=resy
		|| pECI->getimages()->at(i)->width!=w
		|| pECI->getimages()->at(i)->height!=h
		|| modf(pECI->getimages()->at(i)->getxmin()-pECI->getxmin()/(w*resx),&intpart)!=0
		|| modf(pECI->getimages()->at(i)->getymax()-pECI->getymax()/(h*resy),&intpart)!=0)
		LOGGER_WARN("Image composite irreguliere : impossible d'ajouter des miroirs");
		return;
	}
	
	unsigned int nx=(unsigned int)floor((pECI->getxmax()-pECI->getxmin())/resx + 0.5),
	    	     ny=(unsigned int)floor((pECI->getymax()-pECI->getymin())/resy + 0.5);

	unsigned int k,l;
	Image*pI0,*pI1,*pI2,*pI3;
	double xmin,ymax;
	mirrorImageFactory MIFactory;
	for (i=0;i<nx;i++)
		for (j=0;j<ny;j++){
			for (k=0;k<pECI->getimages()->size();k++)
				if (pECI->getimages()->at(k)->getxmin()==pECI->getxmin()+i*w*resx
				 && pECI->getimages()->at(k)->getymax()==pECI->getymax()-j*h*resy)
					break;
			if (k==pECI->getimages()->size()){
				// Image 0
				pI0=NULL;
				xmin=pECI->getxmin()+(i-1)*w*resx;
				ymax=pECI->getymax()-j*h*resy;
				for (l=0;l<pECI->getimages()->size();l++)
					if (pECI->getimages()->at(l)->getxmin()==xmin
					 || pECI->getimages()->at(l)->getymax()==ymax)
				if (l<pECI->getimages()->size())
					pI0=pECI->getimages()->at(k);
				// Image 1
                                pI1=NULL;
                                xmin=pECI->getxmin()+i*w*resx;
                                ymax=pECI->getymax()-(j-1)*h*resy;
                                for (l=0;l<pECI->getimages()->size();l++)
                                        if (pECI->getimages()->at(l)->getxmin()==xmin
                                         || pECI->getimages()->at(l)->getymax()==ymax)
                                if (l<pECI->getimages()->size())
                                        pI1=pECI->getimages()->at(k);
				// Image 2
                                pI2=NULL;
                                xmin=pECI->getxmin()+(i+1)*w*resx;
                                ymax=pECI->getymax()-j*h*resy;
                                for (l=0;l<pECI->getimages()->size();l++)
                                        if (pECI->getimages()->at(l)->getxmin()==xmin
                                         || pECI->getimages()->at(l)->getymax()==ymax)
                                if (l<pECI->getimages()->size())
                                        pI2=pECI->getimages()->at(k);
                                // Image 3
                                pI3=NULL;
                                xmin=pECI->getxmin()+i*w*resx;
                                ymax=pECI->getymax()-(j+1)*h*resy;
                                for (l=0;l<pECI->getimages()->size();l++)
                                        if (pECI->getimages()->at(l)->getxmin()==xmin
                                         || pECI->getimages()->at(l)->getymax()==ymax)
                                if (l<pECI->getimages()->size())
                                        pI3=pECI->getimages()->at(k);
			}
			MirrorImage* mirror=MIFactory.createMirrorImage(pI0,pI1,pI2,pI3);
			if (mirror!=NULL)
				pECI->getimages()->push_back(mirror);
		} 
}

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

/*
* @brief Reechantillonnage d'une dalle de type ExtendedCompoundImage
* @brief Objectif : la rendre superposable a l'image finale
* @return Image reechantillonnee legerement plus petite
*/

ResampledImage* resampleDalles(LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, Kernel::KernelType& interpolation, ExtendedCompoundMaskImage* mask, ResampledImage*& resampledMask)
{/*
	const Kernel& K = Kernel::getInstance(interpolation);

	double xmin_src=pECI->getxmin(), ymin_src=pECI->getymin(), xmax_src=pECI->getxmax(), ymax_src=pECI->getymax();
	double resx_src=pECI->getresx(), resy_src=pECI->getresy(), resx_dst=pImageOut->getresx(), resy_dst=pImageOut->getresy();
	double ratio_x=resx_dst/resx_src, ratio_y=resy_dst/resy_src;

	// L'image reechantillonnee est limitee a l'image de sortie
	double xmin_dst=__max(xmin_src+K.size(ratio_x),pImageOut->getxmin()), xmax_dst=__min(xmax_src-K.size(ratio_x),pImageOut->getxmax()),
	       ymin_dst=__max(ymin_src+K.size(ratio_y),pImageOut->getymin()), ymax_dst=__min(ymax_src-K.size(ratio_y),pImageOut->getymax());

	// Exception : l'image d'entree n'intersecte pas l'image finale
        if (xmax_src-K.size(ratio_x)<pImageOut->getxmin() || xmin_src+K.size(ratio_x)>pImageOut->getxmax() || ymax_src-K.size(ratio_y)<pImageOut->getymin() || ymin_src+K.size(ratio_y)>pImageOut->getymax())
{
                LOGGER_ERROR("Un paquet d'images (homogenes en résolutions et phase) est situe entierement a l'exterieur de la dalle finale");
		return NULL;	
        }
	
	// Coordonnees de l'image reechantillonnee en pixels
	xmin_dst/=resx_dst;
	xmin_dst=floor(xmin_dst+0.1);
	ymin_dst/=resy_dst;
        ymin_dst=floor(ymin_dst+0.1);
	xmax_dst/=resx_dst;
        xmax_dst=ceil(xmax_dst-0.1);
	ymax_dst/=resy_dst;
        ymax_dst=ceil(ymax_dst-0.1);
	// Dimension de l'image reechantillonnee
	int width_dst = int(xmax_dst-xmin_dst+0.1);
        int height_dst = int(ymax_dst-ymin_dst+0.1);
	//LOGGER_DEBUG(width_dst<<" "<<height_dst<<"   "<<xmin_dst<<" "<<ymax_dst<<" "<<xmax_dst<<" "<<ymin_dst);
	//LOGGER_DEBUG(xmin_dst*resx_dst<<" "<<ymax_dst*resx_dst<<" "<<xmax_dst*resx_dst<<" "<<ymin_dst*resx_dst);
	xmin_dst*=resx_dst;
	xmax_dst*=resx_dst;
	ymin_dst*=resy_dst;
        ymax_dst*=resy_dst;
	//LOGGER_DEBUG(xmin_dst<<" "<<ymax_dst<<" "<<xmax_dst<<" "<<ymin_dst);

	double off_x=(xmin_dst-xmin_src)/resx_src,off_y=(ymax_src-ymax_dst)/resy_src;

	BoundingBox<double> bbox_dst(xmin_dst, ymin_dst, xmax_dst, ymax_dst);

	// Reechantillonnage
	ResampledImage* pRImage = new ResampledImage(pECI, width_dst, height_dst, off_x, off_y, ratio_x, ratio_y, interpolation, bbox_dst);

	// Reechantillonage du masque
	resampledMask = new ResampledImage( mask, width_dst, height_dst, off_x, off_y, ratio_x, ratio_y, interpolation, bbox_dst);

	return pRImage;
*/
        double xmin=pECI->getxmin(), ymin=pECI->getymin(), xmax=pECI->getxmax(), ymax=pECI->getymax() ;
        double resx_src = pECI->getresx(),resy_src = pECI->getresy(), resx_dst=pImageOut->getresx(), resy_dst=pImageOut->getresy() ;
        double ratiox = resx_dst / resx_src ;
        double ratioy = resy_dst / resy_src ;
        double offx = ratiox * interpolation, offy = ratioy * interpolation ;

        int newwidth = int( ((xmax - xmin)/resx_src - 2*offx) / ratiox + 0.5 );
        int newheight = int( ((ymax - ymin)/resy_src - 2*offy) / ratioy + 0.5 );

        BoundingBox<double> newbbox(xmin + resx_dst*interpolation, ymin + resy_dst*interpolation, xmax -interpolation*resx_src, ymax -interpolation*resy_dst);

        // Reechantillonnage
        ResampledImage* pRImage = new ResampledImage(pECI, newwidth, newheight, offx, offy, ratiox, ratioy, interpolation, newbbox );

        // Reechantillonage du masque
        resampledMask = new ResampledImage( mask, newwidth, newheight, offx, offy, ratiox, ratioy, interpolation, newbbox );

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

int mergeTabDalles(LibtiffImage* pImageOut, std::vector<std::vector<Image*> >& TabImageIn, ExtendedCompoundImage** ppECImage, Kernel::KernelType& interpolation, char* nodata)
{
	extendedCompoundImageFactory ECImgfactory ;
	std::vector<Image*> pOverlayedImage;
	std::vector<Image*> pMask;

	for (unsigned int i=0; i<TabImageIn.size(); i++) {
		// Mise en superposition du paquet d'images en 2 etapes

	        // Etape 1 : Creation d'une image composite
        	ExtendedCompoundImage* pECI = compoundDalles(TabImageIn.at(i),nodata);
		ExtendedCompoundMaskImage* mask = new ExtendedCompoundMaskImage(pECI);

	        if (pECI==NULL) {
        	        LOGGER_ERROR("Impossible d'assembler les images");
                	return -1;
	        }
		if (areOverlayed(pImageOut,pECI))
		{
			pOverlayedImage.push_back(pECI);
			//saveImage(pECI,"test.tif",3,8,PHOTOMETRIC_RGB);
			pMask.push_back(mask);
		}
		else {
        		// Etape 2 : Reechantillonnage de l'image composite si necessaire
			
			// addMirrors(pECI);

			ResampledImage* pResampledMask;
	        	ResampledImage* pRImage = resampleDalles(pImageOut, pECI, interpolation, mask, pResampledMask);

        		if (pRImage==NULL) {
                		LOGGER_ERROR("Impossible de reechantillonner les images");
	                	return -1;
			}
			pOverlayedImage.push_back(pRImage);
			//saveImage(pRImage,"test2.tif",1,8,PHOTOMETRIC_MINISBLACK);
			pMask.push_back(pResampledMask);
			//saveImage(pRImage,"test.tif",1,8,PHOTOMETRIC_MINISBLACK);
			//saveImage(mask,"test1.tif",1,8,PHOTOMETRIC_MINISBLACK);
			//saveImage(pResampledMask,"test2.tif",1,8,PHOTOMETRIC_MINISBLACK);
        	}
	}

	// Assemblage des paquets et decoupage aux dimensions de la dalle de sortie
	uint8_t r=h2i(nodata[0])*16 + h2i(nodata[1]);
	if ( (*ppECImage = ECImgfactory.createExtendedCompoundImage(pImageOut->width, pImageOut->height,
			pImageOut->channels, pImageOut->getbbox(), pOverlayedImage,pMask,r))==NULL) {
		LOGGER_ERROR("Erreur lors de la fabrication de l image finale");
		return -1;
	}
	(*ppECImage)->getbbox().print();

//	delete pTabResampledImage;

	return 0;
}

/**
* Fonction principale
*/

int main(int argc, char **argv) {
	char liste_dalles_filename[256], nodata[6];
	uint16_t sampleperpixel, bitspersample, sampleformat, photometric;
	int type=-1;
	Kernel::KernelType interpolation;

	LibtiffImage * pImageOut ;
	std::vector<Image*> ImageIn;
	std::vector<std::vector<Image*> > TabImageIn;
	ExtendedCompoundImage* pECImage;

	/* Initialisation des Loggers */
        Accumulator* acc = new RollingFileAccumulator("/var/tmp/be4",86400,1024);
        Logger::setAccumulator(DEBUG, acc);
        Logger::setAccumulator(INFO , acc);
        Logger::setAccumulator(WARN , acc);
        Logger::setAccumulator(ERROR, acc);
        Logger::setAccumulator(FATAL, acc);

	std::ostream &log = LOGGER(DEBUG);
        log.precision(20);
	log.setf(std::ios::fixed,std::ios::floatfield);

	// Lecture des parametres de la ligne de commande
	if (parseCommandLine(argc, argv,liste_dalles_filename,interpolation,nodata,type,sampleperpixel,bitspersample,sampleformat,photometric)<0){
		LOGGER_ERROR("Echec lecture ligne de commande");
		sleep(1);
		return -1;
	}

	// TODO : gérer le type mtd !!
	if (type==0) {
		LOGGER_ERROR("Le type mtd n'est pas pris en compte");
		sleep(1);
		return -1;
	}

	// Chargement des dalles
	if (loadDalles(liste_dalles_filename,&pImageOut,&ImageIn,sampleperpixel,bitspersample,photometric)<0){
		LOGGER_ERROR("Echec chargement des dalles"); 
		sleep(1);
		return -1;
	}

	// Controle des dalles
	if (checkDalles(pImageOut,ImageIn)<0){
		LOGGER_ERROR("Echec controle des dalles");
		sleep(1);
		return -1;
	}
LOGGER_DEBUG("Sort ");
	// Tri des dalles
	if (sortDalles(ImageIn, &TabImageIn)<0){
		LOGGER_ERROR("Echec tri des dalles");
		sleep(1);
		return -1;
	}
LOGGER_DEBUG("Merge "<<ImageIn.size()<<" "<<TabImageIn.size()<<" "<<TabImageIn[0].size());
	// Fusion des paquets de dalles
	if (mergeTabDalles(pImageOut, TabImageIn, &pECImage, interpolation,nodata) < 0){
		LOGGER_ERROR("Echec fusion des paquets de dalles");
		sleep(1);
		return -1;
	}
LOGGER_DEBUG("Save");
	// Enregistrement de la dalle fusionnee
	if (saveImage(pECImage,pImageOut->getfilename(),pImageOut->channels,bitspersample,sampleformat,photometric)<0){
		LOGGER_ERROR("Echec enregistrement dalle finale");
		sleep(1);
		return -1;
	}

	// TODO Nettoyage
/*	delete pImageOut ;
	delete pECImage ;
*/
	LOGGER_INFO( " dalles_base ; fin du programme " );

	return 0;
}
