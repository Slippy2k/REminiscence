
#ifndef __DPOLY_H__
#define __DPOLY_H__

#include "endian.h"
#include "graphics.h"

#define MAX_VERTICES 256
#define DRAWING_BUFFER_W 256
#define DRAWING_BUFFER_H 224

struct DPoly {
	const char *_setFile;
	FILE *_fp;
	Graphics *_gfx;
	uint16_t _amigaPalette[16];
	uint8_t _palette[256 * 3];
	Point _vertices[MAX_VERTICES];
	int _currentShape;
	int _numShapes;

	void Decode(const char *setFile);
	void ReadFrame();
	void SetPalette(const uint16_t *pal);
	int  GetShapeOffsetForSet(const char *filename);
	void ReadShapeMarker();
	void ReadPaletteMarker();
	void ReadSequenceBuffer();
	void ReadAffineBuffer();
	void WriteShapeToBitmap(int group, int shape);
};

#endif // __DPOLY_H__
