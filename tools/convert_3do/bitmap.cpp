
#include <assert.h>
#include <stdio.h>
#include "bitmap.h"
#include "fileio.h"

static void TO_LE16(uint8_t *dst, uint16_t value) {
	for (int i = 0; i < 2; ++i) {
		dst[i] = value & 255;
		value >>= 8;
	}
}

#define kTgaImageTypeUncompressedTrueColor 2
#define kTgaImageTypeRunLengthEncodedTrueColor 10
#define kTgaDirectionTop (1 << 5)

static const int TGA_HEADER_SIZE = 18;

void saveTGA(const char *filename, const uint8_t *rgba, int w, int h) {

	static const uint8_t kImageType = kTgaImageTypeRunLengthEncodedTrueColor;
	uint8_t buffer[TGA_HEADER_SIZE];
	buffer[0]            = 0; // ID Length
	buffer[1]            = 0; // ColorMap Type
	buffer[2]            = kImageType;
	TO_LE16(buffer +  3,   0); // ColorMap Start
	TO_LE16(buffer +  5,   0); // ColorMap Length
	buffer[7]            = 0;  // ColorMap Bits
	TO_LE16(buffer +  8,   0); // X-origin
	TO_LE16(buffer + 10,   0); // Y-origin
	TO_LE16(buffer + 12,   w); // Image Width
	TO_LE16(buffer + 14,   h); // Image Height
	buffer[16]           = 24; // Pixel Depth
	buffer[17]           = kTgaDirectionTop;  // Descriptor

	FILE *fp = fopen(filename, "wb");
	if (fp) {
		fwrite(buffer, sizeof(buffer), 1, fp);
		if (kImageType == kTgaImageTypeUncompressedTrueColor) {
			for (int i = 0; i < w * h; ++i) {
				fputc(rgba[0], fp);
				fputc(rgba[1], fp);
				fputc(rgba[2], fp);
				rgba += 4;
			}
		} else {
			assert(kImageType == kTgaImageTypeRunLengthEncodedTrueColor);
			int prevColor = rgba[2] + (rgba[1] << 8) + (rgba[0] << 16); rgba += 4;
			int count = 0;
			for (int i = 1; i < w * h; ++i) {
				int color = rgba[2] + (rgba[1] << 8) + (rgba[0] << 16); rgba += 4;
				if (prevColor == color && count < 127) {
					++count;
					continue;
				}
				fputc(count | 0x80, fp);
				fputc((prevColor >> 16) & 255, fp);
				fputc((prevColor >>  8) & 255, fp);
				fputc( prevColor        & 255, fp);
				count = 0;
				prevColor = color;
			}
			if (count != 0) {
				fputc(count | 0x80, fp);
				fputc((prevColor >> 16) & 255, fp);
				fputc((prevColor >>  8) & 255, fp);
				fputc( prevColor        & 255, fp);
			}
		}

		fclose(fp);
	}
}

static const uint16_t TAG_BM = 0x4D42;

void saveBMP(const char *filename, const uint8_t *bits, const uint8_t *pal, int w, int h) {
	FILE *fp = fopen(filename, "wb");
	if (fp) {
		const int alignWidth = (w + 3) & ~3;
		const int imageSize = alignWidth * h;

		// Write file header
		fwriteUint16LE(fp, TAG_BM);
		fwriteUint32LE(fp, 14 + 40 + 4 * 256 + imageSize);
		fwriteUint16LE(fp, 0); // reserved1
		fwriteUint16LE(fp, 0); // reserved2
		fwriteUint32LE(fp, 14 + 40 + 4 * 256);

		// Write info header
		fwriteUint32LE(fp, 40);
		fwriteUint32LE(fp, w);
		fwriteUint32LE(fp, h);
		fwriteUint16LE(fp, 1); // planes
		fwriteUint16LE(fp, 8); // bit_count
		fwriteUint32LE(fp, 0); // compression
		fwriteUint32LE(fp, imageSize); // size_image
		fwriteUint32LE(fp, 0); // x_pels_per_meter
		fwriteUint32LE(fp, 0); // y_pels_per_meter
		fwriteUint32LE(fp, 0); // num_colors_used
		fwriteUint32LE(fp, 0); // num_colors_important

		// Write palette data
		for (int i = 0; i < 256; ++i) {
			fputc(pal[2], fp);
			fputc(pal[1], fp);
			fputc(pal[0], fp);
			fputc(0, fp);
			pal += 3;
		}

		// Write bitmap data
		const int pitch = w;
		bits += h * pitch;
		for (int i = 0; i < h; ++i) {
			bits -= pitch;
			fwrite(bits, w, 1, fp);
			int pad = alignWidth - w;
			while (pad--) {
				fputc(0, fp);
			}
		}

		fclose(fp);
	}
}
