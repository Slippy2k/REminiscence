/* fbply - Flashback Cutscene Player
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "cutscene.h"
#include "systemstub.h"


void Cutscene::dumpCutNum() {
	int size = 19 * 8;
	for (int i = 0; i < size; i += 2) {
		uint16 num = _offsetsTable[i];
		if (num != 0xFFFF) {
			printf("  num=%d name:%s index:%d\n", i / 2, _namesTable[num & 0xFF], _offsetsTable[i + 1]);
		}
	}
}

void Cutscene::readFile(const char *filename, const char *ext, Buf *b) {
	memset(b, 0, sizeof(Buf));
	char filePath[512];
	sprintf(filePath, "%s/%s.%s", _dataDir, filename, ext);
	char *p = filePath + strlen(_dataDir) + 1;
	string_lower(p);
	FILE *fp = fopen(filePath, "rb");
	if (!fp) {
		string_upper(p);
		fp = fopen(filePath, "rb");
		if (!fp) {
			error("Can't open '%s'", filePath);
			return;
		}
	}
	fseek(fp, 0, SEEK_END);
	b->size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	b->buf = (uint8 *)malloc(b->size);
	if (fread(b->buf, 1, b->size, fp) != b->size) {
		error("Cutscene::readFile(%s) read error", filePath);
	}
	fclose(fp);	
}

void Cutscene::setPalBufColors(const uint8 *pal, uint16 num) {
	uint8 *dst = (uint8 *)_palBuf;
	if (num != 0) {
		dst += 0x20;
	}
	memcpy(dst, pal, 0x20);
	_palNew = 0xFF;
}

void Cutscene::setPalette0xC0() {
	if (_palNew) {
		const uint8 *src = _palBuf;
		uint8 i = 0xC0;
		uint16 n = 0x20;
		while (n--) {
			uint8 d;
			uint8 c1 = *src++;
			uint8 c2 = *src++;
			if (c1 == 0 && c2 == 0) {
				d = 0;
			} else {
				d = 3;
			}
			Color col;
			col.b = ((c2 & 0xF) << 2) | d;
			col.g = ((c2 >> 0x4) << 2) | d;
			col.r = (c1 << 2) | d;
			_stub->setPaletteEntry(i, &col);
			++i;
		}
		_palNew = 0;
	}
}

void Cutscene::setPalette() {
	int32 delay = _stub->getTimeStamp() - _tstamp;
	int32 pause = _frameDelay * TIMER_SLICE - delay;
	if (pause > 0) {
		_stub->sleep(pause);
	}
	setPalette0xC0(); // set palette *before* blitting
	flipAndVSync();
	_tstamp = _stub->getTimeStamp();
}

void Cutscene::blitAndFlip() {
	debug(DBG_CUTSCENE, "blitAndFlip()");
	if (_clearScreen == 0) {
		_stub->copyList();
	} else {
		_stub->clearList();
	}
}

void Cutscene::flipAndVSync() {
	debug(DBG_CUTSCENE, "flipAndVSync(page0, page1)");
	_stub->blitList();
}

void Cutscene::op_markCurPos() {
	debug(DBG_CUTSCENE, "Cutscene::op_markCurPos()");
	_cmdPtrBak = _cmdPtr.pc;
	// XXX
	_frameDelay = 5;
	setPalette();
	blitAndFlip();
	_cutVarText = 0;
}

void Cutscene::op_refreshScreen() {
	debug(DBG_CUTSCENE, "Cutscene::op_refreshScreen()");
	_clearScreen = _cmdPtr.fetchByte();
	if (_clearScreen != 0) {
		blitAndFlip();
		_cutVarText = 0;
	}
}

void Cutscene::op_waitForSync() {
	debug(DBG_CUTSCENE, "Cutscene::op_waitForSync()");
	if (_creditsScene != 0) {
		// XXX
	} else {
		_frameDelay = _cmdPtr.fetchByte() * 4;
		int32 delay = _stub->getTimeStamp() - _tstamp;
		int32 pause = _frameDelay * TIMER_SLICE - delay;
		debug(DBG_CUTSCENE, "Cutscene::op_waitForSync(0x%X)", _frameDelay / 4);
		if (pause > 0) {
			_stub->sleep(pause);
			if (_inp_keysMask & 0x80) {
				_interrupted = 0xFF;
			}		
		}
		_tstamp = _stub->getTimeStamp();
	}
}

void Cutscene::drawText(int16 x, int16 y, uint8 *p, uint16 a, uint8 *page, uint8 mode) {
	error("Cutscene::drawText() unimplemented");
}

void Cutscene::initRotData(uint16 a, uint16 b, uint16 c) {
	int16 n1 = _sinTable[a];
	int16 n2 = _cosTable[a];
	int16 n3 = _sinTable[c];
	int16 n4 = _cosTable[c];
	int16 n5 = _sinTable[b];
	int16 n6 = _cosTable[b];
	_rotData[0] = ((n2 * n6) >> 8) - ((((n4 * n1) >> 8) * n5) >> 8);
	_rotData[1] = ((n1 * n6) >> 8) + ((((n4 * n2) >> 8) * n5) >> 8);
	_rotData[2] = ( n3 * n1) >> 8;
	_rotData[3] = (-n3 * n2) >> 8;
}

void Cutscene::op_drawShape0Helper(const uint8 *data, int16 x, int16 y) {
	debug(DBG_CUTSCENE, "Cutscene::op_drawShape0Helper()");
	uint8 numVertices = *data++;
	if (numVertices & 0x80) {
		Point pt;
		pt.x = READ_BE_UINT16(data) + x; data += 2;
		pt.y = READ_BE_UINT16(data) + y; data += 2;
		uint16 rx = READ_BE_UINT16(data); data += 2;
		uint16 ry = READ_BE_UINT16(data); data += 2;
		_stub->addEllipseToList(_primitiveColor, &pt, rx, ry);
	} else if (numVertices == 0) {
		Point pt;
		pt.x = READ_BE_UINT16(data) + x; data += 2;
		pt.y = READ_BE_UINT16(data) + y; data += 2;
		_stub->addPointToList(_primitiveColor, &pt);
	} else {
		Point *pt = _vertices;
		int16 ix = READ_BE_UINT16(data); data += 2;
		int16 iy = READ_BE_UINT16(data); data += 2;
		pt->x = ix + x; 
		pt->y = iy + y;
		++pt;
		int16 n = numVertices - 1;
		++numVertices;
		for (; n >= 0; --n) {
			int16 dx = (int8)*data++;
			int16 dy = (int8)*data++;
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
		_stub->addPolygonToList(_primitiveColor, _vertices, numVertices);
	}
}

void Cutscene::op_drawShape0() {
	debug(DBG_CUTSCENE, "Cutscene::op_drawShape0()");

	int16 x = 0;
	int16 y = 0;
	uint16 shapeOffset = _cmdPtr.fetchWord();
	if (shapeOffset & 0x8000) {
		x = _cmdPtr.fetchWord();
		y = _cmdPtr.fetchWord();
	}

	uint8 *shapeOffsetTable    = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x02);
	uint8 *shapeDataTable      = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x0E);
	uint8 *verticesOffsetTable = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x0A);	
	uint8 *verticesDataTable   = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x12);

	uint8 *shapeData = shapeDataTable + READ_BE_UINT16(shapeOffsetTable + (shapeOffset & 0x7FF) * 2);
	uint16 primitiveCount = READ_BE_UINT16(shapeData); shapeData += 2;

	while (primitiveCount--) {
		uint16 verticesOffset = READ_BE_UINT16(shapeData); shapeData += 2;
		uint8 *primitiveVertices = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2);
		int16 dx = 0;
		int16 dy = 0;
		if (verticesOffset & 0x8000) {
			dx = READ_BE_UINT16(shapeData); shapeData += 2;
			dy = READ_BE_UINT16(shapeData); shapeData += 2;
		}
		if (verticesOffset & 0x4000) {
			_cutUnk1 = 0xFF;
		}
		uint8 color = *shapeData++;
		if (_clearScreen == 0) {
			color += 0x10; // 2nd pal buf
		}
		_primitiveColor = 0xC0 + color;
		op_drawShape0Helper(primitiveVertices, x + dx, y + dy);
	}
	if (_clearScreen != 0) {
		_stub->saveList();
	}
}

void Cutscene::op_setPalette() {
	debug(DBG_CUTSCENE, "Cutscene::op_setPalette()");
	uint8 num = _cmdPtr.fetchByte();
	uint8 palNum = _cmdPtr.fetchByte();

	uint16 off = READ_BE_UINT16(_polBuf.buf + 6);
	const uint8 *p = _polBuf.buf + off + num * 32;
	setPalBufColors(p, palNum ^ 1);
	if (_creditsScene != 0) {
		_palBuf[0x20] = 0x0F;
		_palBuf[0x21] = 0xFF;
	}
}

void Cutscene::op_drawStringAtBottom() {
	debug(DBG_CUTSCENE, "Cutscene::op_drawStringAtBottom()");
	/*uint16 strId =*/ _cmdPtr.fetchWord();
	if (_creditsScene == 0) {
		// XXX
	}
}

