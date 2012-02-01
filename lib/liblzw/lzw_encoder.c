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

//#include "lzw_encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

 
/* -------- aux stuff ---------- */
void* mem_alloc(size_t item_size, size_t n_item)
{
    size_t *x = (size_t*) calloc(1, sizeof(size_t)*2 + n_item * item_size);
    x[0] = item_size;
    x[1] = n_item;
    return x + 2;
}
 
void* mem_extend(void *m, size_t new_n)
{
    size_t *x = (size_t*)m - 2;
    x = (size_t*)realloc(x, sizeof(size_t) * 2 + *x * new_n);
    if (new_n > x[1])
        memset((char*)(x + 2) + x[0] * x[1], 0, x[0] * (new_n - x[1]));
    x[1] = new_n;
    return x + 2;
}
 
/* FIXME resoudre les problemes de compilation pour remettre le inline
 * inline void _clear(void *m)
  */
void _clear(void *m)
{
    size_t *x = (size_t*)m - 2;
    memset(m, 0, x[0] * x[1]);
}
 
#define _new(type, n)    mem_alloc(sizeof(type), n)
#define _del(m)        { free((size_t*)(m) - 2); m = 0; }
#define _len(m)        *((size_t*)m - 1)
#define _setsize(m, n)    m = mem_extend(m, n)
#define _extend(m)    m = mem_extend(m, _len(m) * 2)
 
 
/* ----------- LZW stuff -------------- */
typedef uint8_t byte;
typedef uint16_t ushort;
 
#define M_CLR    256    /* clear table marker */
#define M_EOD    257    /* end-of-data marker */
#define M_NEW    258    /* new code index */
 
/* encode and decode dictionary structures.
   for encoding, entry at code index is a list of indices that follow current one,
   i.e. if code 97 is 'a', code 387 is 'ab', and code 1022 is 'abc',
   then dict[97].next['b'] = 387, dict[387].next['c'] = 1022, etc. */
typedef struct {
    ushort next[256];
} lzw_enc_t;
 
 
byte* _lzw_encode(byte *in, size_t len, int max_bits)
{
    int bits = 9, next_shift = 512;
    ushort code, c, nc, next_code = M_NEW;
    lzw_enc_t *d = (lzw_enc_t*) _new(lzw_enc_t, 512);
 
    if (max_bits > 16) max_bits = 16;
    if (max_bits < 9 ) max_bits = 12;
 
    byte *out = (byte *) _new(ushort, 4);
    int out_len = 0, o_bits = 0;
    uint32_t tmp = 0;
    
    inline void write_bits(ushort x) {
        tmp = (tmp << bits) | x;
        o_bits += bits;
        if (_len(out) <= out_len) _extend(out);
        while (o_bits >= 8) {
            o_bits -= 8;
            out[out_len++] = tmp >> o_bits;
            tmp &= (1 << o_bits) - 1;
        }
    }
    
    write_bits(M_CLR);
    for (code = *(in++); --len; ) {
        c = *(in++);
        if ((nc = d[code].next[c])) {
            code = nc;
        }
        else {         
            write_bits(code);
            nc = d[code].next[c] = next_code++;
            code = c;
        }
        

        /* next new code would be too long for current table */
        if (next_code == next_shift) {
            /* either reset table back to 9 bits */
            if (bits+1 > max_bits) {
                /* table clear marker must occur before bit reset */
                /* 256 on 12 bits */
                write_bits(M_CLR);
 
                bits = 9;
                next_shift = 512;
                next_code = M_NEW;
                _clear(d);
                
            } else {   /* or extend table */
                bits++;
                _setsize(d, next_shift *= 2);
            }
        }
        
    }
    
    write_bits(code);
    
    write_bits(M_EOD);
    
    if (tmp) {
        write_bits(tmp);
    }
    
    _del(d);
    
    _setsize(out, out_len);
    
    return out;
}


size_t lzw_encode(byte * in, size_t in_len, byte * buffer){
    
    byte * out;
    out = _lzw_encode(in, in_len, 12);

//  On copie le contenu de out dans buffer. Cette méthode doit être changée à terme.
    for (int i = 0; i<_len(out); i++){
        buffer[i]=out[i];
    }
    
    size_t length = _len(out);
    _del(out);
    
    return length;
}

