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


Contrainte:
1. Toutes les dalles sont dans le meme SRS (pas de reprojection)
*/


using namespace std;
#include <iostream> //pour cout, endl

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include "tiffio.h"
#include <jpeglib.h>

#include "CDalle.h"

#include "TiffReader.h"
#include "TiledTiffWriter.h"
#include "Logger.h"

#include "Image.h"
#include "LibtiffImage.h"
#include "CompoundImage.h"
#include "ResampledImage.h"
#include "ExtendedCompoundImage.h"

// -----------  VARIABLES GLOBALES --------------------------
	char * pfiletext = 0, * nodata = 0 ;
	Kernel::KernelType interpolation = Kernel::LANCZOS_3 ;

	uint32_t bitspersample = 8; // il faut lui mettre une valeur sinon valgrind pas content!!
	uint16_t sampleperpixel = 3 ;

	vector<CDalle> lesDallesEnEntree;
	CDalle dalleEnSortie ;
	vector< GeoreferencedImage* > lesGeoreferencedImages ;
// -----------------------------------------------------------

int creerFichierSurDisque(char * pName, Image * pImg) {
	TIFF* output=TIFFOpen(pName,"w");
	if (!output) {
		LOGGER_ERROR( " Impossible d'ouvrir le fichier en ecriture ! " );
		return 1;
	}

	TIFFSetField(output, TIFFTAG_IMAGEWIDTH, pImg->width);
	TIFFSetField(output, TIFFTAG_IMAGELENGTH, pImg->height);

	TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);
	TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, bitspersample);
	TIFFSetField(output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB); // PHOTOMETRIC_MINISBLACK
	TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, 1);
	TIFFSetField(output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

	unsigned char * buf_ligne = (unsigned char *) malloc((pImg->width)*sampleperpixel*bitspersample/8 );
	for(int ligne = 0; ligne < pImg->height; ligne++)	{
		pImg->getline(buf_ligne,ligne);
		TIFFWriteScanline(output, buf_ligne, ligne, 0); // en contigu (entrelace) on prend chanel=0 (!)
	}
	free(buf_ligne);
	TIFFClose(output);
	return 0;
}

void usage() {
	cout << endl ;
	cout << "USAGE : CreerDalleCacheNiveauMinimum -file -interpolation -nodata -entree -sortie " << endl ;
	cout << " expl : CreerDalleCacheNiveauMinimum -f textfile -i [lanczos/ppv/bicubique] -n [couleur_hexa] " << endl ;
}

