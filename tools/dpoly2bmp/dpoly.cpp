
#include <sys/param.h>
#include "bitmap.h"
#include "dpoly.h"
#include "file.h"
#include "graphics.h"


void DPoly::Decode(const char *setFile) {
	_fp = fopen(setFile, "rb");
	if (!_fp) {
		return;
	}
	_setFile = setFile;
	_gfx = new Graphics;
	_gfx->_layer = (uint8_t *)malloc(DRAWING_BUFFER_W * DRAWING_BUFFER_H);
	memset(_seqOffsets, 0, sizeof(_seqOffsets));
	memset(_shapesOffsets, 0, sizeof(_shapesOffsets));
	uint8_t hdr[8];
	int ret = fread(hdr, 8, 1, _fp);
	if (ret != 1) {
		fprintf(stderr, "fread() failed, ret %d", ret);
	}
	assert(memcmp(hdr, "POLY\x00\x0A\x00\x02", 8) == 0) ;
	ReadSequenceBuffer();
	if (strcasecmp(setFile, "CAILLOU-F.SET") == 0) {
		fseek(_fp, 0x432, SEEK_SET);
		ReadAffineBuffer();
	}
	if (strcasecmp(setFile, "MEMOSTORY3.SET") == 0) {
		fseek(_fp, 0x1C56, SEEK_SET);
		ReadAffineBuffer();
	}
	const int offset = GetShapeOffsetForSet(setFile);
	fseek(_fp, offset, SEEK_SET);
	for (int counter = 0; ; ++counter) {
		const int groupCount = freadUint16BE(_fp);
		if (feof(_fp)) {
			break;
		}
		for (int i = 0; i < groupCount; ++i) {
			const int pos = ftell(_fp);
			assert(counter < 2 && i < MAX_SHAPES);
			_shapesOffsets[counter][i] = pos;
			const int numShapes = freadUint16BE(_fp);
			printf("counter %d group %d/%d numShapes %d pos 0x%X\n", counter, i, groupCount, numShapes, pos);
			memset(_gfx->_layer, 0, DRAWING_BUFFER_W * DRAWING_BUFFER_H);
			DecodeShape(numShapes, 0, 0);
			WriteShapeToBitmap(counter, i);
		}
	}
	if (1) {
		memset(_gfx->_layer, 0, DRAWING_BUFFER_W * DRAWING_BUFFER_H);
		_gfx->setClippingRect(8, 50, 240, 128);

		for (int i = 0; _seqOffsets[i] != 0; ++i) {
			// background
			int shapeOffset = _shapesOffsets[0][0];
			if (shapeOffset != 0) {
				fseek(_fp, shapeOffset, SEEK_SET);
				int count = freadUint16BE(_fp);
				DecodeShape(count, 0, 0);
			}
			// shapes
			fseek(_fp, _seqOffsets[i], SEEK_SET);
			int shapesCount = freadUint16BE(_fp);
			for (int j = 0; j < shapesCount; ++j) {
				fseek(_fp, _seqOffsets[i] + 2 + j * 6, SEEK_SET);
				int frame = freadUint16BE(_fp);
				int dx = (int16_t)freadUint16BE(_fp);
				int dy = (int16_t)freadUint16BE(_fp);
				shapeOffset = _shapesOffsets[1][frame];
				if (shapeOffset != 0) {
					fseek(_fp, shapeOffset, SEEK_SET);
					int count = freadUint16BE(_fp);
					DecodeShape(count, dx, dy);
				}
			}
			WriteFrameToBitmap(i);
		}
	}
}

void DPoly::DecodeShape(int count, int dx, int dy) {
	count--;
	assert(count >= 0);
	for (int j = 0; j < count; ++j) {
		ReadShapeMarker();
		int numVertices = freadByte(_fp);
		int ix = (int16_t)freadUint16BE(_fp);
		int iy = (int16_t)freadUint16BE(_fp);
		int color1 = freadByte(_fp);
		int color2 = freadByte(_fp);
		printf(" shape %d/%d x=%d y=%d color1=%d color2=%d\n", j, count, ix, iy, color1, color2);
		_gfx->setClippingRect(8, 50, 240, 128);
		if (numVertices == 255) {
			int rx = dx + (int16_t)freadUint16BE(_fp);
			int ry = dy + (int16_t)freadUint16BE(_fp);
			Point pt;
			pt.x = ix;
			pt.y = iy;
			_gfx->drawEllipse(color1, false, &pt, rx, ry);
			continue;
		}
		assert((numVertices & 0x80) == 0);
		assert(numVertices < MAX_VERTICES);
		for (int i = 0; i < numVertices; ++i) {
			_vertices[i].x = dx + (int16_t)freadUint16BE(_fp);
			_vertices[i].y = dy + (int16_t)freadUint16BE(_fp);
			printf("  vertex %d/%d x=%d y=%d\n", i, numVertices, _vertices[i].x, _vertices[i].y);
		}
		_gfx->drawPolygon(color1, false, _vertices, numVertices);
	}
	ReadPaletteMarker();
	for (int i = 0; i < 16; ++i) {
		_amigaPalette[i] = freadUint16BE(_fp);
	}
	SetPalette(_amigaPalette);
	const int b = freadByte(_fp); /* 0x00 */
//	assert(b == 0);
	printf("after palette pos 0x%x, b %d\n", (int)ftell(_fp), b);
}

