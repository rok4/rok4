#ifndef LZW_ENCODER_H
#define LZW_ENCODER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern size_t lzw_encode(uint8_t * in, size_t in_len, uint8_t * out);

#ifdef __cplusplus
}
#endif 

#endif // LZW_ENCODER_H
