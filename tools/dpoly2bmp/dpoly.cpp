
#include <math.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "bitmap.h"
#include "dpoly.h"
#include "file.h"
#include "graphics.h"

static const bool kNoClipping = false;

void DPoly::Decode(const char *setFile) {
	_fp = fopen(setFile, "rb");
	if (!_fp) {
		return;
	}
	_setFile = setFile;
	_gfx._layer = (uint8_t *)malloc(DRAWING_BUFFER_W * DRAWING_BUFFER_H);
	int gfxOffsetX = 0;
	int gfxOffsetY = 0;
	if (kNoClipping) {
		_gfx.setClippingRect(0, 0, DRAWING_BUFFER_W, DRAWING_BUFFER_H);
		gfxOffsetX = GFX_CLIP_X;
		gfxOffsetY = GFX_CLIP_Y;
	} else {
		_gfx.setClippingRect(GFX_CLIP_X, GFX_CLIP_Y, GFX_CLIP_W, GFX_CLIP_H);
	}
	memset(_seqOffsets, 0, sizeof(_seqOffsets));
	memset(_shapesOffsets, 0, sizeof(_shapesOffsets));
	uint8_t hdr[8];
	int ret = fread(hdr, 8, 1, _fp);
	if (ret != 1) {
		fprintf(stderr, "fread() failed, ret %d", ret);
	}
	assert(memcmp(hdr, "POLY\x00\x0A\x00\x02", 8) == 0) ;
	const int count = ReadSequenceBuffer();
	printf("fpos1 0x%x\n", (int)ftell(_fp));
	ret = fread(_unkData, count, 4, _fp);
	const uint32_t unk = freadUint32BE(_fp);
	printf("fpos2 0x%x unk 0x%x\n", (int)ftell(_fp) - 4, unk);
	assert(unk == 0xFFFFFFFF);
	_points = 0;
	_rotations = 0;
	if (strcasecmp(setFile, "CAILLOU-F.SET") == 0) {
		fseek(_fp, 0x432, SEEK_SET);
		ReadAffineBuffer(62, 0);
	}
	if (strcasecmp(setFile, "MEMOSTORY3.SET") == 0) {
		fseek(_fp, 0x1C56, SEEK_SET);
		ReadAffineBuffer(607, count);
	}
	if (strcasecmp(setFile, "JUPITERStation1.set") == 0 || strcasecmp(setFile, "TAKEMecha1-F.set") == 0) {
		fseek(_fp, 0x443A, SEEK_SET);
		ReadAffineBuffer(1231, count);
	}
	const int offset = GetShapeOffsetForSet(setFile);
	printf("fpos3 0x%x (0x%x)\n", (int)ftell(_fp), offset);
	fseek(_fp, offset, SEEK_SET);
	for (int counter = 0; ; ++counter) {
		const int groupCount = freadUint16BE(_fp);
		if (feof(_fp)) {
			break;
		}
		if (counter >= 2) {
			fprintf(stderr, "Unexpected group %d count %d\n", counter, groupCount);
			break;
		}
		for (int i = 0; i < groupCount; ++i) {
			const int pos = ftell(_fp);
			assert(counter < 2 && i < MAX_SHAPES);
			_shapesOffsets[counter][i] = pos;
			const int numShapes = freadUint16BE(_fp);
			printf("counter %d group %d/%d numShapes %d pos 0x%X\n", counter, i, groupCount, numShapes, pos);
			memset(_gfx._layer, 0xFF, DRAWING_BUFFER_W * DRAWING_BUFFER_H);
			DecodeShape(numShapes, gfxOffsetX, gfxOffsetY);
			DecodePalette();
			DumpPalette();
			WriteShapeToBitmap(counter, i);
		}
	}
	struct stat st;
	stat(setFile, &st);
	printf("fpos4 0x%x (0x%x)\n", (int)ftell(_fp), (int)st.st_size);
	if (1) {
		int total = 0;
		for (int i = 0; _seqOffsets[i] != 0; ++i) {
			// memset(_rgb, 0, sizeof(_rgb));
			for (size_t j = 0; j < sizeof(_rgb) / sizeof(uint32_t); ++j) {
				_rgb[j] = 0xFF000000;
			}
			fprintf(stdout, "sequence %d data 0x%x\n", i, READ_LE_UINT32(_unkData + i * 4));
			// background
			fseek(_fp, _seqOffsets[i], SEEK_SET);
			int background = freadUint16BE(_fp);
			fprintf(stdout, "sequence %d background shape %d\n", i, background);
			assert(background < MAX_SHAPES);
			const int shapeOffset = _shapesOffsets[0][background];
			if (shapeOffset != 0) {
				_currentShapeRot = -1;
				fseek(_fp, shapeOffset, SEEK_SET);
				memset(_gfx._layer, 0xFF, DRAWING_BUFFER_W * DRAWING_BUFFER_H);
				const int count = freadUint16BE(_fp);
				DecodeShape(count, gfxOffsetX, gfxOffsetY);
				DecodePalette();
				DoFrameLUT();
			}
			// shapes
			fseek(_fp, _seqOffsets[i] + 2, SEEK_SET);
			int shapesCount = freadUint16BE(_fp);
			for (int j = 0; j < shapesCount; ++j) {
				fseek(_fp, _seqOffsets[i] + 4 + j * 6, SEEK_SET);
				int frame = freadUint16BE(_fp);
				int dx = gfxOffsetX + (int16_t)freadUint16BE(_fp);
				int dy = gfxOffsetY + (int16_t)freadUint16BE(_fp);
				fprintf(stdout, "sequence %d foreground shape %d frame %d dx %d dy %d total %d\n", i, j, frame, dx, dy, total);
				assert(frame < MAX_SHAPES);
				const int shapeOffset = _shapesOffsets[1][frame];
				if (shapeOffset != 0) {
					_currentShapeRot = (total < _rotations) ? total : -1;
					fseek(_fp, shapeOffset, SEEK_SET);
					memset(_gfx._layer, 0xFF, DRAWING_BUFFER_W * DRAWING_BUFFER_H);
					const int count = freadUint16BE(_fp);
					DecodeShape(count, dx, dy, frame);
					DecodePalette();
					DoFrameLUT();
				}
				++total;
			}
			WriteFrameToBitmap(i);
		}
	}
}

