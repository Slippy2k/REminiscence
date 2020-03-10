
#ifndef __DPOLY_H__
#define __DPOLY_H__

#include "endian.h"
#include "graphics.h"

#define MAX_SEQUENCES 512
#define MAX_SHAPES    256
#define MAX_VERTICES  256
#define DRAWING_BUFFER_W 256
#define DRAWING_BUFFER_H 256
#define GFX_CLIP_X ((DRAWING_BUFFER_W - GFX_CLIP_W) / 2)
#define GFX_CLIP_Y ((DRAWING_BUFFER_H - GFX_CLIP_H) / 2)
#define GFX_CLIP_W 240
#define GFX_CLIP_H 128
#define MAX_ROTATIONS 2048

struct DPoly {
	const char *_setFile;
	FILE *_fp;
	Graphics _gfx;
	uint16_t _amigaPalette[16];
	uint8_t _currentPalette[16 * 3];
	uint32_t _seqOffsets[MAX_SEQUENCES];
	uint32_t _shapesOffsets[2][MAX_SHAPES]; // background, foreground
	uint32_t _rgb[DRAWING_BUFFER_W * DRAWING_BUFFER_H];
	uint16_t _rotPt[MAX_SHAPES][2];
	int _points;
	uint32_t _rotMat[MAX_ROTATIONS][3];
	int _rotations;
	int _currentShapeRot;
	uint8_t _unkData[MAX_SEQUENCES * 4];

	void Decode(const char *setFile);
	void DecodeShape(int count, int dx, int dy, int shape = -1);
	void DecodePalette();
	void SetPalette(const uint16_t *pal);
	void ReadShapeMarker();
	int  ReadPaletteMarker();
	int  ReadSequenceBuffer();
	void ReadAffineBuffer(int rotations, int unk);
	void DumpPalette();
	void WriteShapeToBitmap(int group, int shape);
	void WriteFrameToBitmap(int frame);
	void DoFrameLUT();
};

#endif // __DPOLY_H__
