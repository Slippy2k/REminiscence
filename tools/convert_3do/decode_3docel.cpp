
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "decode_3docel.h"
#include "endian.h"
#include "fileio.h"

struct ccb_t {
	uint32_t version;
	uint32_t flags;
	uint16_t ppmp0;
	uint16_t ppmp1;
	uint32_t pre0;
	uint32_t pre1;
	uint32_t width;
	uint32_t height;
};

static const uint8_t _cel_bitsPerPixelLookupTable[8] = {
        0, 1, 2, 4, 6, 8, 16, 0
};

static uint8_t _bitmapBuffer[320 * 240 * sizeof(uint16_t)];
static uint16_t _paletteBuffer[256];

struct BitStream {
	int _bits;
	int _size;

	void reset(FILE *fp) {
		_bits = fgetc(fp);
		_size = 8;
	}

	uint32_t readBits(FILE *fp, int count) {
		uint32_t value = 0;
		while (count != 0) {
			if (count < _size) {
				const uint16_t mask = (1 << count) - 1;
				value |= (_bits >> (_size - count)) & mask;
				_size -= count;
				break;
			}
			count -= _size;
			const uint16_t mask = (1 << _size) - 1;
			value |= (_bits & mask) << count;
			// refill
			_bits = fgetc(fp);
			_size = 8;
		}
		return value;
	}
};

static void decodeCel_PDAT(const struct ccb_t *ccb, FILE *fp, uint32_t size) {
	const int ccbPRE0_bitsPerPixel = _cel_bitsPerPixelLookupTable[ccb->pre0 & 7];
	const int bpp = (ccbPRE0_bitsPerPixel <= 8) ? 8 : ccbPRE0_bitsPerPixel;
	assert(ccb->width * ccb->height * bpp / 8 <= sizeof(_bitmapBuffer));
	BitStream bs;
	if (ccb->flags & 0x200) { // RLE
		for (int j = 0; j < ccb->height; ++j) {
			uint8_t *dst = _bitmapBuffer + j * ccb->width * bpp / 8;
			const int pos = ftell(fp);
			const int lineSize = (ccbPRE0_bitsPerPixel >= 8) ? freadUint16BE(fp) : fgetc(fp);
			bs.reset(fp);
			for (int w = ccb->width; w > 0; ) {
				int type = bs.readBits(fp, 2);
				int count = bs.readBits(fp, 6) + 1;
				if (type == 0) { // PACK_EOL
					break;
				}
				if (count > w) {
					count = w;
				}
				switch (type) {
				case 1: // PACK_LITERAL
					for (int i = 0; i < count; ++i) {
						if (bpp == 16) {
							const uint16_t color = bs.readBits(fp, ccbPRE0_bitsPerPixel);
							*(uint16_t *)dst = color;
							dst += 2;
						} else {
							const uint8_t color = bs.readBits(fp, ccbPRE0_bitsPerPixel);
							*dst++ = color;
						}
					}
					break;
				case 2: // PACK_TRANSPARENT
					dst += count * bpp / 8;
					break;
				case 3: // PACK_REPEAT
					if (bpp == 16) {
						const uint16_t color = bs.readBits(fp, ccbPRE0_bitsPerPixel);
						for (int i = 0; i < count; ++i) {
							*(uint16_t *)dst = color;
							dst += 2;
						}
					} else {
						const uint8_t color = bs.readBits(fp, ccbPRE0_bitsPerPixel);
						memset(dst, color, count);
						dst += count;
					}
					break;
				}
				w -= count;
			}
			const int align = (lineSize + 2) * sizeof(uint32_t);
			fseek(fp, pos + align, SEEK_SET);
		}
	} else {
		if (ccbPRE0_bitsPerPixel == 4) {
			const int lineSize = ccb->width * ccbPRE0_bitsPerPixel / 8;
			const int align = ((lineSize + 3) & ~3) - lineSize;
			for (int j = 0; j < ccb->height; ++j) {
				const int offset = j * ccb->width;
				for (int i = 0; i < ccb->width; i += 2) {
					const uint8_t color = fgetc(fp);
					_bitmapBuffer[offset + i + 1] = color & 15;
					_bitmapBuffer[offset + i + 0] = color >> 4;
				}
				fseek(fp, align, SEEK_CUR);
			}
		} else {
			fread(_bitmapBuffer, 1, ccb->height * ccb->width * bpp / 8, fp);
		}
	}
}

