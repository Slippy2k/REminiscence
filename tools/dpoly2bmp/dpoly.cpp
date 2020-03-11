
#include <math.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "bitmap.h"
#include "dpoly.h"
#include "file.h"
#include "graphics.h"

static const bool kNoClipping = false;

static void findOffsetsForSet(const char *filename, uint32_t *shapesOffset, uint32_t *affinesOffset, uint32_t *count) {
	*shapesOffset = 0;
	*affinesOffset = 0;
	*count = 0;
	FILE *fp = fopen("dpoly.txt", "r");
	if (fp) {
		char buf[256];
		int found = 0;
		while (fgets(buf, sizeof(buf), fp)) {
			if (buf[0] == '#') {
				continue;
			}
			if (found) {
				if (sscanf(buf, "\t0x%x,0x%x,%d,", shapesOffset, affinesOffset, count) > 1) {
					break;
				}
			}
			found = (strncasecmp(buf, filename, strlen(filename)) == 0);
		}
		fclose(fp);
	}
}

void DPoly::Decode(const char *setFile) {
	_fp = fopen(setFile, "rb");
	if (!_fp) {
		return;
	}
	const char *p = strrchr(setFile, '/');
	_setFile = p ? p + 1 : setFile;
	_gfx.setLayer((uint8_t *)malloc(DRAWING_BUFFER_W * DRAWING_BUFFER_H), DRAWING_BUFFER_W);
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
	//assert(unk == 0xFFFFFFFF);
	uint32_t shapesOffset, affinesOffset, affinesCount;
	findOffsetsForSet(_setFile, &shapesOffset, &affinesOffset, &affinesCount);
	if (shapesOffset == 0) {
		fprintf(stderr, "set file '%s' has no known offsets\n", _setFile);
		return;
	}
	_points = 0;
	_rotations = 0;
	if (affinesOffset != 0) {
		fseek(_fp, affinesOffset, SEEK_SET);
		ReadAffineBuffer(affinesCount, count);
	}
	const int offset = shapesOffset;
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
					if (_points == 0) {
						DecodeShape(count, dx, dy, frame);
					} else {
						DrawShapeScaleRotate(count, dx, dy, frame);
					}
					DecodePalette();
					DoFrameLUT();
				}
				++total;
			}
			WriteFrameToBitmap(i);
		}
	}
}

#define SIN(a) (int16_t)(sin(a * M_PI / 180) * 256)
#define COS(a) (int16_t)(cos(a * M_PI / 180) * 256)

static uint32_t rotData[4];
static Point _vertices[256];

void setRotationTransform(uint16_t a, uint16_t b, uint16_t c) { // identity a:0 b:180 c:90
        const int16_t sin_a = SIN(a);
        const int16_t cos_a = COS(a);
        const int16_t sin_c = SIN(c);
        const int16_t cos_c = COS(c);
        const int16_t sin_b = SIN(b);
        const int16_t cos_b = COS(b);
        rotData[0] = ((cos_a * cos_b) >> 8) - ((((cos_c * sin_a) >> 8) * sin_b) >> 8);
        rotData[1] = ((sin_a * cos_b) >> 8) + ((((cos_c * cos_a) >> 8) * sin_b) >> 8);
        rotData[2] = ( sin_c * sin_a) >> 8;
        rotData[3] = (-sin_c * cos_a) >> 8;
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
		for (int i = 0; i < numVertices; ++i) {
			int vx = (int16_t)freadUint16BE(_fp);
			int vy = (int16_t)freadUint16BE(_fp);
			vertices[i].x = dx + vx;
			vertices[i].y = dy + vy;
		}
		_gfx.drawPolygon(color1, false, vertices, numVertices);
	}
}

uint32_t _shape_cur_x16, _shape_cur_y16;
int16_t _shape_cur_x, _shape_cur_y;
uint32_t _shape_prev_x16, _shape_prev_y16;
int16_t _shape_prev_x, _shape_prev_y;
int16_t _shape_ox, _shape_oy;