void Cutscene::op_nop() {
	debug(DBG_CUTSCENE, "Cutscene::op_nop()");
}

void Cutscene::op_skip3() {
	debug(DBG_CUTSCENE, "Cutscene::op_skip3()");
	_cmdPtr.pc += 3;
}

void Cutscene::op_refreshAll() {
	debug(DBG_CUTSCENE, "Cutscene::op_refreshAll()");
	_frameDelay = 5;
	setPalette();
	blitAndFlip();
	_cutVarText = 0xFF;
	op_handleKeys();	
}

void Cutscene::op_drawShape1Helper(uint8 *data, int16 zoom, int16 b, int16 c, int16 d, int16 e, int16 f, int16 g) {
	debug(DBG_CUTSCENE, "Cutscene::op_drawShape1Helper(%d, %d, %d, %d, %d, %d, %d)", zoom, b, c, d, e, f, g);
	uint8 numVertices = *data++;
	if (numVertices & 0x80) {
		int16 x, y;
		Point *pt = _vertices;
		Point pr[2];

		cut_shape_cur_x = b + READ_BE_UINT16(data); data += 2;
		cut_shape_cur_y = c + READ_BE_UINT16(data); data += 2;
		x = READ_BE_UINT16(data); data += 2; // ellipse rx
		y = READ_BE_UINT16(data); data += 2; // ellipse ry
		cut_shape_cur_x16 = 0;
		cut_shape_cur_y16 = 0;
		pr[0].x =  0;
		pr[0].y = -y;
		pr[1].x = -x;
		pr[1].y =  y;

		if (cut_shape_count == 0) {
			f -= ((((cut_shape_ix - cut_shape_ox) * zoom) * 128) + 0x8000) >> 16;
			g -= ((((cut_shape_iy - cut_shape_oy) * zoom) * 128) + 0x8000) >> 16;
			pt->x = f;
			pt->y = g;
			++pt;
			cut_shape_cur_x16 = f << 16;
			cut_shape_cur_y16 = g << 16;
		} else {	
			cut_shape_cur_x16 = cut_shape_prev_x16 + ((cut_shape_cur_x - cut_shape_prev_x) * zoom) * 128;
			pt->x = (cut_shape_cur_x16 + 0x8000) >> 16;
			cut_shape_cur_y16 = cut_shape_prev_y16 + ((cut_shape_cur_y - cut_shape_prev_y) * zoom) * 128;
			pt->y = (cut_shape_cur_y16 + 0x8000) >> 16;
			++pt;
		}
		for (int i = 0; i < 2; ++i) {
			cut_shape_cur_x += pr[i].x;
			cut_shape_cur_x16 += pr[i].x * zoom * 128;
			pt->x = (cut_shape_cur_x16 + 0x8000) >> 16;
			cut_shape_cur_y += pr[i].y;
			cut_shape_cur_y16 += pr[i].y * zoom * 128;
			pt->y = (cut_shape_cur_y16 + 0x8000) >> 16;
			++pt;
		}
		cut_shape_prev_x = cut_shape_cur_x;
		cut_shape_prev_y = cut_shape_cur_y;
		cut_shape_prev_x16 = cut_shape_cur_x16;
		cut_shape_prev_y16 = cut_shape_cur_y16;
		Point po;
		po.x = _vertices[0].x + d + cut_shape_ix;
		po.y = _vertices[0].y + e + cut_shape_iy;
		int16 rx = _vertices[0].x - _vertices[2].x;
		int16 ry = _vertices[0].y - _vertices[1].y;
		_stub->addEllipseToList(_primitiveColor, &po, rx, ry);
	} else if (numVertices == 0) {
		Point pt;
 		pt.x = cut_shape_cur_x = b + READ_BE_UINT16(data); data += 2;
	 	pt.y = cut_shape_cur_y = c + READ_BE_UINT16(data); data += 2;
 		if (cut_shape_count == 0) {
			f -= ((((cut_shape_ix - pt.x) * zoom) * 128) + 0x8000) >> 16;
			g -= ((((cut_shape_iy - pt.y) * zoom) * 128) + 0x8000) >> 16;
			pt.x = f + cut_shape_ix + d;
			pt.y = g + cut_shape_iy + e;
			cut_shape_cur_x16 = f << 16;
			cut_shape_cur_y16 = g << 16;	
		} else {
			cut_shape_cur_x16 = cut_shape_prev_x16 + ((pt.x - cut_shape_prev_x) * zoom) * 128;
			cut_shape_cur_y16 = cut_shape_prev_y16 + ((pt.y - cut_shape_prev_y) * zoom) * 128;
			pt.x = ((cut_shape_cur_x16 + 0x8000) >> 16) + cut_shape_ix + d;
			pt.y = ((cut_shape_cur_y16 + 0x8000) >> 16) + cut_shape_iy + e;
		}
		cut_shape_prev_x = cut_shape_cur_x;
		cut_shape_prev_y = cut_shape_cur_y;
		cut_shape_prev_x16 = cut_shape_cur_x16;
		cut_shape_prev_y16 = cut_shape_cur_y16;
		_stub->addPointToList(_primitiveColor, &pt);
	} else {
		Point *pt = _vertices;
		int16 ix, iy;
		cut_shape_cur_x = ix = READ_BE_UINT16(data) + b; data += 2;
		cut_shape_cur_y = iy = READ_BE_UINT16(data) + c; data += 2;
		if (cut_shape_count == 0) {
			f -= ((((cut_shape_ix - cut_shape_ox) * zoom) * 128) + 0x8000) >> 16;
			g -= ((((cut_shape_iy - cut_shape_oy) * zoom) * 128) + 0x8000) >> 16;
			pt->x = f + cut_shape_ix + d;
			pt->y = g + cut_shape_iy + e;
			++pt;
			cut_shape_cur_x16 = f << 16;
			cut_shape_cur_y16 = g << 16;
		} else {
			cut_shape_cur_x16 = cut_shape_prev_x16 + ((cut_shape_cur_x - cut_shape_prev_x) * zoom) * 128;
			cut_shape_cur_y16 = cut_shape_prev_y16 + ((cut_shape_cur_y - cut_shape_prev_y) * zoom) * 128;
			pt->x = ix = ((cut_shape_cur_x16 + 0x8000) >> 16) + cut_shape_ix + d;
			pt->y = iy = ((cut_shape_cur_y16 + 0x8000) >> 16) + cut_shape_iy + e;
			++pt;
		}
		int16 n = numVertices - 1;
		++numVertices;
		int16 sx = 0;
		for (; n >= 0; --n) {
			ix = (int8)(*data++) + sx;
			iy = (int8)(*data++);
			if (iy == 0 && n != 0 && *(data + 1) == 0) {
				sx = ix;
				--numVertices;
			} else {
				sx = 0;
				cut_shape_cur_x += ix;
				cut_shape_cur_y += iy;
				cut_shape_cur_x16 += ix * zoom * 128;
				cut_shape_cur_y16 += iy * zoom * 128;
				pt->x = ((cut_shape_cur_x16 + 0x8000) >> 16) + cut_shape_ix + d;	
				pt->y = ((cut_shape_cur_y16 + 0x8000) >> 16) + cut_shape_iy + e;
				++pt;
			}
		}
		cut_shape_prev_x = cut_shape_cur_x;
		cut_shape_prev_y = cut_shape_cur_y;
		cut_shape_prev_x16 = cut_shape_cur_x16;
		cut_shape_prev_y16 = cut_shape_cur_y16;
		_stub->addPolygonToList(_primitiveColor, _vertices, numVertices);
	}
}