void DPoly::SetPalette(const uint16_t *pal) {
	memset(_palette, 0, sizeof(_palette));
	for (int i = 0; i < 16; ++i) {
		_palette[i * 3 + 0] = (pal[i] >> 8) & 0xF;
		_palette[i * 3 + 1] = (pal[i] >> 4) & 0xF;
		_palette[i * 3 + 2] = (pal[i] >> 0) & 0xF;
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

	mark0 = freadUint16BE(_fp);
	mark2 = freadUint16BE(_fp);
	mark1 = freadByte(_fp);
	printf("shape - pos 0x%X mark0 %d mark2 %d mark1 %d\n", (int)ftell(_fp), mark0, mark2, mark1);
	assert(mark0 == 0 && mark2 == 0 && (mark1 == 1 || mark1 == 0));
}

/* 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x01 0x01 */
void DPoly::ReadPaletteMarker() {
	int mark0, mark1;
	
	mark0 = freadUint16BE(_fp);
	mark0 &= freadUint16BE(_fp);
	mark0 &= freadUint16BE(_fp);
	mark0 &= freadUint16BE(_fp);
	mark0 &= freadUint16BE(_fp);
	mark1 = freadByte(_fp);
	mark1 &= freadByte(_fp);
	printf("palette - pos 0x%X mark0 %d mark1 %d\n", (int)ftell(_fp), mark0, mark1);
	assert(mark0 == 0 && (mark1 != 0 && mark1 < 16));
}

void DPoly::ReadSequenceBuffer() {
	int i, mark, count, num;
	unsigned char buf[6];

	mark = freadUint16BE(_fp);
	assert(mark == 0xFFFF);
	count = freadUint16BE(_fp);
	printf("sequence count %d\n", count);
	num = 0;
	while (1) {
		mark = freadUint16BE(_fp);
//		assert(mark == 0);
		assert(num < MAX_SEQUENCES);
		_seqOffsets[num++] = ftell(_fp);
		count = freadUint16BE(_fp);
		printf("sequence - mark %d count %d pos 0x%x\n", mark, count, (int)ftell(_fp));
		if (count == 0) {
			break;
		}
		for (i = 0; i < count; ++i) {
			const int ret = fread(buf, 1, sizeof(buf), _fp);
			if (ret != sizeof(buf)) {
				fprintf(stderr, "fread() failed, ret %d\n", ret);
			}
			printf("  frame=%d .x=%d .y=%d\n", READ_BE_UINT16(buf), (int16_t)READ_BE_UINT16(buf + 2), (int16_t)READ_BE_UINT16(buf + 4));
		}
	}
}

void DPoly::ReadAffineBuffer() {
	int i, mark, count;

	mark = freadUint16BE(_fp);
	assert(mark == 0xFFFF);
	count = freadUint16BE(_fp);
	printf("unk count %d\n", count);
	for (i = 0; i < count; ++i) {
		int x1 = (int16_t)freadUint16BE(_fp);
		int y1 = (int16_t)freadUint16BE(_fp);
		printf("  bounds=%d .x1=%d .y1=%d\n", i, x1, y1);
	}
	mark = freadUint16BE(_fp);
	assert(mark == 0xFFFF);
	for (i = 0; i < 62; ++i) {
		int r1 = (int16_t)freadUint16BE(_fp);
		int r2 = (int16_t)freadUint16BE(_fp);
		int r3 = (int16_t)freadUint16BE(_fp);
		printf("  rotation .r1=%d .r2=%d .r3=%d\n", r1, r2, r3);
	}
	printf("pos 0x%x\n", (int)ftell(_fp));
}

void DPoly::WriteShapeToBitmap(int group, int shape) {
	char filePath[MAXPATHLEN];
	strcpy(filePath, _setFile);
	char *p = strrchr(filePath, '.');
	if (!p) {
		p = filePath + strlen(filePath);
	}
	sprintf(p, "-SHAPE-%02d-%03d.BMP", group, shape);
	WriteBitmapFile(filePath, DRAWING_BUFFER_W, DRAWING_BUFFER_H, _gfx->_layer, _palette);
}

void DPoly::WriteFrameToBitmap(int frame) {
	char filePath[MAXPATHLEN];
	strcpy(filePath, _setFile);
	char *p = strrchr(filePath, '.');
	if (!p) {
		p = filePath + strlen(filePath);
	}
	sprintf(p, "-FRAME-%03d.BMP", frame);
	WriteBitmapFile(filePath, DRAWING_BUFFER_W, DRAWING_BUFFER_H, _gfx->_layer, _palette);
}
