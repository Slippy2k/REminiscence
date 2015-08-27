
#ifndef __DPOLY_H__
#define __DPOLY_H__

#include "endian.h"
#include "graphics.h"

#define MAX_VERTICES 256
#define DRAWING_BUFFER_W 256
#define DRAWING_BUFFER_H 224

struct DPoly {
	const char *m_setFile;
	FILE *m_fp;
	Graphics *m_gfx;
	uint16_t m_amigaPalette[16];
	uint8_t m_palette[256 * 3];
	Point m_vertices[MAX_VERTICES];
	int m_currentShape;
	int m_numShapes;

	void Decode(const char *setFile);
	void ReadFrame();
	void SetPalette(const uint16_t *pal);
	int  GetShapeOffsetForSet(const char *filename);
	void ReadShapeMarker();
	void ReadPaletteMarker();
	void ReadSequenceBuffer();
	void ReadAffineBuffer();
	void WriteShapeToBitmap();
};

#endif // __DPOLY_H__
