
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"

uint8_t *decodeLzss(File &f) {
	const uint32_t decodedSize = f.readUint32BE();
	printf("decodeLzss decodedSize %d\n", decodedSize);
	uint8_t *dst = (uint8_t *)malloc(decodedSize);
	int count = 0;
	while (count < decodedSize) {
		const int code = f.readByte();
		for (int i = 0; i < 8 && count < decodedSize; ++i) {
			if ((code & (1 << i)) == 0) {
				dst[count++] = f.readByte();
			} else {
				int offset = f.readUint16BE();
				const int len = (offset >> 12) + 3;
				offset &= 0xFFF;
				for (int j = 0; j < len; ++j) {
					dst[count + j] = dst[count - offset - 1 + j];
				}
				count += len;
			}
		}
	}
	assert(count == decodedSize);
	return dst;
}

void decodeC103(const uint8_t *a3, uint8_t *a0, int w, int h) {
	uint8_t *baseA0 = a0;
	uint8_t d0;
	int d3 = 0;
	int d7 = 1;
	int d6 = 0;
	int d1 = 0;
	static const uint32_t d5 = 0xFFF;
	uint8_t a1[0x1000];

	for (int y = 0; y < h; ++y) {
		a0 = baseA0 + y * w;
		for (int x = 0; x < w; ++x) {
			assert(d6 >= 0);
			if (d6 == 0) {
				int carry = d7 & 1;
				d7 >>= 1;
				if (d7 == 0) {
					d7 = *a3++;
					const int extended_bit = carry ? 0x80 : 0;
					carry = d7 & 1;
					d7 = extended_bit | (d7 >> 1); // should be roxr (extended bit in 0x80)
				}
				if (!carry) {
					d0 = *a3++;
					a1[d3] = d0;
					++d3;
					d3 &= d5;
					*a0++ = d0;
					continue;
				}
				d1 = a3[0] * 256 + a3[1];
				a3 += 2;
				d6 = d1;
				d1 &= d5;
				++d1;
				d1 = (d3 - d1) & d5;
				d6 >>= 12;
				d6 += 3;
			}
			d0 = a1[d1++];
			d1 &= d5;
			a1[d3++] = d0;
			d3 &= d5;
			*a0++ = d0;
			--d6;
		}
	}
}

void decodeC211(const uint8_t *a3, uint8_t *a0, int pitch, int h) {
	struct {
		const uint8_t *ptr;
		int repeatCount;
	} stack[512];
	uint8_t d0;
	uint8_t *baseA0 = a0;
	int Y = 0;
	int SP = 0;

	while (1) {
		d0 = *a3++;
		if ((d0 & 0x80) != 0) {
			++Y;
			a0 = baseA0 + Y * pitch;
		}
		int d1 = d0 & 0x1F;
		if (d1 == 0) {
			d1 = a3[0] * 256 + a3[1];
			a3 += 2;
		}
		const int carry_set = (d0 & 0x40) != 0;
		d0 <<= 2;
		if (!carry_set) {
			if ((d0 & 0x80) == 0) {
				--d1;
				if (d1 == 0) {
					assert(SP > 0);
					--stack[SP - 1].repeatCount;
					if (stack[SP - 1].repeatCount >= 0) {
						a3 = stack[SP - 1].ptr;
					} else {
						--SP;
					}
				} else {
					assert(SP < sizeof(stack) / sizeof(stack[0]));
					stack[SP].ptr = a3;
					stack[SP].repeatCount = d1;
					++SP;
				}
			} else {
				a0 += d1;
			}
		} else {
			if ((d0 & 0x80) == 0) {
				if (d1 == 1) {
					return;
				}
				memset(a0, *a3++, d1);
			} else {
				memcpy(a0, a3, d1);
				a3 += d1;
			}
			a0 += d1;
		}
	}
}

