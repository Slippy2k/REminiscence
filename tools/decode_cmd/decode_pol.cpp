
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bitmap.h"
#include "graphics.h"

static uint32_t _shapeOffset; // 0x0
static uint32_t _paletteOffset; // 0x4
static uint32_t _shapeDataOffset; // 0xA
static uint32_t _verticesOffset; // 0x8
static uint32_t _verticesDataOffset; // 0x10

static uint8_t _buffer[0x10000];

static uint8_t _screen[256 * 256];

static Graphics _gfx;

static Point _vertices[128];
static uint8_t _primitiveColor;
static bool _hasAlphaColor;

static uint32_t readBE32(const uint8_t *p) {
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static uint16_t READ_BE_UINT16(const uint8_t *p) {
	return (p[0] << 8) | p[1];
}

static void drawShape(const uint8_t *data, int x, int y) {
	uint8_t numVertices = *data++;
	if (numVertices & 0x80) {
		Point pt;
		pt.x = READ_BE_UINT16(data) + x; data += 2;
		pt.y = READ_BE_UINT16(data) + y; data += 2;
		uint16_t rx = READ_BE_UINT16(data); data += 2;
		uint16_t ry = READ_BE_UINT16(data); data += 2;
		_gfx.drawEllipse(_primitiveColor, _hasAlphaColor, &pt, rx, ry);
	} else if (numVertices == 0) {
		Point pt;
		pt.x = READ_BE_UINT16(data) + x; data += 2;
		pt.y = READ_BE_UINT16(data) + y; data += 2;
		_gfx.drawPoint(_primitiveColor, &pt);
	} else {
		Point *pt = _vertices;
		int16_t ix = READ_BE_UINT16(data); data += 2;
		int16_t iy = READ_BE_UINT16(data); data += 2;
		pt->x = ix + x;
		pt->y = iy + y;
		++pt;
		int16_t n = numVertices - 1;
		++numVertices;
		for (; n >= 0; --n) {
			int16_t dx = (int8_t)*data++;
			int16_t dy = (int8_t)*data++;
			if (dy == 0 && n != 0 && *(data + 1) == 0) {
				ix += dx;
				--numVertices;
			} else {
				ix += dx;
				iy += dy;
				pt->x = ix + x;
				pt->y = iy + y;
				++pt;
			}
		}
		_gfx.drawPolygon(_primitiveColor, _hasAlphaColor, _vertices, numVertices);
	}
}

static void dumpShape(int shapeNum, int x, int y) {
	memset(_screen, 0, sizeof(_screen));
	_gfx.setLayer(_screen, 256);
	_gfx.setClippingRect(0, 0, 256, 256);

	const uint8_t *shapeOffsetTable    = _buffer + _shapeOffset;
	const uint8_t *shapeDataTable      = _buffer + _shapeDataOffset;
	const uint8_t *verticesOffsetTable = _buffer + _verticesOffset;
	const uint8_t *verticesDataTable   = _buffer + _verticesDataOffset;

	const uint8_t *shapeData = shapeDataTable + READ_BE_UINT16(shapeOffsetTable + (shapeNum & 0x7FF) * 2);
	uint16_t primitiveCount = READ_BE_UINT16(shapeData); shapeData += 2;

	while (primitiveCount--) {
		uint16_t verticesOffset = READ_BE_UINT16(shapeData); shapeData += 2;
		const uint8_t *primitiveVertices = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2);
		int16_t dx = 0;
		int16_t dy = 0;
		if (verticesOffset & 0x8000) {
			dx = READ_BE_UINT16(shapeData); shapeData += 2;
			dy = READ_BE_UINT16(shapeData); shapeData += 2;
                }
		_hasAlphaColor = (verticesOffset & 0x4000) != 0;
		uint8_t color = *shapeData++;
		_primitiveColor = 0xC0 + color;
		drawShape(primitiveVertices, x + dx, y + dy);
	}
	uint8_t paletteBuffer[256 * 3];
	for (int i = 0; i < 256; ++i) {
		paletteBuffer[3 * i] = paletteBuffer[3 * i + 1] = paletteBuffer[3 * i + 2] = i;
	}
	char name[64];
	snprintf(name, sizeof(name), "shape_%02d.bmp", shapeNum);
	WriteFile_BMP_PAL(name, 256, 256, 256, _screen, paletteBuffer);
}

static void dumpPalette(int paletteNum, const uint8_t *p) {
	static const int W = 16;
	static const int H = 16;
	for (int color = 0; color < 16; ++color) {
		for (int y = 0; y < H; ++y) {
			memset(_screen + y * 256 + color * W, color, W);
		}
	}
	uint8_t paletteBuffer[256 * 3];
	for (int i = 0; i < 16; ++i) {
		const uint16_t color = READ_BE_UINT16(p + i * 2);
		const int r = (color >> 8) & 15;
		const int g = (color >> 4) & 15;
		const int b =  color       & 15;
		paletteBuffer[i * 3]     = (r << 4) | r;
		paletteBuffer[i * 3 + 1] = (g << 4) | g;
		paletteBuffer[i * 3 + 2] = (b << 4) | b;
	}
	char name[64];
	snprintf(name, sizeof(name), "palette_%02d.bmp", paletteNum);
	WriteFile_BMP_PAL(name, W * 16, H, 256, _screen, paletteBuffer);
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			fread(_buffer, 1, sizeof(_buffer), fp);
			_shapeOffset = readBE32(_buffer);
			_paletteOffset = readBE32(_buffer + 0x4);
			_shapeDataOffset = readBE32(_buffer + 0xC);
			int paletteNum = 0;
			for (uint32_t offset = _paletteOffset; offset < _shapeDataOffset; offset += 32) {
				dumpPalette(paletteNum, _buffer + offset);
				++paletteNum;
			}
			_verticesOffset = readBE32(_buffer + 0x8);
			_verticesDataOffset = readBE32(_buffer + 0x10);
			int shapeNum = 0;
			for (uint32_t offset = _shapeOffset; offset < _paletteOffset; offset += 2) {
				dumpShape(shapeNum, 0, 0);
				++shapeNum;
			}
			fclose(fp);
		}
	}
	return 0;
}