void DPoly::DecodeShape(int count, int dx, int dy, int shape) {
	count--;
	assert(count >= 0);
	for (int j = 0; j < count; ++j) {
		ReadShapeMarker();
		int numVertices = freadByte(_fp);
		int ix = (int16_t)freadUint16BE(_fp);
		int iy = (int16_t)freadUint16BE(_fp);
		int color1 = freadByte(_fp);
		int color2 = freadByte(_fp);
		printf(" shape %d/%d ix=%d iy=%d color1=%d color2=%d\n", j, count, ix, iy, color1, color2);
		assert(color1 == color2);
		if (numVertices == 255) {
			int rx = (int16_t)freadUint16BE(_fp);
			int ry = (int16_t)freadUint16BE(_fp);
			Point pt;
			pt.x = dx + ix;
			pt.y = dy + iy;
			_gfx.drawEllipse(color1, false, &pt, rx, ry);
			continue;
		}
		assert((numVertices & 0x80) == 0);
		Point vertices[MAX_VERTICES];
		assert(numVertices < MAX_VERTICES);
		if (shape != -1 && _currentShapeRot != -1 && _rotMat[_currentShapeRot][0] != 0) {

			// assert(_rotMat[_currentShapeRot][1] == 180);
			assert(_rotMat[_currentShapeRot][2] == 90);

			const float s =  sin(_rotMat[_currentShapeRot][0] * M_PI / 180);
			const float c = -cos(_rotMat[_currentShapeRot][0] * M_PI / 180);

			assert(shape < _points);
			const int ox = _rotPt[shape][0];
			const int oy = _rotPt[shape][1];

			for (int i = 0; i < numVertices; ++i) {
				int x = (ox - (int16_t)freadUint16BE(_fp));
				int y = (oy - (int16_t)freadUint16BE(_fp));
				vertices[i].x = dx + ox + int(x * c - y * s);
				vertices[i].y = dy + oy + int(x * s + y * c);
			}
		} else {
			for (int i = 0; i < numVertices; ++i) {
				vertices[i].x = dx + (int16_t)freadUint16BE(_fp);
				vertices[i].y = dy + (int16_t)freadUint16BE(_fp);
				printf("  vertex %d/%d x=%d y=%d\n", i, numVertices, vertices[i].x, vertices[i].y);
			}
		}
		_gfx.drawPolygon(color1, false, vertices, numVertices);
	}
}

