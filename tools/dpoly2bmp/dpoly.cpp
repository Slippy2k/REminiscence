
#include "bitmap.h"
#include "dpoly.h"
#include "file.h"
#include "graphics.h"


void DPoly::Decode(const char *setFile) {
	m_fp = fopen(setFile, "rb");
	if (!m_fp) {
		return;
	}
	m_setFile = setFile;
	m_gfx = new Graphics;
	m_gfx->_layer = (uint8_t *)malloc(DRAWING_BUFFER_W * DRAWING_BUFFER_H);
	m_currentShape = 0;
	uint8_t hdr[8];
	int ret = fread(hdr, 8, 1, m_fp);
	if (ret != 1) {
		fprintf(stderr, "fread() failed, ret %d", ret);
	}
	assert(memcmp(hdr, "POLY\x00\x0A\x00\x02", 8) == 0) ;
	ReadSequenceBuffer();
	// TODO:
	const int offset = GetShapeOffsetForSet(setFile);
	fseek(m_fp, offset, SEEK_SET);
	for (int counter = 0; ; ++counter) {
		int pos = ftell(m_fp);
		m_numShapes = freadUint16BE(m_fp);
		if (feof(m_fp)) {
			break;
		}
		printf("counter %d numShapes %d pos 0x%X\n", counter, m_numShapes, pos);
		memset(m_gfx->_layer, 0, DRAWING_BUFFER_W * DRAWING_BUFFER_H);
		m_numShapes--;
		assert(m_numShapes >= 0);
		for (int j = 0; j < m_numShapes; ++j) {
			ReadShapeMarker();
			int numVertices = freadByte(m_fp);
			if (numVertices == 255) numVertices = 1;
			int ix = (int16_t)freadUint16BE(m_fp);
			int iy = (int16_t)freadUint16BE(m_fp);
			int color1 = freadByte(m_fp);
			int color2 = freadByte(m_fp);
			printf(" shape %d/%d x=%d y=%d color1=%d color2=%d\n", j, m_numShapes, ix, iy, color1, color2);
			assert(numVertices < MAX_VERTICES);
			for (int i = 0; i < numVertices; ++i) {
				m_vertices[i].x = (int16_t)freadUint16BE(m_fp);
				m_vertices[i].y = (int16_t)freadUint16BE(m_fp);
				printf("  vertex %d/%d x=%d y=%d\n", i, numVertices, m_vertices[i].x, m_vertices[i].y);
			}
			m_gfx->setClippingRect(8, 50, 240, 128);
			m_gfx->drawPolygon(color1, false, m_vertices, numVertices);
		}
		ReadPaletteMarker();
		for (int i = 0; i < 16; ++i) {
			m_amigaPalette[i] = freadUint16BE(m_fp);
		}
		SetPalette(m_amigaPalette);
		freadByte(m_fp); /* 0x00 */
		WriteShapeToBitmap();
	}
}

void DPoly::SetPalette(const uint16_t *pal) {
	memset(m_palette, 0, sizeof(m_palette));
	for (int i = 0; i < 16; ++i) {
		m_palette[i * 3 + 0] = (pal[i] >> 8) & 0xF;
		m_palette[i * 3 + 1] = (pal[i] >> 4) & 0xF;
		m_palette[i * 3 + 2] = (pal[i] >> 0) & 0xF;
	}
}

int DPoly::GetShapeOffsetForSet(const char *filename) {
	if (strcasecmp(filename, "CAILLOU-F.SET") == 0) {
		return 0x5E6;
	}
	if (strcasecmp(filename, "MEMOTECH.SET") == 0) {
		return 0x530A;
	}
	if (strcasecmp(filename, "MEMOSTORY0.SET") == 0) {
		return 0x859D;
	}
	if (strcasecmp(filename, "MEMOSTORY1.SET") == 0) {
		return 0x9596;
	}
	if (strcasecmp(filename, "MEMOSTORY2.SET") == 0) {
		return 0x54BD;
	}
	if (strcasecmp(filename, "MEMOSTORY3.SET") == 0) {
		return 0x3480;
	}
	if (strcasecmp(filename, "JUPITERStation1.set") == 0) {
		return 0x822C;
	}
	if (strcasecmp(filename, "TAKEMecha1-F.set") == 0) {
		return 0x822C;
	}
	return 0;
}

/* 0x00 0x00 0x00 0x00 0x01 */
void DPoly::ReadShapeMarker() {
	int mark0, mark1;
	
	/* HACK */
	mark0 = freadUint16BE(m_fp);
	if (mark0 != 0) {
		m_numShapes = mark0;
		mark0 = freadUint16BE(m_fp);
		--m_numShapes;
		printf("numShapes %d\n", m_numShapes);
	}
	mark0 &= freadUint16BE(m_fp);
	mark1 = freadByte(m_fp);
	printf("shape - pos 0x%X mark0 %d mark1 %d\n", (int)ftell(m_fp), mark0, mark1);
	assert(mark0 == 0 && (mark1 == 1 || mark1 == 0)); /* CAILLOU-F.SET 0x1679 */
}

/* 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x01 0x01 */
void DPoly::ReadPaletteMarker() {
	int mark0, mark1;
	
	mark0 = freadUint16BE(m_fp);
	mark0 &= freadUint16BE(m_fp);
	mark0 &= freadUint16BE(m_fp);
	mark0 &= freadUint16BE(m_fp);
	mark0 &= freadUint16BE(m_fp);
	mark1 = freadByte(m_fp);
	mark1 &= freadByte(m_fp);
	printf("palette - pos 0x%X mark0 %d mark1 %d\n", (int)ftell(m_fp), mark0, mark1);
	assert(mark0 == 0 && (mark1 == 1 || mark1 == 2 || mark1 == 8 || mark1 == 9 || mark1 == 12));
}

/* 0x00 0x00 0x00 0xB4 0x00 0x5A */

void DPoly::ReadSequenceBuffer() {
	int i, mark, count;
	unsigned char buf[6];

	mark = freadUint16BE(m_fp);
	assert(mark == 0xFFFF);
	count = freadUint16BE(m_fp);
	printf("count %d\n", count);
	while (1) {
		mark = freadUint16BE(m_fp);
		assert(mark == 0);
		count = freadUint16BE(m_fp);
		printf("sequence - mark %d count %d pos 0x%x\n", mark, count, (int)ftell(m_fp));
		if (count == 0) {
			break;
		}
		for (i = 0; i < count; ++i) {
			const int ret = fread(buf, 1, sizeof(buf), m_fp);
			if (ret != sizeof(buf)) {
				fprintf(stderr, "fread() failed, ret %d\n", ret);
			}
			printf("  frame=%d .x=%d .y=%d\n", READ_BE_UINT16(buf), (int16_t)READ_BE_UINT16(buf + 2), (int16_t)READ_BE_UINT16(buf + 4));
		}
	}
}

void DPoly::WriteShapeToBitmap() {
	char filePath[1024];
	strcpy(filePath, m_setFile);
	char *p = strrchr(filePath, '.');
	if (!p) {
		p = filePath + strlen(filePath);
	}
	sprintf(p, "-SHAPE-%03d.BMP", m_currentShape);
	WriteBitmapFile(filePath, DRAWING_BUFFER_W, DRAWING_BUFFER_H, m_gfx->_layer, m_palette);
	++m_currentShape;
}