void init(int argc, char** argv) {

	if (argc != 7) {
		fprintf(stderr, " Nombre de parametres incorrect : %s !! \n", argv[0]);  exit(2);;
	}

	// ETAPE 0 : lecture des parametres
	for(int i = 1; i < argc; i++) {
		cout << " " << argv[i] << " " << argv[i+1] ;
		if(argv[i][0] == '-') {
			switch(argv[i][1]) {
			case 'f': // fichier texte de parametres
				if(i++ >= argc) {cerr << "Erreur sur l'option -f " << endl; exit(2);}
				pfiletext = argv[i];
				break;
			case 'i': // interpolation
				if(i++ >= argc) {cerr << "Erreur sur l'option -i " << endl; exit(2);}
				if(strncmp(argv[i], "lanczos",7) == 0) interpolation = Kernel::LANCZOS_3;
				else if(strncmp(argv[i], "ppv",3) == 0) interpolation = Kernel::NEAREST_NEIGHBOUR;
				else if(strncmp(argv[i], "bicubique",9) == 0) interpolation = Kernel::CUBIC;
				else {cerr << "Erreur sur l'option -i " << endl; exit(2);}
				break;
			case 'n': // nodata
				if(i++ >= argc) {cerr << "Erreur sur l'option -n " << endl; exit(2);}
				nodata = argv[i];
				break;
			default: usage(); exit (1);
			}
		}
	}
	cout << endl ;


	// ETAPE 1 : ouverture du fichier texte contenant les parametres :

	FILE * pFichierParametres;
	pFichierParametres = fopen (pfiletext,"r");
	if (pFichierParametres==NULL) {
		fprintf(stderr, " impossible d'ouvrir le fichier %s ! \n", pfiletext);  exit(2);
	}

	// gestion de la dalle à créer en sortie de programme:
	float xmin=1, xmax=1, ymin=1, ymax=1 ;
	float resx=0.0, resy=0.0 ;
	char nom [128];  //char* nom = new char[128];

	int ret = -1 ;
	ret = fscanf (pFichierParametres, "%s %f %f %f %f %f %f", nom, &xmin, &xmax, &ymin, &ymax, &resx, &resy );
	if (ret < 1 ) {
		LOGGER_ERROR( " fichier texte: dalle en sortie: ligne 1 : mal formatté ! " );  exit(2);
	}
	if (ret != 7 ) {
		LOGGER_ERROR( " fichier texte: dalle en sortie: ligne 1 : nombre de champs incorrect" );  exit(2);
	}
	dalleEnSortie = CDalle(nom, xmin, xmax, ymin, ymax, resx, resy);

	cout << " info: Image à créer : " << endl; // debug
	dalleEnSortie.affiche()  ; // debug

	// gestion des dalles en entrée de programme :
	float _resx=0.0, _resy=0.0 ;
	int iligne = 1 ;
	while ( (ret = fscanf(pFichierParametres, "%s %f %f %f %f %f %f", nom, &xmin, &xmax, &ymin, &ymax, &_resx, &_resy)) > 0 ) {
		iligne++ ;
		if (ret < 1 ) {
			LOGGER_ERROR( " fichier texte: dalles en entree: ligne " << iligne << " : mal formattée! " );  exit(2);
		}
		if (ret != 7 ) {
			LOGGER_ERROR( " fichier texte: dalles en entree: ligne " << iligne << " : nombre de champs incorrect!" );  exit(2);
		}
		if ( (_resx!=resx) || (_resy!=resy) ) {
			LOGGER_ERROR( " dalles en entree: ligne " << iligne << " : la resolution n'est pas identique à l'image à produire!" );  exit(2);
		}
		lesDallesEnEntree.push_back( CDalle(nom, xmin, xmax, ymin, ymax, resx, resy) );
	}
	fclose (pFichierParametres);

	cout << " info: nombre de dalles source : " << lesDallesEnEntree.size() << endl ; // debug
	for(uint i=0; i<lesDallesEnEntree.size(); i++ )  // debug
		lesDallesEnEntree.at(i).affiche() ; // debug

}