void Cutscene::op_drawShape1() {
	debug(DBG_CUTSCENE, "Cutscene::op_drawShape1()");
	
	cut_shape_unk_x = 0;
	cut_shape_unk_y = 0;
	cut_shape_count = 0;
	
	int16 x = 0;
	int16 y = 0;
	uint16 shapeOffset = _cmdPtr.fetchWord();
	if (shapeOffset & 0x8000) {
		x = _cmdPtr.fetchWord();
		y = _cmdPtr.fetchWord();
	}

	uint16 zoom = _cmdPtr.fetchWord() + 512;
	cut_shape_ix = _cmdPtr.fetchByte();
	cut_shape_iy = _cmdPtr.fetchByte();

	initRotData(0, 180, 90); // so we can use drawShapeScaleRotate
	
	uint8 *shapeOffsetTable    = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x02);
	uint8 *shapeDataTable      = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x0E);
	uint8 *verticesOffsetTable = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x0A);	
	uint8 *verticesDataTable   = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x12);

	uint8 *shapeData = shapeDataTable + READ_BE_UINT16(shapeOffsetTable + (shapeOffset & 0x7FF) * 2);
	uint16 primitiveCount = READ_BE_UINT16(shapeData); shapeData += 2;

	if (primitiveCount != 0) {
		uint16 verticesOffset = READ_BE_UINT16(shapeData);
		int16 dx = 0;
		int16 dy = 0;
		if (verticesOffset & 0x8000) {
			dx = READ_BE_UINT16(shapeData + 2);
			dy = READ_BE_UINT16(shapeData + 4);
		}
		uint8 *p = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2) + 1;
		cut_shape_ox = READ_BE_UINT16(p) + dx; p += 2;
		cut_shape_oy = READ_BE_UINT16(p) + dy; p += 2;
		while (primitiveCount--) {
//			_unkVar8 = 0;
			verticesOffset = READ_BE_UINT16(shapeData); shapeData += 2;
			p = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2);
			dx = 0;
			dy = 0;
			if (verticesOffset & 0x8000) {
				dx = READ_BE_UINT16(shapeData); shapeData += 2;
				dy = READ_BE_UINT16(shapeData); shapeData += 2;
			}
			if (verticesOffset & 0x4000) {
				_cutUnk1 = 0xFF;
			}
			uint8 color = *shapeData++;
			if (_clearScreen == 0) {
				color += 0x10; // 2nd pal buf
			}
			_primitiveColor = 0xC0 + color;
			op_drawShape2Helper(p, zoom, dx, dy, x, y, 0, 0);
			++cut_shape_count;
		}	
	}
}

