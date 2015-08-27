
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
	if (strcasecmp(setFile, "CAILLOU-F.SET") == 0) {
		fseek(m_fp, 0x432, SEEK_SET);
		ReadAffineBuffer();
	}
	if (strcasecmp(setFile, "MEMOSTORY3.SET") == 0) {
		fseek(m_fp, 0x1C56, SEEK_SET);
		ReadAffineBuffer();
	}
	const int offset = GetShapeOffsetForSet(setFile);
	fseek(m_fp, offset, SEEK_SET);
	for (int counter = 0; ; ++counter) {
		const int groupCount = freadUint16BE(m_fp);
		for (int i = 0; i < groupCount; ++i) {
			const int pos = ftell(m_fp);
			m_numShapes = freadUint16BE(m_fp);
			if (feof(m_fp)) {
				return;
			}
			printf("counter %d group %d/%d numShapes %d pos 0x%X\n", counter, i, groupCount, m_numShapes, pos);
			ReadFrame();
			WriteShapeToBitmap(counter, i);
		}
	}
}

void DPoly::ReadFrame() {
	memset(m_gfx->_layer, 0, DRAWING_BUFFER_W * DRAWING_BUFFER_H);
	m_numShapes--;
	assert(m_numShapes >= 0);
	for (int j = 0; j < m_numShapes; ++j) {
		ReadShapeMarker();
		int numVertices = freadByte(m_fp);
		int ix = (int16_t)freadUint16BE(m_fp);
		int iy = (int16_t)freadUint16BE(m_fp);
		int color1 = freadByte(m_fp);
		int color2 = freadByte(m_fp);
		printf(" shape %d/%d x=%d y=%d color1=%d color2=%d\n", j, m_numShapes, ix, iy, color1, color2);
		m_gfx->setClippingRect(8, 50, 240, 128);
		if (numVertices == 255) {
			int rx = (int16_t)freadUint16BE(m_fp);
			int ry = (int16_t)freadUint16BE(m_fp);
			Point pt;
			pt.x = ix;
			pt.y = iy;
			m_gfx->drawEllipse(color1, false, &pt, rx, ry);
			continue;
		}
		assert((numVertices & 0x80) == 0);
		assert(numVertices < MAX_VERTICES);
		for (int i = 0; i < numVertices; ++i) {
			m_vertices[i].x = (int16_t)freadUint16BE(m_fp);
			m_vertices[i].y = (int16_t)freadUint16BE(m_fp);
			printf("  vertex %d/%d x=%d y=%d\n", i, numVertices, m_vertices[i].x, m_vertices[i].y);
		}
		m_gfx->drawPolygon(color1, false, m_vertices, numVertices);
	}
	ReadPaletteMarker();
	for (int i = 0; i < 16; ++i) {
		m_amigaPalette[i] = freadUint16BE(m_fp);
	}
	SetPalette(m_amigaPalette);
	const int b = freadByte(m_fp); /* 0x00 */
//	assert(b == 0);
	printf("after palette pos 0x%x, b %d\n", (int)ftell(m_fp), b);
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
		return 0x5E4;
	}
	if (strcasecmp(filename, "MEMOTECH.SET") == 0) {
		return 0x52EC;
	}
	if (strcasecmp(filename, "MEMOSTORY0.SET") == 0) {
		return 0x55D6;
	}
	if (strcasecmp(filename, "MEMOSTORY1.SET") == 0) {
		return 0x7070;
	}
	if (strcasecmp(filename, "MEMOSTORY2.SET") == 0) {
		return 0x49FA;
	}
	if (strcasecmp(filename, "MEMOSTORY3.SET") == 0) {
		return 0x347E;
	}
	if (strcasecmp(filename, "JUPITERStation1.set") == 0) {
		return 0x822A;
	}
	if (strcasecmp(filename, "TAKEMecha1-F.set") == 0) {
		return 0x822A;
	}
	return 0;
}

/* 0x00 0x00 0x00 0x00 0x01 */
void DPoly::ReadShapeMarker() {
	int mark0, mark2, mark1;

	mark0 = freadUint16BE(m_fp);
	mark2 = freadUint16BE(m_fp);
	mark1 = freadByte(m_fp);
	printf("shape - pos 0x%X mark0 %d mark2 %d mark1 %d\n", (int)ftell(m_fp), mark0, mark2, mark1);
	assert(mark0 == 0 && mark2 == 0 && (mark1 == 1 || mark1 == 0));
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
	assert(mark0 == 0 && (mark1 != 0 && mark1 < 16));
}

void DPoly::ReadSequenceBuffer() {
	int i, mark, count;
	unsigned char buf[6];

	mark = freadUint16BE(m_fp);
	assert(mark == 0xFFFF);
	count = freadUint16BE(m_fp);
	printf("sequence count %d\n", count);
	while (1) {
		mark = freadUint16BE(m_fp);
//		assert(mark == 0);
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

void DPoly::ReadAffineBuffer() {
	int i, mark, count;

	mark = freadUint16BE(m_fp);
	assert(mark == 0xFFFF);
	count = freadUint16BE(m_fp);
	printf("unk count %d\n", count);
	for (i = 0; i < count; ++i) {
		int x1 = (int16_t)freadUint16BE(m_fp);
		int y1 = (int16_t)freadUint16BE(m_fp);
		printf("  bounds=%d .x1=%d .y1=%d\n", i, x1, y1);
	}
	mark = freadUint16BE(m_fp);
	assert(mark == 0xFFFF);
	for (i = 0; i < 62; ++i) {
		int r1 = (int16_t)freadUint16BE(m_fp);
		int r2 = (int16_t)freadUint16BE(m_fp);
		int r3 = (int16_t)freadUint16BE(m_fp);
		printf("  rotation .r1=%d .r2=%d .r3=%d\n", r1, r2, r3);
	}
	printf("pos 0x%x\n", (int)ftell(m_fp));
}

void DPoly::WriteShapeToBitmap(int group, int shape) {
	char filePath[1024];
	strcpy(filePath, m_setFile);
	char *p = strrchr(filePath, '.');
	if (!p) {
		p = filePath + strlen(filePath);
	}
	sprintf(p, "-SHAPE-%02d-%03d.BMP", group, shape);
	WriteBitmapFile(filePath, DRAWING_BUFFER_W, DRAWING_BUFFER_H, m_gfx->_layer, m_palette);
	++m_currentShape;
}
