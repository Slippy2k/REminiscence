
#ifndef __UNPACK_H__
#define __UNPACK_H__

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef struct {
	int size, datasize;
	uint32 crc;
	uint32 chk;
	uint8 *dst;
	const uint8 *src;	
} unpack_context_t;


extern int delphine_unpack(const uint8 *src, uint8 *dst); // a4, a5


#endif