void Cutscene::op_drawShape2Helper(uint8 *data, int16 zoom, int16 b, int16 c, int16 d, int16 e, int16 f, int16 g) {
	assert(f == 0 && g == 0);
	// b == shape primitive dx, c == shape primitive dy
	// d == shape x, e == shape y
	assert(f == 0 && g == 0);
	debug(DBG_CUTSCENE, "Cutscene::op_drawShape2Helper(%d, %d, %d, %d, %d, %d, %d)", zoom, b, c, d, e, f, g);
	uint8 numVertices = *data++;
	if (numVertices & 0x80) {
		int16 x, y, ix, iy;
		Point pr[2];
		Point *pt = _vertices;
		
		cut_shape_cur_x = ix = b + READ_BE_UINT16(data); data += 2;
		cut_shape_cur_y = iy = c + READ_BE_UINT16(data); data += 2;

		x = READ_BE_UINT16(data); data += 2;
		y = READ_BE_UINT16(data); data += 2;

		cut_shape_cur_x16 = cut_shape_ix - ix;	
		cut_shape_cur_y16 = cut_shape_iy - iy;

		cut_shape_ox = cut_shape_cur_x = cut_shape_ix + ((cut_shape_cur_x16 * _rotData[0] + cut_shape_cur_y16 * _rotData[1]) >> 8);
		cut_shape_oy = cut_shape_cur_y = cut_shape_iy + ((cut_shape_cur_x16 * _rotData[2] + cut_shape_cur_y16 * _rotData[3]) >> 8);

		pr[0].x =  0;
		pr[0].y = -y;
		pr[1].x = -x;
		pr[1].y =  y;
	
		if (cut_shape_count == 0) {
			f -= ((cut_shape_ix - cut_shape_cur_x) * zoom * 128 + 0x8000) >> 16;
			g -= ((cut_shape_iy - cut_shape_cur_y) * zoom * 128 + 0x8000) >> 16;
			pt->x = f;
			pt->y = g;
			++pt;
			cut_shape_cur_x16 = f << 16;
			cut_shape_cur_y16 = g << 16;
		} else {
			cut_shape_cur_x16 = cut_shape_prev_x16 + (cut_shape_cur_x - cut_shape_prev_x) * zoom * 128;
			cut_shape_cur_y16 = cut_shape_prev_y16 + (cut_shape_cur_y - cut_shape_prev_y) * zoom * 128;
			pt->x = (cut_shape_cur_x16 + 0x8000) >> 16;
			pt->y = (cut_shape_cur_y16 + 0x8000) >> 16;
			++pt;
		}
		for (int i = 0; i < 2; ++i) {
			cut_shape_cur_x += pr[i].x;
			cut_shape_cur_x16 += pr[i].x * zoom * 128;
			pt->x = (cut_shape_cur_x16 + 0x8000) >> 16;
			cut_shape_cur_y += pr[i].y;
			cut_shape_cur_y16 += pr[i].y * zoom * 128;
			pt->y = (cut_shape_cur_y16 + 0x8000) >> 16;				
			++pt;
		}
		cut_shape_prev_x = cut_shape_cur_x;
		cut_shape_prev_y = cut_shape_cur_y;
		cut_shape_prev_x16 = cut_shape_cur_x16;
		cut_shape_prev_y16 = cut_shape_cur_y16;
		Point po;
		po.x = _vertices[0].x + d + cut_shape_ix;
		po.y = _vertices[0].y + e + cut_shape_iy;
		int16 rx = _vertices[0].x - _vertices[2].x;
		int16 ry = _vertices[0].y - _vertices[1].y;		
		_stub->addEllipseToList(_primitiveColor, &po, rx, ry);
	} else if (numVertices == 0) {
		Point pt;
		pt.x = b + READ_BE_UINT16(data); data += 2;
		pt.y = c + READ_BE_UINT16(data); data += 2;
	
		cut_shape_cur_x16 = cut_shape_ix - pt.x;
		cut_shape_cur_y16 = cut_shape_iy - pt.y;
		cut_shape_cur_x = cut_shape_ix + ((_rotData[0] * cut_shape_cur_x16 + _rotData[1] * cut_shape_cur_y16) >> 8);		
		cut_shape_cur_y = cut_shape_iy + ((_rotData[2] * cut_shape_cur_x16 + _rotData[3] * cut_shape_cur_y16) >> 8);

		if (cut_shape_count != 0) {		
			cut_shape_cur_x16 = cut_shape_prev_x16 + (cut_shape_cur_x - cut_shape_prev_x) * zoom * 128;
			pt.x = ((cut_shape_cur_x16 + 0x8000) >> 16) + cut_shape_ix + d;
			cut_shape_cur_y16 = cut_shape_prev_y16 + (cut_shape_cur_y - cut_shape_prev_y) * zoom * 128;
			pt.y = ((cut_shape_cur_y16 + 0x8000) >> 16) + cut_shape_iy + e;
		} else {
			f -= (((cut_shape_ix - cut_shape_cur_x) * zoom * 128) + 0x8000) >> 16;
			g -= (((cut_shape_iy - cut_shape_cur_y) * zoom * 128) + 0x8000) >> 16;
			cut_shape_cur_x16 = f << 16;
			cut_shape_cur_y16 = g << 16;
			pt.x = f + cut_shape_ix + d;
			pt.y = g + cut_shape_iy + e;
		}		
		cut_shape_prev_x = cut_shape_cur_x;
		cut_shape_prev_y = cut_shape_cur_y;
		cut_shape_prev_x16 = cut_shape_cur_x16;
		cut_shape_prev_y16 = cut_shape_cur_y16;
		_stub->addPointToList(_primitiveColor, &pt);			
	} else {
		int16 x, y, a;
		Point tempVertices[40];
		cut_shape_cur_x = b + READ_BE_UINT16(data); data += 2;
		x = cut_shape_cur_x;
		cut_shape_cur_y = c + READ_BE_UINT16(data); data += 2;
		y = cut_shape_cur_y;
		cut_shape_cur_x16 = cut_shape_ix - x;
		cut_shape_cur_y16 = cut_shape_iy - y;

		a = cut_shape_ix + ((_rotData[0] * cut_shape_cur_x16 + _rotData[1] * cut_shape_cur_y16) >> 8);
		if (cut_shape_count == 0) {
			cut_shape_ox = a;
		}
		cut_shape_cur_x = cut_shape_unk_x2 = a;
		a = cut_shape_iy + ((_rotData[2] * cut_shape_cur_x16 + _rotData[3] * cut_shape_cur_y16) >> 8);
		if (cut_shape_count == 0) {
			cut_shape_oy = a;
		}
		cut_shape_cur_y = cut_shape_unk_y2 = a;

		int16 ix = x;
		int16 iy = y;
		Point *pt2 = tempVertices;
		cut_shape_unk_x = 0;
		for (int16 n = numVertices - 1; n >= 0; --n) {
			x = (int8)(*data++) + cut_shape_unk_x;
			y = (int8)(*data++);
			if (y == 0 && n != 0 && *(data + 1) == 0) {
				cut_shape_unk_x = x;
				--numVertices;
			} else {
				ix += x;
				iy += y;
				cut_shape_unk_x = 0;
				cut_shape_cur_x16 = cut_shape_ix - ix;
				cut_shape_cur_y16 = cut_shape_iy - iy;
				a = cut_shape_ix + ((_rotData[0] * cut_shape_cur_x16 + _rotData[1] * cut_shape_cur_y16) >> 8);				
				pt2->x = a - cut_shape_unk_x2;
				cut_shape_unk_x2 = a;
				a = cut_shape_iy + ((_rotData[2] * cut_shape_cur_x16 + _rotData[3] * cut_shape_cur_y16) >> 8);
				pt2->y = a - cut_shape_unk_y2;
				cut_shape_unk_y2 = a;
				++pt2;
			}
		}
		Point *pt = _vertices;
		if (cut_shape_count == 0) {
			ix = cut_shape_ox;
			iy = cut_shape_oy;
			f -= (((cut_shape_ix - ix) * zoom * 128) + 0x8000) >> 16;
			g -= (((cut_shape_iy - iy) * zoom * 128) + 0x8000) >> 16;
			pt->x = f + cut_shape_ix + d;
			pt->y = g + cut_shape_iy + e;
			++pt;
			cut_shape_cur_x16 = f << 16;
			cut_shape_cur_y16 = g << 16;
		} else {		
			cut_shape_cur_x16 = cut_shape_prev_x16 + ((cut_shape_cur_x - cut_shape_prev_x) * zoom * 128);
			pt->x = cut_shape_ix + d + ((cut_shape_cur_x16 + 0x8000) >> 16);
			cut_shape_cur_y16 = cut_shape_prev_y16 + ((cut_shape_cur_y - cut_shape_prev_y) * zoom * 128);
			pt->y = cut_shape_iy + e + ((cut_shape_cur_y16 + 0x8000) >> 16);
			++pt;
		}
		for (int i = 0; i < numVertices; ++i) {
			cut_shape_cur_x += tempVertices[i].x;
			cut_shape_cur_x16 += tempVertices[i].x * zoom * 128;
			pt->x = d + cut_shape_ix + ((cut_shape_cur_x16 + 0x8000) >> 16);
			cut_shape_cur_y += tempVertices[i].y;
			cut_shape_cur_y16 += tempVertices[i].y * zoom * 128;
			pt->y = e + cut_shape_iy + ((cut_shape_cur_y16 + 0x8000) >> 16);
			++pt;
		}
		cut_shape_prev_x = cut_shape_cur_x;
		cut_shape_prev_y = cut_shape_cur_y;
		cut_shape_prev_x16 = cut_shape_cur_x16;
		cut_shape_prev_y16 = cut_shape_cur_y16;
		_stub->addPolygonToList(_primitiveColor, _vertices, numVertices + 1);
	}
}

