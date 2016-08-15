
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "unpack.h"
#include "util.h"

#define DECODE_BUFSIZE (65536 * 2)

static unsigned char buf[2][DECODE_BUFSIZE];

static void cutscene_decode_cmp(const unsigned char *d4) {
#if 0
	int d0;
	const unsigned char *a0, *d7;

	a0 = d4;
	d0 = movel(a0); a0 += 4;
	a0 += d0;
	d7 = a0 - 4;
	d0 = movel(a0); a0 += 4;
	if (d0 < 0) {
		d7 += 6 - d0;
	} else {
		a0 += d0;
		d7 = a0 - 4;
		d7 += 2;
	}

	a0 = d4;
	d0 = movel(a0); a0 += 4;
	a1 = a0 + d0;
	a4 = a1;
	d0 = movel(a4 - 4);
	a1 = a5 + d0;
	delphine_unpack // _cutsceneBufferPOL = a5;

	a1 = d0;
	d0 = (a1 + 1) & ~1;
	a5 = d0;
	d0 = movel(a0); a0 += 4;
	if (d0 < 0) {
		d0 = -d0 - 1;
		_cutsceneBufferCMD = a5;
	} else {
		a1 = a0 + d0;
		a4 = a1;
		delphine_unpack // _cutsceneBufferCMD = a5
	}
#endif
	const unsigned char *p = d4;
	int i, ret, packed_size, size;

	for (i = 0; i < 2; ++i) {
		packed_size = movel(p); p += 4;
		if (packed_size < 0) {
			size = packed_size = -packed_size;
			assert(size < DECODE_BUFSIZE);
			memcpy(buf[i], p, packed_size);
			ret = 1;
		} else {
			size = movel(p + packed_size - 4);
			assert(size < DECODE_BUFSIZE);
			assert((packed_size & 1) == 0);
			ret = delphine_unpack(p + packed_size - 4, buf[i]);
		}
		dump_file(i, buf[i], size);
		p += packed_size;
		printf("cmp index %d packed_size %4d size %4d ret %d\n", i, packed_size, size, ret);
	}
}

static void blit_5p(unsigned char *dst, int w, int h, unsigned char *src, int plane_size) {
	int x, y, i, j, c;

	printf("blit_5p plane_size %d w %d h %d\n", plane_size, w, h);
	for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
			for (i = 0; i < 8; ++i) {
				const int c_mask = 1 << (7 - i);
				c = 0;
				for (j = 0; j < 5; ++j) {
					if (src[j * plane_size] & c_mask) {
						c |= 1 << j;
					}
				}
				dst[320 * y + 8 * x + i] = c;
			}
			++src;
		}
	}
}

static const int word_12DBC[] = { 0, 0x123, 0x12, 0x134, 0x433, 0x453, 0x46, 0x245, 0x751, 0x455, 0x665, 0x268, 0x961, 0x478, 0x677, 0x786 };
static const int word_12DDC[] = { 0x17B, 0x788, 0xB84, 0xC92, 0x49C, 0xF00, 0x9A8, 0x9AA, 0xCA7, 0xEA3, 0x8BD, 0xBBB, 0xEC7, 0xBCD, 0xDDB, 0xEED };

static int convert_amiga_color(unsigned char *p, int color) {
	int i, c;

	for (i = 2; i >= 0; --i) {
		c = color & 15; color >>= 4;
		p[i] = (c << 4) | c;
	}
}

static void image_decode_cmp(unsigned char *p) {
	int packed_size, size, ret, i;
	unsigned char pal[256 * 3];

	packed_size = movel(p); p += 4;
	size = movel(p + packed_size - 4);
	ret = delphine_unpack(p + packed_size - 4, buf[0]);
	assert(ret);
	printf("size %d\n", size);
	blit_5p(buf[1], 160 / 4, 224, buf[0] + 6, 160 / 4 * 224);
	for (i = 0; i < 32; ++i) {
		convert_amiga_color(pal + 3 * i, i < 16 ? word_12DBC[i] : word_12DDC[i - 16]);
	}
	write_png_image_data("present.png", buf[1], pal, 320, 224);
}

void decode_cmp(const char *filepath) {
	if (strstr(filepath, "present.cmp")) {
		image_decode_cmp(load_file(filepath));
		return;
	}
	cutscene_decode_cmp(load_file(filepath));
}
