
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2018 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include "unpack.h"

struct UnpackCtx {
	int datasize;
	uint32_t crc;
	uint32_t bits;
	uint8_t *dst;
	const uint8_t *src;
};

static int shiftBit(UnpackCtx *uc, uint32_t CF) {
	const int rCF = (uc->bits & 1);
	uc->bits = (uc->bits >> 1) | (CF << 31);
	return rCF;
}

static int nextBit(UnpackCtx *uc) {
	int CF = shiftBit(uc, 0);
	if (uc->bits == 0) {
		uc->bits = READ_BE_UINT32(uc->src); uc->src -= 4;
		uc->crc ^= uc->bits;
		CF = shiftBit(uc, 1);
	}
	return CF;
}

static uint16_t getBits(UnpackCtx *uc, int num_bits) {
	uint16_t c = 0;
	for (int i = 0; i < num_bits; ++i) {
		c <<= 1;
		if (nextBit(uc)) {
			c |= 1;
		}
	}
	return c;
}

static void unpackHelper1(UnpackCtx *uc, int num_bits, int add_count) {
	const int count = getBits(uc, num_bits) + add_count + 1;
	for (int i = 0; i < count; ++i) {
		*uc->dst = (uint8_t)getBits(uc, 8);
		--uc->dst;
	}
	uc->datasize -= count;
}

static void unpackHelper2(UnpackCtx *uc, int num_bits, int count) {
	const uint16_t offset = getBits(uc, num_bits);
	++count;
	for (int i = 0; i < count; ++i) {
		*uc->dst = *(uc->dst + offset);
		--uc->dst;
	}
	uc->datasize -= count;
}

bool delphine_unpack(uint8_t *dst, const uint8_t *src, int len) {
	UnpackCtx uc;
	uc.src = src + len - 4;
	uc.datasize = READ_BE_UINT32(uc.src); uc.src -= 4;
	uc.dst = dst + uc.datasize - 1;
	uc.crc = READ_BE_UINT32(uc.src); uc.src -= 4;
	uc.bits = READ_BE_UINT32(uc.src); uc.src -= 4;
	uc.crc ^= uc.bits;
	int count = 0;
	do {
		if (!nextBit(&uc)) {
			count = 1;
			if (!nextBit(&uc)) {
				unpackHelper1(&uc, 3, 0);
			} else {
				unpackHelper2(&uc, 8, count);
			}
		} else {
			const int code = getBits(&uc, 2);
			switch (code) {
			case 3:
				unpackHelper1(&uc, 8, 8);
				break;
			case 2:
				count = getBits(&uc, 8);
				unpackHelper2(&uc, 12, count);
				break;
			default:
				count = code + 2;
				unpackHelper2(&uc, code + 9, count);
				break;
			}
		}
	} while (uc.datasize > 0);
	return uc.crc == 0;
}