void Cutscene::op_drawShape2() {
	debug(DBG_CUTSCENE, "Cutscene::op_drawShape2()");
	
	cut_shape_unk_x = 0;
	cut_shape_unk_y = 0;
	cut_shape_count = 0;

	int16 x = 0;
	int16 y = 0;
	uint16 shapeOffset = _cmdPtr.fetchWord();
	if (shapeOffset & 0x8000) {
		x = _cmdPtr.fetchWord();
		y = _cmdPtr.fetchWord();
	}

	uint16 zoom = 512;
	if (shapeOffset & 0x4000) {
		zoom += _cmdPtr.fetchWord();
	}
	cut_shape_ix = _cmdPtr.fetchByte();
	cut_shape_iy = _cmdPtr.fetchByte();

	uint16 r1, r2, r3;
	r1 = _cmdPtr.fetchWord();
	r2 = 180;
	if (shapeOffset & 0x2000) {
		r2 = _cmdPtr.fetchWord();
	}
	r3 = 90;
	if (shapeOffset & 0x1000) {
		r3 = _cmdPtr.fetchWord();
	}
	initRotData(r1, r2, r3);

	uint8 *shapeOffsetTable    = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x02);
	uint8 *shapeDataTable      = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x0E);
	uint8 *verticesOffsetTable = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x0A);	
	uint8 *verticesDataTable   = _polBuf.buf + READ_BE_UINT16(_polBuf.buf + 0x12);

	uint8 *shapeData = shapeDataTable + READ_BE_UINT16(shapeOffsetTable + (shapeOffset & 0x7FF) * 2);
	uint16 primitiveCount = READ_BE_UINT16(shapeData); shapeData += 2;

	while (primitiveCount--) {
		uint16 verticesOffset = READ_BE_UINT16(shapeData); shapeData += 2;
		uint8 *p = verticesDataTable + READ_BE_UINT16(verticesOffsetTable + (verticesOffset & 0x3FFF) * 2);
		int16 dx = 0;
		int16 dy = 0;
		if (verticesOffset & 0x8000) {
			dx = READ_BE_UINT16(shapeData); shapeData += 2;
			dy = READ_BE_UINT16(shapeData); shapeData += 2;
		}
		if (verticesOffset & 0x4000) {
			_cutUnk1 = 0xFF;
		}
		uint8 color = *shapeData++;
		if (_clearScreen == 0) {
			color += 0x10; // 2nd pal buf
		}
		_primitiveColor = 0xC0 + color;
		op_drawShape2Helper(p, zoom, dx, dy, x, y, 0, 0);
		++cut_shape_count;
	}			
}