void DPoly::DrawShapeScaleRotate(int count, int dx, int dy, int shape) {

	count--;
	assert(count >= 0);

	assert(shape != -1 && shape < _points);
	const int _shape_ix = _rotPt[shape][0];
	const int _shape_iy = _rotPt[shape][1];
	const int d = dx;
	const int e = dy;
	int zoom = 512;
	if (_currentShapeRot != -1) {
		setRotationTransform(_rotMat[_currentShapeRot][0], _rotMat[_currentShapeRot][1], _rotMat[_currentShapeRot][2]);
		zoom = _rotMat[_currentShapeRot][3];
	} else {
		setRotationTransform(0, 180, 90);
	}
	for (int j = 0; j < count; ++j) {
		ReadShapeMarker();
		int numVertices = freadByte(_fp);
		const int ix = (int16_t)freadUint16BE(_fp);
		const int iy = (int16_t)freadUint16BE(_fp);
		int color1 = freadByte(_fp);
		int color2 = freadByte(_fp);
		printf(" shape %d/%d ix=%d iy=%d color1=%d color2=%d\n", j, count, ix, iy, color1, color2);
		assert(color1 == color2);

		int _shape_count = j;
		int f = 0;
		int g = 0;

		if (numVertices & 0x80) {
			int16_t x, y; //, ix, iy;
			Point pr[2];
			Point *pt = _vertices;
			_shape_cur_x = ix; // = b + READ_BE_UINT16(data); data += 2;
			_shape_cur_y = iy; // = c + READ_BE_UINT16(data); data += 2;
			x = (int16_t)freadUint16BE(_fp); // READ_BE_UINT16(data); data += 2;
			y = (int16_t)freadUint16BE(_fp); // READ_BE_UINT16(data); data += 2;
			_shape_cur_x16 = _shape_ix - ix;
			_shape_cur_y16 = _shape_iy - iy;
			_shape_ox = _shape_cur_x = _shape_ix + ((_shape_cur_x16 * rotData[0] + _shape_cur_y16 * rotData[1]) >> 8);
			_shape_oy = _shape_cur_y = _shape_iy + ((_shape_cur_x16 * rotData[2] + _shape_cur_y16 * rotData[3]) >> 8);
			pr[0].x =  0;
			pr[0].y = -y;
			pr[1].x = -x;
			pr[1].y =  y;
			if (_shape_count == 0) {
				f -= ((_shape_ix - _shape_cur_x) * zoom * 128 + 0x8000) >> 16;
				g -= ((_shape_iy - _shape_cur_y) * zoom * 128 + 0x8000) >> 16;
				pt->x = f;
				pt->y = g;
				++pt;
				_shape_cur_x16 = f << 16;
				_shape_cur_y16 = g << 16;
			} else {
				_shape_cur_x16 = _shape_prev_x16 + (_shape_cur_x - _shape_prev_x) * zoom * 128;
				_shape_cur_y16 = _shape_prev_y16 + (_shape_cur_y - _shape_prev_y) * zoom * 128;
				pt->x = (_shape_cur_x16 + 0x8000) >> 16;
				pt->y = (_shape_cur_y16 + 0x8000) >> 16;
				++pt;
			}
			for (int i = 0; i < 2; ++i) {
				_shape_cur_x += pr[i].x;
				_shape_cur_x16 += pr[i].x * zoom * 128;
				pt->x = (_shape_cur_x16 + 0x8000) >> 16;
				_shape_cur_y += pr[i].y;
				_shape_cur_y16 += pr[i].y * zoom * 128;
				pt->y = (_shape_cur_y16 + 0x8000) >> 16;
				++pt;
			}
			_shape_prev_x = _shape_cur_x;
			_shape_prev_y = _shape_cur_y;
			_shape_prev_x16 = _shape_cur_x16;
			_shape_prev_y16 = _shape_cur_y16;
			Point po;
			po.x = _vertices[0].x + d + _shape_ix;
			po.y = _vertices[0].y + e + _shape_iy;
			int16_t rx = _vertices[0].x - _vertices[2].x;
			int16_t ry = _vertices[0].y - _vertices[1].y;
			_gfx.drawEllipse(color1, false, &po, rx, ry);
		} else {
			int16_t x, y, a, shape_last_x, shape_last_y;
			Point tempVertices[80];
			_shape_cur_x = ix; // b + READ_BE_UINT16(data); data += 2;
			x = _shape_cur_x;
			_shape_cur_y = iy; // c + READ_BE_UINT16(data); data += 2;
			y = _shape_cur_y;
			_shape_cur_x16 = _shape_ix - x;
			_shape_cur_y16 = _shape_iy - y;

			a = _shape_ix + ((rotData[0] * _shape_cur_x16 + rotData[1] * _shape_cur_y16) >> 8);
			if (_shape_count == 0) {
				_shape_ox = a;
			}
			_shape_cur_x = shape_last_x = a;
			a = _shape_iy + ((rotData[2] * _shape_cur_x16 + rotData[3] * _shape_cur_y16) >> 8);
			if (_shape_count == 0) {
				_shape_oy = a;
			}
			_shape_cur_y = shape_last_y = a;

			Point *pt2 = tempVertices;
			for (int i = 0; i < numVertices; ++i) {
				int ix = (int16_t)freadUint16BE(_fp);
				int iy = (int16_t)freadUint16BE(_fp);
				_shape_cur_x16 = _shape_ix - ix;
				_shape_cur_y16 = _shape_iy - iy;
				a = _shape_ix + ((rotData[0] * _shape_cur_x16 + rotData[1] * _shape_cur_y16) >> 8);
				pt2->x = a - shape_last_x;
				shape_last_x = a;
				a = _shape_iy + ((rotData[2] * _shape_cur_x16 + rotData[3] * _shape_cur_y16) >> 8);
				pt2->y = a - shape_last_y;
				shape_last_y = a;
				++pt2;
			}
			Point *pt = _vertices;
			if (_shape_count == 0) {
				int ix = _shape_ox;
				int iy = _shape_oy;
				f -= (((_shape_ix - ix) * zoom * 128) + 0x8000) >> 16;
				g -= (((_shape_iy - iy) * zoom * 128) + 0x8000) >> 16;
				pt->x = f + _shape_ix + d;
				pt->y = g + _shape_iy + e;
//				++pt;
				_shape_cur_x16 = f << 16;
				_shape_cur_y16 = g << 16;
			} else {
				_shape_cur_x16 = _shape_prev_x16 + ((_shape_cur_x - _shape_prev_x) * zoom * 128);
				pt->x = _shape_ix + d + ((_shape_cur_x16 + 0x8000) >> 16);
				_shape_cur_y16 = _shape_prev_y16 + ((_shape_cur_y - _shape_prev_y) * zoom * 128);
				pt->y = _shape_iy + e + ((_shape_cur_y16 + 0x8000) >> 16);
//				++pt;
			}
			for (int i = 0; i < numVertices; ++i) {
				_shape_cur_x += tempVertices[i].x;
				_shape_cur_x16 += tempVertices[i].x * zoom * 128;
				pt->x = d + _shape_ix + ((_shape_cur_x16 + 0x8000) >> 16);
				_shape_cur_y += tempVertices[i].y;
				_shape_cur_y16 += tempVertices[i].y * zoom * 128;
				pt->y = e + _shape_iy + ((_shape_cur_y16 + 0x8000) >> 16);
				++pt;
			}
			_shape_prev_x = _shape_cur_x;
			_shape_prev_y = _shape_cur_y;
			_shape_prev_x16 = _shape_cur_x16;
			_shape_prev_y16 = _shape_cur_y16;
			_gfx.drawPolygon(color1, false, _vertices, numVertices); // + 1);
		}
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

	if (rotations != 0) {
		fseek(_fp, -rotations * 2 - 2, SEEK_CUR);
		printf("fpos2 0x%x\n", (int)ftell(_fp));
		mark = freadUint16BE(_fp);
		assert(mark == 0xFFFF);
		for (i = 0; i < rotations; ++i) {
			const int a = (int16_t)freadUint16BE(_fp);
			printf("  a[%d]=%d (%d)\n", i, a, a + 512);
			assert(i < MAX_ROTATIONS);
			_rotMat[i][3] = 512 + a;
		}
	}
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
	if (rotations != 0) {
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
	}
	_rotations = rotations;
	if (unk != 0) {
		mark = freadUint16BE(_fp);
		if (mark != 0xFFFF) {
			fseek(_fp, -2, SEEK_CUR);
			return;
		}
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
