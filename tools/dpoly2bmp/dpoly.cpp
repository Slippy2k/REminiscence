
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
	m_gfx->_layer = (uint8 *)malloc(DRAWING_BUFFER_W * DRAWING_BUFFER_H);
	m_currentAnimFrame = 0;
	uint8 hdr[8];
	fread(hdr, 8, 1, m_fp);
	assert(memcmp(hdr, "POLY\x00\x0A\x00\x02", 8) == 0) ;
	ReadVerticesBuffer();
	const int offset = GetStartingOffsetForSet(setFile);
	fseek(m_fp, offset, SEEK_SET);
	while (1) {
		int pos = ftell(m_fp);
		m_numShapes = freadUint16BE(m_fp);
		if (feof(m_fp)) {
			break;
		}
		printf("numShapes %d pos 0x%X\n", m_numShapes, pos);
		memset(m_gfx->_layer, 0, DRAWING_BUFFER_W * DRAWING_BUFFER_H);
		m_numShapes--;
		assert(m_numShapes >= 0);
		for (int j = 0; j < m_numShapes; ++j) {
			ReadShapeMarker();
			int numVertices = freadByte(m_fp);
			if (numVertices == 255) numVertices = 1;
			int ix = (int16)freadUint16BE(m_fp);
			int iy = (int16)freadUint16BE(m_fp);
			int unk1 = freadByte(m_fp); /* color1 */
			int unk2 = freadByte(m_fp); /* color2 */ /* transparent ? */
			printf("shape %d/%d x=%d y=%d color=%d\n", j, m_numShapes, ix, iy, unk1);
			assert(numVertices < MAX_VERTICES);
			for (int i = 0; i < numVertices; ++i) {
				m_vertices[i].x = (int16)freadUint16BE(m_fp);
				m_vertices[i].y = (int16)freadUint16BE(m_fp);
				printf("  vertex %d/%d x=%d y=%d\n", i, numVertices, m_vertices[i].x, m_vertices[i].y);
			}
			m_gfx->setClippingRect(8, 50, 240, 128);
			m_gfx->drawPolygon(unk1, false, m_vertices, numVertices);
		}
		ReadPaletteMarker();
		for (int i = 0; i < 16; ++i) {
			m_amigaPalette[i] = freadUint16BE(m_fp);
		}
		SetPalette(m_amigaPalette);
		freadByte(m_fp); /* 0x00 */
		WriteAnimFrameToBitmap();
	}
}

void DPoly::SetPalette(const uint16 *pal) {
	memset(m_palette, 0, sizeof(m_palette));
	for (int i = 0; i < 16; ++i) {
		m_palette[i * 3 + 0] = (pal[i] >> 8) & 0xF;
		m_palette[i * 3 + 1] = (pal[i] >> 4) & 0xF;
		m_palette[i * 3 + 2] = (pal[i] >> 0) & 0xF;
	}
}

int DPoly::GetStartingOffsetForSet(const char *filename) {
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
	printf("pos 0x%X mark0 %d mark1 %d\n", ftell(m_fp), mark0, mark1);
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
	printf("pos 0x%X mark0 %d mark1 %d\n", ftell(m_fp), mark0, mark1);
	assert(mark0 == 0 && mark1 == 1);
}

void DPoly::ReadVerticesBuffer() {
	int i, mark;
	unsigned char buf[8];

	mark = freadUint16BE(m_fp);
	assert(mark == 0xFFFF);
	while (1) {
		fread(buf, sizeof(buf), 1, m_fp);
		if (memcmp(buf, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8) == 0) {
			break;
		}
		for (i = 0; i < sizeof(buf); i += 4) {
			int x = (int16)READ_BE_UINT16(buf + i);
			int y = (int16)READ_BE_UINT16(buf + i + 2);
			printf("vertex buffer x=%d y=%d\n", x, y);
		}
	} 
}

void DPoly::WriteAnimFrameToBitmap() {
	char filePath[1024];
	strcpy(filePath, m_setFile);
	char *p = strrchr(filePath, '.');
	if (!p) {
		p = filePath + strlen(filePath);
	}
	sprintf(p, "-%03d.BMP", m_currentAnimFrame);
	WriteBitmapFile(filePath, DRAWING_BUFFER_W, DRAWING_BUFFER_H, m_gfx->_layer, m_palette);
	++m_currentAnimFrame;
}
