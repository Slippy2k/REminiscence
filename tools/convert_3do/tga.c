
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "tga.h"

static void fileWriteUint16LE(FILE *fp, int n) {
	fputc(n & 255, fp);
	fputc((n >> 8) & 255, fp);
}

#define kTgaImageTypeUncompressedColorMapImage 1
#define kTgaImageTypeUncompressedTrueColorImage 2
#define kTgaDirectionRight (1 << 4)
#define kTgaDirectionTop (1 << 5)

struct TgaFile {
	FILE *fp;
	int width, height, bitdepth;
	const unsigned char *colorLookupTable;
};

static int _imageType = kTgaImageTypeUncompressedTrueColorImage;

static void tgaWriteHeader(struct TgaFile *t) {
	int colorMapType = 0;
	int colorMapLength = 0;
	int colorMapDepth = 0;
	int bitDepth = t->bitdepth;
	if (bitDepth <= 8 && _imageType == kTgaImageTypeUncompressedColorMapImage) {
		colorMapType = 1;
		colorMapLength = 256;
		colorMapDepth = 24;
		bitDepth = 8;
	}
	fputc(0, t->fp); // 0, ID Length
	fputc(colorMapType, t->fp); // 1, ColorMap Type
	fputc(_imageType, t->fp); // 2, Image Type
	fileWriteUint16LE(t->fp, 0); // 3, colourmapstart
	fileWriteUint16LE(t->fp, colorMapLength); // 5, colourmaplength
	fputc(colorMapDepth, t->fp); // 7, colourmapbits
	fileWriteUint16LE(t->fp, 0); // 8, xstart
	fileWriteUint16LE(t->fp, 0); // 10, ystart
	fileWriteUint16LE(t->fp, t->width); // 12
	fileWriteUint16LE(t->fp, t->height); // 14
	fputc(bitDepth, t->fp); // 16
	fputc(kTgaDirectionTop, t->fp); // 17, descriptor
}

struct TgaFile *tgaOpen(const char *filePath, int width, int height, int depth) {
	struct TgaFile *t;

	t = malloc(sizeof(struct TgaFile));
	assert(t);
	t->fp = fopen(filePath, "wb");
	assert(t->fp);
	t->width = width;
	t->height = height;
	t->bitdepth = depth;
	t->colorLookupTable = 0;
	if (depth == 8) {
		_imageType = kTgaImageTypeUncompressedColorMapImage;
	}
	return t;
}

void tgaClose(struct TgaFile *t) {
	if (t->fp) {
		fclose(t->fp);
		t->fp = NULL;
	}
	free(t);
}

void tgaSetLookupColorTable(struct TgaFile *t, const unsigned char *paletteData) {
	t->colorLookupTable = paletteData;
}

void tgaWritePixelsData(struct TgaFile *t, const unsigned char *pixelsData, int pixelsDataSize) {
	int i, color, rgb;

	tgaWriteHeader(t);
	if (t->colorLookupTable) {
		assert(pixelsDataSize == t->height * t->width);
		if (_imageType == kTgaImageTypeUncompressedColorMapImage) {
			for (i = 0; i < 256; ++i) {
				fputc(t->colorLookupTable[i * 3 + 2], t->fp);
				fputc(t->colorLookupTable[i * 3 + 1], t->fp);
				fputc(t->colorLookupTable[i * 3], t->fp);
			}
			fwrite(pixelsData, 1, pixelsDataSize, t->fp);
			return;
		}
		assert(_imageType == kTgaImageTypeUncompressedTrueColorImage);
		for (i = 0; i < pixelsDataSize; ++i) {
			color = pixelsData[i];
			for (rgb = 0; rgb < 3; ++rgb) {
				static const int p[] = { 2, 1, 0 };
				fputc(t->colorLookupTable[color * 3 + p[rgb]], t->fp);
			}
		}
	} else {
		assert(pixelsDataSize == t->height * t->width * t->bitdepth / 8);
		fwrite(pixelsData, 1, pixelsDataSize, t->fp);
	}
}

