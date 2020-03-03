
#ifndef __DPOLY_H__
#define __DPOLY_H__

#include "endian.h"
#include "graphics.h"

#define MAX_SEQUENCES 512
#define MAX_SHAPES    256
#define MAX_VERTICES  256
#define DRAWING_BUFFER_W 256
#define DRAWING_BUFFER_H 224
#define GFX_CLIP_X   8
#define GFX_CLIP_Y  50
#define GFX_CLIP_W 240
#define GFX_CLIP_H 128

struct DPoly {
	const char *_setFile;
	FILE *_fp;
	Graphics _gfx;
	uint16_t _amigaPalette[16];
	uint8_t _currentPalette[16 * 3];
	uint32_t _seqOffsets[MAX_SEQUENCES];
	uint32_t _shapesOffsets[2][MAX_SHAPES]; // background, foreground
	uint32_t _rgb[DRAWING_BUFFER_W * DRAWING_BUFFER_H];

	void Decode(const char *setFile);
	void DecodeShape(int count, int dx, int dy);
	void DecodePalette();
	void SetPalette(const uint16_t *pal);
	int  GetShapeOffsetForSet(const char *filename);
	void ReadShapeMarker();
	void ReadPaletteMarker();
	void ReadSequenceBuffer();
	void ReadAffineBuffer(int rotations, int unk);
	void WriteShapeToBitmap(int group, int shape);
	void WriteFrameToBitmap(int frame);
	void DoFrameLUT();
};

#endif // __DPOLY_H__
