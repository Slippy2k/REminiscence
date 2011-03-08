
#include <stdio.h>
#include <assert.h>
#include "util.h"

static unsigned char iconData16[64 * 16];

static void decode_icon_spr(int w, int h, unsigned char *data) {
	int y, x, i, bit, color, mask;
	const int planarSize = w * 2 * h;

	assert(w == 1);
	for (y = 0; y < h; ++y) {
			for (i = 0; i < 16; ++i) {
				color = 0;
				mask = 1 << (15 - i);
				for (bit = 0; bit < 4; ++bit) {
					if (movew(data + bit * planarSize) & mask) {
						color |= 1 << bit;
					}
				}
				assert(color < 16);
				iconData16[y * 16 + i] = color;
			}
			data += 2;
	}
}

void decode_spr(unsigned char *data, int data_size) {
	unsigned char pal[256 * 3], name[16];
	int w, h, count, i;

	for (i = 0; i < 256; ++i) {
		pal[3 * i] = pal[3 * i + 1] = pal[3 * i + 2] = 16 * i;
	}
	i = 0;
	while (data_size > 0) {
		h = *data++;
		++h;
		w = *data++;
		++w;
		count = w * h * 8 + 4;
		printf("icon %dx%d count %d (%d,%d)\n", w, h, count, data[0], data[1]);
		decode_icon_spr(w, h, data + 4);
		snprintf(name, sizeof(name), "DUMP/spr%02d.png", i);
		write_png_image_data(name, iconData16, pal, 16, h);
		data += count;
		data_size -= count + 2;
		++i;
	}
	assert(data_size == 0);
}