int creerImageGlobale() {
	// ETAPE 2 : creation de la ExtendedCompoundImage

	// creation du vector< GeoreferencedImage* > :
	cout << " info: creation du vector< GeoreferencedImage* > " << endl ;
	libtiffImageFactory LTImgfactory;

	for( uint i = 0; i < lesDallesEnEntree.size(); i++ ) {
		LibtiffImage* pLTImg = LTImgfactory.createLibtiffImage( lesDallesEnEntree.at(i).getNom() );
		pLTImg->setx0( lesDallesEnEntree.at(i).getXmin() );
		pLTImg->sety0( lesDallesEnEntree.at(i).getYmax() );
		pLTImg->setresx( lesDallesEnEntree.at(i).getResx() );
		pLTImg->setresy( lesDallesEnEntree.at(i).getResy() );
		lesGeoreferencedImages.push_back( pLTImg ) ;
	}


	cout << " info: debuggage: " << endl ;
	for( uint i = 0; i < lesGeoreferencedImages.size(); i++ ) {
		cout << lesGeoreferencedImages.at(i)->getxmin() << " " << lesGeoreferencedImages.at(i)->getymax() << " " ;
		cout << lesGeoreferencedImages.at(i)->getxmax() << " " << lesGeoreferencedImages.at(i)->getymin() << " " << endl;
	}

	cout << " info: creation de la ExtendedCompoundImage " << endl ;
	extendedCompoundImageFactory ECImgfactory ;
	ExtendedCompoundImage* pECImage = ECImgfactory.createExtendedCompoundImage(
			4000, 4000, 3,
			/*lesDallesEnEntree.at(0).getXmin(), lesDallesEnEntree.at(0).getYmax(),
		lesDallesEnEntree.at(0).getResx(), lesDallesEnEntree.at(0).getResy(),*/
			0, 2000,
			0.5, 0.5,
			lesGeoreferencedImages);
	cout << " info: ExtendedCompoundImage créée : " << pECImage->width << " " << pECImage->height << endl ;

	// // ETAPE 2 : creation de la compoundImage : creation du vector< vector< Image* > > :
	//	vector< Image* > ligneDeDalles ;
	//	vector< vector< Image* > > tableauDeDalles ;
	//	int nbcol = 2, nblig = 2 ;
	//	libtiffImageFactory factory;
	//	for( int ilign = 0; ilign < nblig; ilign++ ) {
	//		ligneDeDalles = vector< Image* >() ;
	//		for( int icol = 0; icol < nbcol; icol++ ) {
	//			LibtiffImage *pImg = factory.createLibtiffImage(lesDallesEnEntree.at(ilign*nbcol+icol).getNom() );
	//			ligneDeDalles.push_back( pImg ) ;
	//		}
	//		tableauDeDalles.push_back( ligneDeDalles );
	//	}
	//	CompoundImage compoundimg = CompoundImage(tableauDeDalles) ;
	//	cout << " compoundImage créée : " << compoundimg.height << " par " << compoundimg.width << endl ;

	// ETAPE 3 : creation du fichier image de sortie à partir de la ExtendedCompoundImage :
	cout << " info: creation du fichier image de sortie à partir de la ExtendedCompoundImage " << endl ;

	if ( creerFichierSurDisque(dalleEnSortie.getNom(), pECImage)!=0 ) {
		LOGGER_ERROR( " Impossible de creer le fichier image de sortie ! " );  return 30;
	}
	cout << " Le fichier image de sortie est créé sur le disque." << endl ;

	// ETAPE 4 :
	cout << " info: creation du fichier image resamplé à partir de la ExtendedCompoundImage " << endl ;

	ResampledImage RImage(pECImage,1500, 1500, 5, 5, 2, 2, interpolation); // left et top doivent etre ratio x size_du_kernel
	char tmp[10]={'r','e','s','i','z','e','.','t','i','f' };
	if ( creerFichierSurDisque( tmp, &RImage)!=0 ) {
		LOGGER_ERROR( " Impossible de creer le fichier image resamplé ! " );  return 31;
	}
	cout << " Le fichier image resamplé est créé sur le disque." << endl ;
	return 0;

}


int main(int argc, char **argv) {

	Logger::configure("/home/alain/cpp/gpp3/wmts/config/logConfigBee4.xml");
	LOGGER_INFO( " Programme CreerDallesCacheNiveauMinimum ---------" );

	// lecture des parametres et creation des objets dalles (1 en sortie, n en entrée)
	init(argc, argv);

	// creation d'une image "globale" : une ExtendedCompoundImage :
	creerImageGlobale();

	LOGGER_INFO( " FIN DU  Programme CreerDallesCacheNiveauMinimum ---------" );
	return 0;
}


