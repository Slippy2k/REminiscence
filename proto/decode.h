
#ifndef DECODE_H__
#define DECODE_H__

#include <stdint.h>
#include "file.h"

static inline uint16_t READ_BE_UINT16(const uint8_t *ptr) {
	return (ptr[0] << 8) | ptr[1];
}

static inline uint32_t READ_BE_UINT32(const uint8_t *ptr) {
	return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
}

uint8_t *decodeLzss(File &f);
void decodeC103(const uint8_t *a3, uint8_t *a0, int w, int h);
void decodeC211(const uint8_t *a3, uint8_t *a0, int w, int h);

#endif
