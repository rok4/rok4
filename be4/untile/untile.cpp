/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière 
 * 
 * Géoportail SAV <geop_services@geoportail.fr>
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

/* -------------------------------------------------------------------------
   Version prototype de l'outil d'extraction des tuiles d'une dalle de cache
   Permet par exemple de contrôler le contenu d'une dalle tuilée en png.
---------------------------------------------------------------------------*/

#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include <sstream>

#define STRIP_OFFSETS      273
#define ROWS_PER_STRIP     278
#define STRIP_BYTE_COUNTS  279


struct Entry {
  uint16_t tag;
  uint16_t type;
  uint32_t count;
  uint32_t value; 
};

int main(int argc, char **argv) {
  // controle de la ligne de commande
  if (argc == 1){
    std::cout << std::endl << "untile: get tiles out of the tiff" << std::endl; 
    std::cout << "usage: until <filename> [-c <colomn> -r <row> -s <suffix>] !! options are not implemented yet !!" << std::endl << std::endl; 
    std::cout << "Kind of prototype..." << std::endl; 
    std::cout << "o not handle tiled image yet" << std::endl; 
    std::cout << "Do not handle bigendian yet" << std::endl; 
    std::cout << "Do not handle multi directories tiff yet" << std::endl << std::endl; 
    return(0);
  }
  if (argc > 2){
    std::cerr << "untile: too many parameters! 1 expected." << std::endl;
    return(1);
  }
  
  // ouverture du fichier
  FILE * pFile; 
  long file_size; 
  pFile = fopen (argv[1],"rb"); 
  if (pFile==NULL) {
    std::cerr << "untile: Unable to open file" << std::endl; 
    return(2);
  } 

  // taille du fichier
  fseek (pFile, 0, SEEK_END); 
  file_size=ftell (pFile); 
  // std::cout << "Size of file: "<< file_size << " bytes."<< std::endl;
  if (file_size < 2048){
    std::cerr << "untile: File is too short ( < 2048 Bytes can't be a rok4 cache)" << std::endl; 
    return(3);
  }
  rewind (pFile);
  
  // ordre des octets
  char byte_order_tag[2];
  if (fread (byte_order_tag,1,2,pFile)!=2){std::cerr << "untile: Can't read in File" << std::endl; return(4);}
  // std::cout << "byte_order:"<< byte_order_tag<< std::endl;
    /* TODO:
     * "II": little-endian
     * "MM": big-endian*/
  
  // controle du type tiff
  uint16_t tiff_file_tag;
  if (fread (&tiff_file_tag,2,1,pFile)!=1){ std::cerr << "untile: Can't read in File" << std::endl;  return(4); }
  // std::cout << "tiff_tag:" << tiff_file_tag << std::endl;
  if (tiff_file_tag != 42){
    std::cerr << "untile: This is not a tiff file." << std::endl;
    return(5);
  }
  
  int tilesPerHeight = 16;
  int tilesPerWidth  = 16;
  int tilesNumber     = tilesPerWidth*tilesPerHeight;
  uint32_t posoff    = 2048; 
  uint32_t possize   = 2048 + tilesNumber * 4;
  uint32_t offset[tilesNumber];
  uint32_t size[tilesNumber];

  fseek(pFile, 2048, SEEK_SET);

  if (fread (offset, sizeof(uint32_t), tilesNumber, pFile) != tilesNumber){std::cerr << "untile: Can't read in File" << std::endl; return(4);}
  if (fread (size  , sizeof(uint32_t), tilesNumber, pFile) != tilesNumber){std::cerr << "untile: Can't read in File" << std::endl; return(4);}


  for (int n=0; n < tilesNumber; n++){
    uint8_t buff[size[n]];
    fseek(pFile, offset[n], SEEK_SET);
    if (fread (buff, sizeof(uint8_t), size[n], pFile) != size[n]){std::cerr << "untile: Can't read in File" << std::endl; return(4);}
    std::ostringstream name;
    name << n << ".tile";
    FILE * pOuputFile = fopen (name.str().c_str() , "wb");
    fwrite (buff , 1 , sizeof(buff) , pOuputFile );
    fclose (pOuputFile);
  }

  fclose (pFile); 
  std::cout << "untile: All seems ok!" << std::endl ;
  return 0;
}  
