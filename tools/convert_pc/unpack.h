
#ifndef UNPACK_H__
#define UNPACK_H__

#include <stdint.h>

typedef struct {
	int size, datasize;
	uint32_t crc;
	uint32_t chk;
	uint8_t *dst;
	const uint8_t *src;	
} unpack_context_t;


extern int delphine_unpack(const uint8_t *src, int size, uint8_t *dst); // a4, a5


#endif