void RecopierUneImageTiff () {

	//	TIFF         * input, * output;
	//	uint32_t width, length;
	//	uint16_t compression ; // = COMPRESSION_NONE;
	//	uint16_t photometric ; // PHOTOMETRIC_RGB PHOTOMETRIC_MINISBLACK
	//	uint32_t bitspersample = 8; // il faut lui mettre une valeur sinon valgrind gueule!!
	//	uint16_t sampleperpixel ;
	//	int sampleSize;        // Taille en octet d'un pixel (RGB = 3, Gray = 1)
	//
	//	input = TIFFOpen(filein, "r"); // NB: TIFFOpen est verbeux...
	//	if(!input) {cerr << " Impossible d'ouvrir le fichier !" << endl; exit(2);}
	//	TIFFGetField(input, TIFFTAG_IMAGEWIDTH, &width);
	//	TIFFGetField(input, TIFFTAG_IMAGELENGTH, &length);
	//	TIFFGetField(input, TIFFTAG_COMPRESSION, &compression);
	//	TIFFGetField(input, TIFFTAG_PHOTOMETRIC, &photometric);
	//	TIFFGetFieldDefaulted(input, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	//	//    TIFFGetField(input, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	//	TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel);
	//	sampleSize = (bitspersample * sampleperpixel) / 8;
	//
	//	cout << " width:" << width << " length:" << length << endl;
	//	cout << " compression:" << compression << " photometric:" << photometric << endl;
	//	cout << " bitspersample:" << bitspersample << " sampleperpixel:" << sampleperpixel << endl;
	//	if(compression != COMPRESSION_NONE) cerr << " Compression non supportee !!!" << endl;
	//
	//	if ((output = TIFFOpen(fileout, "w")) == NULL) {
	//		fprintf(stderr, " impossible d'ouvrir le fichier %s ! \n", fileout);        return 0;
	//	}
	//
	//	TIFFSetField(output, TIFFTAG_IMAGEWIDTH, width);
	//	TIFFSetField(output, TIFFTAG_IMAGELENGTH, length);
	//	TIFFSetField(output, TIFFTAG_COMPRESSION, compression);
	//	TIFFSetField(output, TIFFTAG_PHOTOMETRIC, photometric);
	//	TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, bitspersample);
	//	TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);
	//	TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	//
	//	TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, 1);
	//	TIFFSetField(output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);


	//  int TIFFReadScanline(TIFF *tif, tdata_t buf, uint32 row, tsample_t sample)
	//    Read the data for the specified row into the (user supplied) data buffer buf
	//	int TIFFWriteScanline(TIFF *tif, tdata_t buf, uint32 row, tsample_t sample)
	//	  Write data to a file at the specified row.
	//
	//    uint32_t i;
	//    unsigned char * scan_line = (unsigned char *) malloc(width*sampleperpixel );
	//    for (i = 0; i < (width*sampleperpixel ); i++)
	//        scan_line[i] = 255; // 0 noir / 255 blanc
	//      for (i = 0; i < length / 2; i++)
	//        TIFFWriteScanline(output, scan_line, i, 0);
	//    for (i = 0; i < (width*sampleperpixel ); i++)
	//        scan_line[i] = 0; // 0 noir / 255 blanc
	//      for (i = length / 2; i < length; i++)
	//        TIFFWriteScanline(output, scan_line, i, 0);
	//    free(scan_line);


	//  tsize_t TIFFReadRawStrip(TIFF *tif, tstrip_t strip, tdata_t buf, tsize_t size)
	//    Read the contents of the specified strip into the (user supplied) data buffer
	//  tsize_t TIFFWriteRawStrip(TIFF *tif, tstrip_t strip, tdata_t buf, tsize_t size)
	//    Append size bytes of raw data to the specified strip
	//
	//	int tiled_image ;
	//	unsigned char *buf_data ;
	//	tsize_t chunk_size, byte_count ;
	//	uint32 chunk_no, num_chunks, *bc ;
	//	tiled_image = TIFFIsTiled(input) ;
	//
	//	if (tiled_image) {
	//		num_chunks = TIFFNumberOfTiles(input);
	//		TIFFGetField(input, TIFFTAG_TILEBYTECOUNTS, &bc);
	//	} else {
	//		num_chunks = TIFFNumberOfStrips(input);
	//		TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &bc);
	//	}
	//	if (tiled_image)
	//		chunk_size = TIFFTileSize(input);
	//	else
	//		chunk_size = TIFFStripSize(input);
	//	buf_data = (unsigned char *)_TIFFmalloc(chunk_size);
	//	if (!buf_data) {
	//		cerr << " Erreur a la crea du buffer " << endl; return 21;
	//	}
	//	for (chunk_no = 0; chunk_no < num_chunks; chunk_no++) {
	//		if (tiled_image)
	//			byte_count = TIFFReadRawTile(input, chunk_no, buf_data, chunk_size);
	//		else
	//			byte_count = TIFFReadRawStrip(input, chunk_no, buf_data, chunk_size);
	//		if (byte_count < 0) {
	//			cerr << " Erreur a la lecture du rawStrip " << endl; return 21;
	//		}
	//		if (tiled_image)
	//			TIFFWriteRawTile(input, chunk_no, buf_data, chunk_size);
	//		else
	//			TIFFWriteRawStrip(output, chunk_no, buf_data, byte_count);
	//	}
	//	_TIFFfree( buf_data );

	// code bon :
	//	unsigned char * buf_ligne = (unsigned char *) malloc(width*sampleperpixel );
	//	for(int isample = 0; isample < sampleperpixel; isample++) {
	//		cout << " isample:" << isample << endl;
	//		for(uint32_t y = 0; y < length; y++) {
	//			int total, total2 ;
	//			if((total = TIFFReadScanline(input, buf_ligne, y, isample)) < 0) {
	//				cerr << " Erreur a la lecture du fichier " << endl; return 2;
	//			}
	//			if((total2 = TIFFWriteScanline(output, buf_ligne, y, isample)) < 0) {
	//				cerr << " Erreur a l'ecriture du fichier " << endl; return 2;
	//			}
	//		}
	//	}
	//	free(buf_ligne);
	//	TIFFClose(input);    TIFFClose(output);


	//arguments: -f fichiertest.txt -c none -e 37-2007-0529-6683-LA93.tif -s result.tif
	//	uint32_t tilewidth = 256, tilelength = 256;
	//	// etape 1 : on affiche les infos sur l'image en entree
	//	TiffReader R(filein);
	//	uint32_t width_in = R.getWidth();
	//	uint32_t length_in = R.getLength();
	//	photometric = R.getPhotometric() ;
	//	bitspersample = R.getBitsPerSample() ;
	//	cout << " image en entree::::::::::::::::::::::::::::::::::::::: " << endl;
	//	cout << "     largeur_in: " << width_in << " longueur_in: " << length_in << endl;
	//	cout << "     photometric: " << photometric << " bitspersample: " << bitspersample << " SampleSize: " << R.getSampleSize() << endl;
	//
	//	// etape 2 : on cree l'image de sortie
	//	if(width_in % tilewidth || length_in % tilelength) {cerr << "Image size must be a multiple of tile size" << endl; exit(2);}
	//	cout << " On appelle le TiledTiffWriter:  tilewidth: " << tilewidth << " tilelength: " << tilelength << " quality: " << quality << endl;
	//	// on cree le fichier tuilé de sortie (avec son entete de fichier)
	//	TiledTiffWriter W(fileout, width_in, length_in, photometric, compression, quality, tilewidth, tilelength,bitspersample);
	//
	//	// etape 3 : on cree le buffer d'echange
	//	int tilex = width_in / tilewidth;
	//	int tiley = length_in / tilelength;
	//	cout << "     nb tile en x: " << tilex << " nb tile en y: " << tiley << endl;
	//	uint8_t* data = new uint8_t[ tilelength*tilewidth*R.getSampleSize() ]; // nb d'octets par tuile
	//
	//	// etape 4 : on cree l'image tuilee de sortie
	//	for(int y = 0; y < tiley; y++) {
	//		for(int x = 0; x < tilex; x++) {
	//			// on va chercher la fenetre 0 0, puis 0 1000, puis 1000 0, puis 1000 1000
	//			R.getWindow(x*tilewidth , y*tilelength , tilewidth, tilelength, data);
	//			//cout << "  Ecriture de la tuile : " << x << "," << y << " " << endl;
	//			if(W.WriteTile(x, y, data) < 0) {
	//				cerr << "Error while writting tile (" << x << "," << y << ")" << endl; return 2;
	//			}
	//		}
	//	}
	//
	//	// etape 5 : on ferme les fichiers et on affiche les infos sur le fichier cree
	//	R.close();
	//	if(W.close() < 0) {cerr << "Error while writting index" << endl; return 2;}
	//	TiffReader RR(fileout);
	//	cout << " image en sortie:::::::  largeur_out: " << RR.getWidth() << " longueur_out: " << RR.getLength() << endl;
	//	cout << "       photometric: " << RR.getPhotometric() << " bitspersample: " << RR.getBitsPerSample() << " SampleSize: " << RR.getSampleSize() << endl;
	//	RR.close();


}

FILE * ouvrirFichier (char *filename) {
	FILE * pFile;
	pFile = fopen (filename,"r");
	if (pFile==NULL) {
		fprintf(stderr, " impossible d'ouvrir le fichier %s ! \n", filename);
	}
	return pFile ;
}

int afficherFichier (char *filename) {
	FILE * pFile;
	pFile = ouvrirFichier(filename);
	if (pFile==NULL) {
		fprintf(stderr, " impossible d'ouvrir le fichier %s ! \n", filename);
		return -1;
	}
	int num = 0 ;
	char ligne [256] ; // tampon d'une ligne limitee à 256 caracteres
	while ( fgets ( ligne, 256, pFile ) ) {
		num++ ;
		printf("%5d : ", num);
		printf(    "%s", ligne);
	}
	return num ;
}

