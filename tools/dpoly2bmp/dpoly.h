
#ifndef __DPOLY_H__
#define __DPOLY_H__

#include "endian.h"
#include "graphics.h"

#define MAX_PALETTES  256
#define MAX_SEQUENCES 512
#define MAX_SHAPES    256
#define MAX_VERTICES  256
#define DRAWING_BUFFER_W 256
#define DRAWING_BUFFER_H 224

struct DPoly {
	const char *_setFile;
	FILE *_fp;
	Graphics *_gfx;
	uint16_t _amigaPalette[16];
	uint8_t _currentPalette[16 * 3];
	int _paletteCount;
	uint16_t _palettes[MAX_PALETTES][16];
	Point _vertices[MAX_VERTICES];
	uint32_t _seqOffsets[MAX_SEQUENCES];
	uint32_t _shapesOffsets[2][MAX_SHAPES];

	void Decode(const char *setFile);
	void DecodeShape(int count, int dx, int dy);
	void DecodePalette();
	void SetPalette(const uint16_t *pal);
	int  GetShapeOffsetForSet(const char *filename);
	void ReadShapeMarker();
	void ReadPaletteMarker();
	void ReadSequenceBuffer();
	void ReadAffineBuffer();
	void WriteShapeToBitmap(int group, int shape);
	void WriteFrameToBitmap(int frame);
};

#endif // __DPOLY_H__