void Cutscene::op_drawCreditsText() {
	debug(DBG_CUTSCENE, "Cutscene::op_drawCreditsText()");
	_cutVarText = 0xFF;
	// XXX
	_frameDelay = 0xA;
	// XXX
}

void Cutscene::op_drawStringAtPos() {
	debug(DBG_CUTSCENE, "Cutscene::op_drawStringAtPos()");
	uint16 strId = _cmdPtr.fetchWord();
	if (strId != 0xFFFF) {
		/*int16 x =*/ (int8)_cmdPtr.fetchByte(); // * 8;
		/*int16 y =*/ (int8)_cmdPtr.fetchByte(); // * 8;
//		uint16 var_6 = (strId >> 0xC) + 0xD0;
//		uint16 offsetIndex = (strId & 0xFFF) * 2;
		// XXX
	}
}

void Cutscene::op_handleKeys() {
	debug(DBG_CUTSCENE, "Cutscene::op_handleKeys()");
	uint8 a = _cmdPtr.fetchByte();
	if (a != 0xFF) {
		bool b;
		if (a == 1) {
			b = (_inp_keysMask & 1) != 0;
		} else if (a == 2) {
			b = (_inp_keysMask & 2) != 0;
		} else if (a == 4) {
			b = (_inp_keysMask & 4) != 0;
		} else if (a == 8) {
			b = (_inp_keysMask & 8) != 0;
		} else if (a == 0x80) {
			b = (_inp_keysMask & (0x40 | 0x20 | 0x10)) != 0;
		} else {
			b = true;
		}
		if (!b) {
			_cmdPtr.pc += 2;
			op_handleKeys();
		} else {
			_inp_keysMask = 0;
			int16 n = _cmdPtr.fetchWord();
			if (n < 0) {
				n = -n;
				--n;
				if (_cutVarKey == 0) {
					return;
				}
				if (_cutVarKey != n) {
					_cmdPtr.pc = _cmdPtrBak;
					return;
				}
				_cutVarKey = 0;
				--n;
				_cmdPtr.pc = _cmdBuf.buf;
				n = READ_BE_UINT16(_cmdPtr.pc + n * 2 + 2);
			}
			_cmdPtr.pc = _cmdPtrBak = _cmdBuf.buf + n + _startOffset;
		}
	}
}

