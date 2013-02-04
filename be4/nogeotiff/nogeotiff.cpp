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

/* Ce programme a pour but d'oter les entetes geotiff de l'image en parametre.
   Il considere pour cela qu'il y a 3 champs geotiff et qu'il sont toujours les 
   dernier du repertoire de l'image il est possible que cela ne fonctionne que 
   pour les image planet observer (et encore par pour les dalles de mer) */

#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include "../be4version.h"

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
    std::cout << "nogeotiff version "<< BE4_VERSION << std::endl;
    std::cout << std::endl << "nogeotiff: delete geotiff tag" << std::endl; 
    std::cout << "usage: nogeotiff <filename>" << std::endl << std::endl; 
    std::cout << "Kind of a prototype..." << std::endl; 
    std::cout << "Drop the 3 last fields of the first directory" << std::endl; 
    std::cout << "Do not handle bigendian yet" << std::endl; 
    std::cout << "Do not handle multi directories tiff yet" << std::endl << std::endl; 
    return(0);
  }
  if (argc > 2){
    std::cerr << "nogeotiff: too many parameters! 1 expected." << std::endl;
    return(1);
  }
  
  // ouverture du fichier
  FILE * pFile; 
  long file_size; 
  pFile = fopen (argv[1],"r+b"); 
  if (pFile==NULL) {
    std::cerr << "nogeotiff: Unable to open file" << std::endl; 
    return(2);
  } 

  // taille du fichier
  fseek (pFile, 0, SEEK_END); 
  file_size=ftell (pFile); 
  // std::cout << "Size of file: "<< file_size << " bytes."<< std::endl;
  if (file_size < 32){
    std::cerr << "nogeotiff: File is too short (<32B)" << std::endl; 
    return(3);
  }
  rewind (pFile);
  
  // ordre des octets
  char byte_order_tag[2];
  if (fread (byte_order_tag,1,2,pFile)!=2){std::cerr << "nogeotiff: Can't read in File" << std::endl; return(4);}
  // std::cout << "byte_order:"<< byte_order_tag<< std::endl;
    /* TODO:
     * "II": little-endian
     * "MM": big-endian*/
  
  // controle du type tiff
  uint16_t tiff_file_tag;
  if (fread (&tiff_file_tag,2,1,pFile)!=1){ std::cerr << "nogeotiff: Can't read in File" << std::endl;  return(4); }
  // std::cout << "tiff_tag:" << tiff_file_tag << std::endl;
  if (tiff_file_tag != 42){
    std::cerr << "nogeotiff: This is not a tiff file." << std::endl;
    return(5);
  }
  
  // read de first directory offset
  uint32_t dir_offset;
  if (fread (&dir_offset,sizeof(uint32_t),1,pFile)!=1){std::cerr << "nogeotiff: Can't read in File" << std::endl; return(4);}
  // std::cout << "dir_offset:" << dir_offset << std::endl;
  if (dir_offset + 2 > file_size){
    std::cerr << "nogeotiff: File is too short (dir offset)" << std::endl;
    return(3);
  }

  // read the number of entries
  uint16_t dir_entries_num;
  fseek(pFile, dir_offset, SEEK_SET);
  if (fread (&dir_entries_num,sizeof(uint16_t),1,pFile)!=1){std::cerr << "nogeotiff: Can't read in File" << std::endl; return(4);}
  // std::cout << "dir_entries_num:" << dir_entries_num << std::endl;
  if (dir_offset + 2 + dir_entries_num + 12 > file_size){
    std::cerr << "File is too short (dir entries)" << std::endl;
    return(3);
  }
  if (dir_entries_num > 256){
    std::cerr << "nogeotiff: nogeotiff don't check image with more than 256 entries in a directory" << std::endl;
    return(0);
  }

  // drop the three last fields (hopefully being the geotiff tags...)
  dir_entries_num = dir_entries_num - 3;
  fseek(pFile, dir_offset, SEEK_SET);
  if (fwrite(&dir_entries_num,sizeof(uint16_t),1,pFile)!=1){std::cerr << "nogeotiff: Write in File" << std::endl; return(5);}

  fseek(pFile, dir_offset + 2 + dir_entries_num * 12, SEEK_SET);
  uint32_t next_dir_offset = 0;
  if (fwrite(&next_dir_offset, sizeof(uint32_t), 1, pFile) != 1){std::cerr << "nogeotiff: Write in File" << std::endl; return(5);}

  fclose (pFile); 
  std::cout << "nogeotiff: Done!" << std::endl ;
  return 0;
}  
