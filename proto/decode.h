
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

uint8_t *decodeLzss(File &f, uint32_t &decodedSize);

struct DecodeBuffer {
	uint8_t *ptr;
	int w, h, pitch;
	int x, y;
	bool xflip, yflip;

	void *lut;
	void (*setPixel)(DecodeBuffer *buf, int src_x, int src_y, int src_w, int src_h, uint8_t color);
};

void decodeC103(const uint8_t *a3, int w, int h, DecodeBuffer *buf);
void decodeC211(const uint8_t *a3, int w, int h, DecodeBuffer *buf);

#endif