enum {
	kMaskCCB  = 1 << 0,
	kMaskPLUT = 1 << 1,
	kMaskPDAT = 1 << 2
};

int decode_3docel(FILE *fp, CelPicture *p) {
	int mask = kMaskCCB;
	ccb_t ccb;
	memset(&ccb, 0, sizeof(ccb));
	int plutSize;
	while (mask != 0) {
		const uint32_t pos = ftell(fp);
		char tag[5];
		const uint32_t size = freadTag(fp, tag);
		if (feof(fp)) {
			break;
		}
		tag[4] = 0;
		fprintf(stdout, "Found tag '%s' size %d\n", tag, size);
		if (memcmp(tag, "CCB ", 4) == 0) {
			ccb.version = freadUint32BE(fp);
			ccb.flags = freadUint32BE(fp);
			fseek(fp, 3 * 4, SEEK_CUR);
			uint32_t xpos = freadUint32BE(fp);
			uint32_t ypos = freadUint32BE(fp);
			uint32_t hdx = freadUint32BE(fp);
			uint32_t hdy = freadUint32BE(fp);
			uint32_t vdx = freadUint32BE(fp);
			uint32_t vdy = freadUint32BE(fp);
			uint32_t hddx = freadUint32BE(fp);
			uint32_t hddy = freadUint32BE(fp);
			fprintf(stdout, "pos 0x%x,0x%x hd 0x%x,0x%x vd 0x%x,0x%x, hdd 0x%x,0x%x\n", xpos, ypos, hdx, hdy, vdx, vdy, hddx, hddy);
			uint32_t ppmp = freadUint32BE(fp);
			ccb.ppmp0 = ppmp >> 16;
			ccb.ppmp1 = ppmp & 0xFFFF;
			ccb.pre0 = freadUint32BE(fp);
			ccb.pre1 = freadUint32BE(fp);
			ccb.width = freadUint32BE(fp);
			ccb.height = freadUint32BE(fp);
			fprintf(stdout, "version %d flags 0x%x width %d height %d\n", ccb.version, ccb.flags, ccb.width, ccb.height);

			// bit9 - 0x200 - compressed
			// bit5 - RGB(0,0,0) is actual black not transparent

			int ccbPRE0_bitsPerPixel = _cel_bitsPerPixelLookupTable[ccb.pre0 & 7];
			int ccbPRE0_height = ((ccb.pre0 >> 6) & 0x3FF) + 1;
			int ccbPRE1_width  = (ccb.pre1 & 0x3FF) + 1;

			fprintf(stdout, "ccbPRE bits %d width %d height %d rle %d\n", ccbPRE0_bitsPerPixel, ccbPRE1_width, ccbPRE0_height, (ccb.flags & 0x200) != 0);

			p->bpp = ccbPRE0_bitsPerPixel;
			p->h = ccbPRE0_height;
			p->w = ccbPRE1_width;

			mask &= ~kMaskCCB;
			mask |= kMaskPDAT;
			if (ccbPRE0_bitsPerPixel <= 8) {
				mask |= kMaskPLUT;
			}

		} else if (memcmp(tag, "PLUT", 4) == 0) {
			int count = freadUint32BE(fp);
			fprintf(stdout, "PLUT count %d\n", count);
			assert(count <= 256);
			memset(_paletteBuffer, 0, sizeof(_paletteBuffer));
			for (int i = 0; i < count; ++i) {
				_paletteBuffer[i] = freadUint16BE(fp);
			}
			plutSize = count;
			p->palSize = count;

			mask &= ~kMaskPLUT;

		} else if (memcmp(tag, "PDAT", 4) == 0) {
			decodeCel_PDAT(&ccb, fp, size);

			mask &= ~kMaskPDAT;
		} else {
			fprintf(stderr, "Unhandled tag '%c%c%c%c' size %d\n", tag[0], tag[1], tag[2], tag[3], size);
		}
		fseek(fp, pos + size, SEEK_SET);
	}
	p->data = _bitmapBuffer;
	p->pal = _paletteBuffer;
	return 0;
}
