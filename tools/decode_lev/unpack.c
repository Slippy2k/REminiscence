
#include "unpack.h"

static uint32 READ_BE_UINT32(const unsigned char *src) {
	uint32 num = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
	return num;
}

static int rcr(unpack_context_t *uc, int CF) {
	int rCF = (uc->chk & 1);
	uc->chk >>= 1;
	if (CF) {
		uc->chk |= 0x80000000;
	}
	return rCF;
}

static int next_chunk(unpack_context_t *uc) {
	int CF = rcr(uc, 0);
	if (uc->chk == 0) {
		uc->chk = READ_BE_UINT32(uc->src); uc->src -= 4;
		uc->crc ^= uc->chk;
		CF = rcr(uc, 1);
	}
	return CF;
}

static uint16 get_code(unpack_context_t *uc, uint8 num_chunks) {
	uint16 c = 0;
	while (num_chunks--) {
		c <<= 1;
		if (next_chunk(uc)) {
			c |= 1;
		}			
	}
	return c;
}

static void dec_unk1(unpack_context_t *uc, uint8 num_chunks, uint8 add_count) {
	uint16 count = get_code(uc, num_chunks) + add_count + 1;
	uc->datasize -= count;
	while (count--) {
		*uc->dst = (uint8)get_code(uc, 8);
		--uc->dst;
	}
}

static void dec_unk2(unpack_context_t *uc, uint8 num_chunks) {
	uint16 i = get_code(uc, num_chunks);
	uint16 count = uc->size + 1;
	uc->datasize -= count;
	while (count--) {
		*uc->dst = *(uc->dst + i);
		--uc->dst;
	}
}

int delphine_unpack(const uint8 *src, uint8 *dst) {
	unpack_context_t uc;
	uc.src = src;
	uc.datasize = READ_BE_UINT32(uc.src); uc.src -= 4;
//printf("datasize %d\n", uc.datasize);
	uc.dst = dst + uc.datasize - 1;
	uc.size = 0;
	uc.crc = READ_BE_UINT32(uc.src); uc.src -= 4;
	uc.chk = READ_BE_UINT32(uc.src); uc.src -= 4;
	uc.crc ^= uc.chk;
	do {
		if (!next_chunk(&uc)) {
			uc.size = 1;
			if (!next_chunk(&uc)) {
				dec_unk1(&uc, 3, 0);
			} else {
				dec_unk2(&uc, 8);
			}
		} else {
			uint16 c = get_code(&uc, 2);
			if (c == 3) {
				dec_unk1(&uc, 8, 8);
			} else if (c < 2) {
				uc.size = c + 2;
				dec_unk2(&uc, c + 9);
			} else {
				uc.size = get_code(&uc, 8);
				dec_unk2(&uc, 12);
			}
		}
	} while (uc.datasize > 0);
	return uc.crc == 0;
}
