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

int main ( int argc, char **argv ) {
    // controle de la ligne de commande
    if ( argc == 1 ) {
        std::cout << "tiffck version "<< BE4_VERSION << std::endl;
        std::cout << std::endl << "tiffck: check tiff image size" << std::endl;
        std::cout << "usage: tiffck <filename>" << std::endl << std::endl;
        std::cout << "Kind of prototype..." << std::endl;
        std::cout << "Do not handle tiled image yet" << std::endl;
        std::cout << "Do not handle bigendian yet" << std::endl;
        std::cout << "Do not handle multi directories tiff yet" << std::endl << std::endl;
        return ( 0 );
    }
    if ( argc > 2 ) {
        std::cerr << "tiffck: too many parameters! 1 expected." << std::endl;
        return ( 1 );
    }

    // ouverture du fichier
    FILE * pFile;
    long file_size;
    pFile = fopen ( argv[1],"rb" );
    if ( pFile==NULL ) {
        std::cerr << "tiffck: Unable to open file" << std::endl;
        return ( 2 );
    }

    // taille du fichier
    fseek ( pFile, 0, SEEK_END );
    file_size=ftell ( pFile );
    // std::cout << "Size of file: "<< file_size << " bytes."<< std::endl;
    if ( file_size < 32 ) {
        std::cerr << "tiffck: File is too short (<32B)" << std::endl;
        return ( 3 );
    }
    rewind ( pFile );

    // ordre des octets
    char byte_order_tag[2];
    if ( fread ( byte_order_tag,1,2,pFile ) !=2 ) {
        std::cerr << "tiffck: Can't read in File" << std::endl;
        return ( 4 );
    }
    // std::cout << "byte_order:"<< byte_order_tag<< std::endl;
    /* TODO:
     * "II": little-endian
     * "MM": big-endian*/

    // controle du type tiff
    uint16_t tiff_file_tag;
    if ( fread ( &tiff_file_tag,2,1,pFile ) !=1 ) {
        std::cerr << "tiffck: Can't read in File" << std::endl;
        return ( 4 );
    }
    std::cout << "tiff_tag:" << tiff_file_tag << std::endl;
    if ( tiff_file_tag != 42 && tiff_file_tag != 10752 ) {
        std::cerr << "tiffck: This is not a tiff file." << std::endl;
        return ( 5 );
    }

    // read de first directory offset
    uint32_t dir_offset;
    if ( fread ( &dir_offset,sizeof ( uint32_t ),1,pFile ) !=1 ) {
        std::cerr << "tiffck: Can't read in File" << std::endl;
        return ( 4 );
    }
    std::cout << "dir_offset:" << dir_offset << std::endl;
    std::cout << "file_size:" << file_size << std::endl;
    if ( dir_offset + 2 > file_size ) {
        std::cerr << "tiffck: File is too short (dir offset)" << std::endl;
        return ( 3 );
    }

    // read the number of entries
    uint16_t dir_entries_num;
    fseek ( pFile, dir_offset, SEEK_SET );
    if ( fread ( &dir_entries_num,sizeof ( uint16_t ),1,pFile ) !=1 ) {
        std::cerr << "tiffck: Can't read in File" << std::endl;
        return ( 4 );
    }
    // std::cout << "dir_entries_num:" << dir_entries_num << std::endl;
    if ( dir_offset + 2 + dir_entries_num + 12 > file_size ) {
        std::cerr << "File is too short (dir entries)" << std::endl;
        return ( 3 );
    }
    if ( dir_entries_num > 256 ) {
        std::cerr << "tiffck: tiffck don't check image with more than 256 entries in a directory" << std::endl;
        return ( 0 );
    }

    // controle de toutes les entries
    Entry entries[dir_entries_num];
    if ( fread ( entries,sizeof ( Entry ),dir_entries_num,pFile ) !=dir_entries_num ) {
        std::cerr << "tiffck: Can't read in File" << std::endl;
        return ( 4 );
    }

    uint32_t  nb_strip = 0;
    uint32_t* strip_offsets;
    uint32_t* strip_sizes;

    uint8_t type_size[5+1];
    type_size[1]=1; //BYTE
    type_size[2]=1; //ASCII
    type_size[3]=2; //SHORT
    type_size[4]=4; //LONG
    type_size[5]=8; //RATIONAL = 2 LONGs

    for ( int i=0; i<dir_entries_num; i++ ) {
        // std::cout << "entrie tag:" << entries[i].tag << std::endl;

        // cas particulier du STRIP_OFFSETS
        if ( entries[i].tag == STRIP_OFFSETS ) {
            //std::cout << "STRIP_OFFSETS" << std::endl ;
            nb_strip = entries[i].count;
            strip_offsets = new uint32_t[nb_strip];

            if ( nb_strip == 1 ) {
                strip_offsets[0] = entries[i].value;
            } else {
                uint32_t values_offset = entries[i].value;
                if ( values_offset + type_size[entries[i].type] * nb_strip > file_size ) {
                    std::cerr << "tiffck: File is too short (strip offset values)" << std::endl;
                    return ( 3 );
                }
                fseek ( pFile, values_offset, SEEK_SET );
                if ( fread ( strip_offsets, sizeof ( uint32_t ), nb_strip, pFile ) != nb_strip ) {
                    std::cerr << "tiffck: Can't read in File" << std::endl;
                    return ( 4 );
                }
            }

            // cas particulier du STRIP_BYTE_COUNTS
        } else if ( entries[i].tag == STRIP_BYTE_COUNTS ) {
            //std::cout << "STRIP_BYTE_COUNTS" << std::endl ;
            if ( nb_strip != entries[i].count ) {
                std::cerr << "tiffck: not the same number of strip offset and strip size!" << std::endl;;
                return 6;
            }
            strip_sizes = new uint32_t[nb_strip];

            if ( nb_strip == 1 ) {
                strip_sizes[0] = entries[i].value;
            } else {
                uint32_t values_offset = entries[i].value;
                if ( values_offset + type_size[entries[i].type] * nb_strip > file_size ) {
                    std::cerr << "tiffck: File is too short (strip size values)" << std::endl;
                    return ( 3 );
                }
                fseek ( pFile, values_offset, SEEK_SET );
                if ( fread ( strip_sizes, sizeof ( uint32_t ), nb_strip, pFile ) != nb_strip ) {
                    std::cerr << "tiffck: Can't read in File" << std::endl;
                    return ( 4 );
                }
            }
        }// cas general, on ne controle que l'emprise des valeurs des entries
        else {
            if ( entries[i].count > 1 ) {
                uint32_t values_offset = entries[i].value;
                if ( values_offset + type_size[entries[i].type] * entries[i].count > file_size ) {
                    std::cerr << "tiffck: File is too short (generic values)" << std::endl;
                    std::cerr << entries[i].tag <<" "<< entries[i].type <<" "<< entries[i].count << std::endl;
                    return ( 3 );
                }
            }
        }
    }

    // controle de l'emprise des strips
    for ( uint32_t s=0; s<nb_strip; s++ ) {
        if ( strip_offsets[s]+strip_sizes[s] > file_size ) {
            std::cerr << "tiffck: File is too short (image emprise)" << std::endl;
            return ( 3 );
        }
    }
    fclose ( pFile );
    std::cout << "tiffck: All seems ok!" << std::endl ;
    return 0;
}