void DPoly::DecodePalette() {
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
	memset(_currentPalette, 0, sizeof(_currentPalette));
	for (int i = 0; i < 16; ++i) {
		const uint8_t r = (pal[i] >> 8) & 0xF;
		_currentPalette[i * 3 + 0] = (r << 4) | r;
		const uint8_t g = (pal[i] >> 4) & 0xF;
		_currentPalette[i * 3 + 1] = (g << 4) | g;
		const uint8_t b = (pal[i] >> 0) & 0xF;
		_currentPalette[i * 3 + 2] = (b << 4) | b;
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
int DPoly::ReadPaletteMarker() {
	int mark0, mark1, mark2;
	
	mark0 = freadUint16BE(_fp);
	mark0 &= freadUint16BE(_fp);
	mark0 &= freadUint16BE(_fp);
	mark0 &= freadUint16BE(_fp);
	mark0 &= freadUint16BE(_fp);
	mark1 = freadByte(_fp);
	mark2 = freadByte(_fp);
	printf("palette - pos 0x%X mark0 %d mark1 %d mark2 %d\n", (int)ftell(_fp), mark0, mark1, mark2);
	assert(mark0 == 0); // && (mark1 != 0 && mark1 < 16));
	assert(mark1 == mark2);
	return mark1;
}

int DPoly::ReadSequenceBuffer() {
	int i, mark, count, num;
	unsigned char buf[6];

	mark = freadUint16BE(_fp);
	assert(mark == 0xFFFF);
	count = freadUint16BE(_fp);
	printf("sequence count %d\n", count);
	num = 0;
	while (1) {
		assert(num < MAX_SEQUENCES);
		mark = freadUint16BE(_fp);
		if (mark == 0xFFFF) {
			break;
		}
		_seqOffsets[num++] = ftell(_fp) - 2;
		count = freadUint16BE(_fp);
		printf("sequence - background %d count %d pos 0x%x\n", mark, count, (int)ftell(_fp));
		for (i = 0; i < count; ++i) {
			const int ret = fread(buf, 1, sizeof(buf), _fp);
			if (ret != sizeof(buf)) {
				fprintf(stderr, "fread() failed, ret %d\n", ret);
			}
			printf("  frame=%d .x=%d .y=%d\n", READ_BE_UINT16(buf), (int16_t)READ_BE_UINT16(buf + 2), (int16_t)READ_BE_UINT16(buf + 4));
		}
	}
	return num;
}

void DPoly::ReadAffineBuffer(int rotations, int unk) {
	int i, mark, count;

	mark = freadUint16BE(_fp);
	assert(mark == 0xFFFF);
	count = freadUint16BE(_fp);
	printf("shapes origin points count %d\n", count);
	assert(count < MAX_SHAPES);
	for (i = 0; i < count; ++i) {
		int ox = (int16_t)freadUint16BE(_fp);
		int oy = (int16_t)freadUint16BE(_fp);
		printf("  origin=%d .x1=%d .y1=%d\n", i, ox, oy);
		_rotPt[i][0] = ox;
		_rotPt[i][1] = oy;
	}
	_points = count;
	mark = freadUint16BE(_fp);
	assert(mark == 0xFFFF);
	for (i = 0; i < rotations; ++i) {
		int r1 = (int16_t)freadUint16BE(_fp);
		int r2 = (int16_t)freadUint16BE(_fp);
		int r3 = (int16_t)freadUint16BE(_fp);
		printf("  rotation=%d .r1=%d .r2=%d .r3=%d\n", i, r1, r2, r3);
		assert(i < MAX_ROTATIONS);
		_rotMat[i][0] = r1;
		_rotMat[i][1] = r2;
		_rotMat[i][2] = r3;
	}
	_rotations = rotations;
	if (unk != 0) {
		mark = freadUint16BE(_fp);
		assert(mark == 0xFFFF);
		for (i = 0; i < unk; ++i) {
			uint32_t val1 = freadUint32BE(_fp);
			assert(val1 == 0xFFFFFFFF);
			uint32_t val2 = freadUint32BE(_fp);
			assert(val2 == 0x02020202);
			uint32_t val3 = freadUint32BE(_fp);
			assert(val3 == 0);
			uint32_t val4 = freadUint32BE(_fp);
			assert(val4 == 0x40404040);
		}
	}
}

void DPoly::DumpPalette() {
	static const int W = 16;
	static const int H = 16;
	for (int color = 0; color < 16; ++color) {
		for (int y = 0; y < H; ++y) {
			memset(_gfx._layer + y * DRAWING_BUFFER_W + color * W, color, W);
		}
	}
}

void DPoly::WriteShapeToBitmap(int group, int shape) {
	char filePath[MAXPATHLEN];
	strcpy(filePath, _setFile);
	char *p = strrchr(filePath, '.');
	if (!p) {
		p = filePath + strlen(filePath);
	}
	sprintf(p, "-SHAPE-%02d-%03d.BMP", group, shape);
	WriteFile_BMP_PAL(filePath, DRAWING_BUFFER_W, DRAWING_BUFFER_H, DRAWING_BUFFER_W, _gfx._layer, _currentPalette);
}

void DPoly::WriteFrameToBitmap(int frame) {
	char tgaPath[MAXPATHLEN];
	strcpy(tgaPath, _setFile);
	char *p = strrchr(tgaPath, '.');
	assert(p);
	sprintf(p, "-FRAME-%03d.TGA", frame);
	if (kNoClipping) {
		WriteFile_TGA_RGB(tgaPath, DRAWING_BUFFER_W, DRAWING_BUFFER_H, DRAWING_BUFFER_W, _rgb);
	} else {
		WriteFile_TGA_RGB(tgaPath, GFX_CLIP_W, GFX_CLIP_H, DRAWING_BUFFER_W, _rgb + GFX_CLIP_Y * DRAWING_BUFFER_W + GFX_CLIP_X);
	}
}

static uint32_t BGRA(int color, const uint8_t *palette) {
	return 0xFF000000 | (palette[color * 3] << 16) | (palette[color * 3 + 1] << 8) | palette[color * 3 + 2];
}

void DPoly::DoFrameLUT() {
	for (int i = 0; i < DRAWING_BUFFER_W * DRAWING_BUFFER_H; ++i) {
		const int color = _gfx._layer[i];
		if (color != 0xFF) {
			_rgb[i] = BGRA(color, _currentPalette);
		}
	}
}