int Cutscene::load() {
	debug(DBG_CUTSCENE, "Cutscene::start() _cutId = %d", _cutId);
	int offset = 0;
	if (_cutId != 0xFFFF) {
		uint16 cutName = _offsetsTable[_cutId * 2 + 0];
		offset = _offsetsTable[_cutId * 2 + 1];
		assert(cutName != 0xFFFF);
		_cutName = _namesTable[cutName & 0xFF];
	}
	debug(DBG_CUTSCENE, "starting cutscene '%s' offset = %d", _cutName, offset);
	readFile(_cutName, "pol", &_polBuf);
	readFile(_cutName, "cmd", &_cmdBuf);
	return offset;
}

void Cutscene::init(SystemStub *stub, const char *dataDir) {
	_stub = stub;
	_dataDir = dataDir;
}

void Cutscene::main(uint16 index) {
	_creditsScene = 0;
	_frameDelay = 5;
	_tstamp = _stub->getTimeStamp();

	int n = 0x20;
	int i = 0xC0;
	Color c;
	c.r = c.g = c.b = 0;
	while (n--) {
		_stub->setPaletteEntry(i, &c);
		++i;
	}
//	if (_cutId != 0x4A) {
//		loadMusic();
//	}
	_palNew = 0;
	uint8 *p = _cmdBuf.buf;
	if (index != 0) {
		index = READ_BE_UINT16(p + (index + 1) * 2);
	}
	_startOffset = (READ_BE_UINT16(p) + 1) * 2;
	_cmdPtrBak = p + _startOffset + index;
	debug(DBG_CUTSCENE, "_startOffset = %d offset = %d", _startOffset, index);
	_cmdPtr.pc = _cmdPtrBak;

	play();
}

void Cutscene::play() {
	while (1) {
		uint8 op = _cmdPtr.fetchByte();
		debug(DBG_CUTSCENE, "Cutscene::play() opcode = 0x%X (%d) ip=0x%X/0x%X", op, (op >> 2), _cmdPtr.pc - _cmdBuf.buf, _cmdBuf.size);
		if (op & 0x80) {
			break;
		}
		op >>= 2;
		if (op >= NUM_OPCODES) {
			error("Invalid cutscene opcode = 0x%02X", op);
		}
		(this->*_opTable[op])();
		if (_inp_keysMask & 0x80) {
			_interrupted = 0xFF;
		}
		_stub->processEvents();
	}
}
