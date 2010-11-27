
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "decode.h"
#include "file.h"

uint8_t *decodeLzss(File &f, uint32_t &decodedSize) {
	decodedSize = f.readUint32BE();
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

void decodeC103(const uint8_t *a3, int w, int h, DecodeBuffer *buf) {
//	uint8_t *baseA0 = a0;
	uint8_t d0;
	int d3 = 0;
	int d7 = 1;
	int d6 = 0;
	int d1 = 0;
	static const uint32_t d5 = 0xFFF;
	uint8_t a1[0x1000];

	for (int y = 0; y < h; ++y) {
//		a0 = baseA0 + y * pitch;
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
//					*a0++ = d0;
					buf->setPixel(x, y, w, h, d0);
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
//			*a0++ = d0;
			buf->setPixel(x, y, w, h, d0);
			--d6;
		}
	}
}

void decodeC211(const uint8_t *a3, int w, int h, DecodeBuffer *buf) {
	struct {
		const uint8_t *ptr;
		int repeatCount;
	} stack[512];
//	uint8_t *baseA0 = a0;
	int y = 0;
	int x = 0;
	int sp = 0;

	while (1) {
		uint8_t d0 = *a3++;
		if ((d0 & 0x80) != 0) {
			++y;
			x = 0;
//			a0 = baseA0 + y * pitch;
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
					assert(sp > 0);
					--stack[sp - 1].repeatCount;
					if (stack[sp - 1].repeatCount >= 0) {
						a3 = stack[sp - 1].ptr;
					} else {
						--sp;
					}
				} else {
					assert(sp < sizeof(stack) / sizeof(stack[0]));
					stack[sp].ptr = a3;
					stack[sp].repeatCount = d1;
					++sp;
				}
			} else {
//				if (xflip) {
//					a0 -= d1;
//				} else {
//					a0 += d1;
//				}
				x += d1;
			}
		} else {
			if ((d0 & 0x80) == 0) {
				if (d1 == 1) {
					return;
				}
//				if (xflip) {
//					const uint8_t code = *a3++;
//					for (int i = 0; i < d1; ++i) {
//						*a0-- = code;
//					}
//				} else {
//					memset(a0, *a3++, d1);
//					a0 += d1;
//				}
				const uint8_t color = *a3++;
				for (int i = 0; i < d1; ++i) {
					buf->setPixel(x++, y, w, h, color);
				}
			} else {
//				if (xflip) {
//					for (int i = 0; i < d1; ++i) {
//						*a0-- = *a3++;
//					}
//				} else {
//					memcpy(a0, a3, d1);
//					a0 += d1;
//					a3 += d1;
//				}
				for (int i = 0; i < d1; ++i) {
					buf->setPixel(x++, y, w, h, *a3++);
				}
			}
		}
	}
}

