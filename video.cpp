/* REminiscence - Flashback interpreter
 * Copyright (C) 2005-2011 Gregory Montoir
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "resource.h"
#include "systemstub.h"
#include "unpack.h"
#include "video.h"


Video::Video(Resource *res, SystemStub *stub)
	: _res(res), _stub(stub) {
	_frontLayer = (uint8 *)malloc(GAMESCREEN_W * GAMESCREEN_H);
	memset(_frontLayer, 0, GAMESCREEN_W * GAMESCREEN_H);
	_backLayer = (uint8 *)malloc(GAMESCREEN_W * GAMESCREEN_H);
	memset(_backLayer, 0, GAMESCREEN_W * GAMESCREEN_H);
	_tempLayer = (uint8 *)malloc(GAMESCREEN_W * GAMESCREEN_H);
	memset(_tempLayer, 0, GAMESCREEN_W * GAMESCREEN_H);
	_tempLayer2 = (uint8 *)malloc(GAMESCREEN_W * GAMESCREEN_H);
	memset(_tempLayer2, 0, GAMESCREEN_W * GAMESCREEN_H);
	_screenBlocks = (uint8 *)malloc((GAMESCREEN_W / SCREENBLOCK_W) * (GAMESCREEN_H / SCREENBLOCK_H));
	memset(_screenBlocks, 0, (GAMESCREEN_W / SCREENBLOCK_W) * (GAMESCREEN_H / SCREENBLOCK_H));
	_fullRefresh = true;
	_shakeOffset = 0;
	_charFrontColor = 0;
	_charTransparentColor = 0;
	_charShadowColor = 0;
}

Video::~Video() {
	free(_frontLayer);
	free(_backLayer);
	free(_tempLayer);
	free(_tempLayer2);
	free(_screenBlocks);
}

void Video::markBlockAsDirty(int16 x, int16 y, uint16 w, uint16 h) {
	debug(DBG_VIDEO, "Video::markBlockAsDirty(%d, %d, %d, %d)", x, y, w, h);
	assert(x >= 0 && x + w <= GAMESCREEN_W && y >= 0 && y + h <= GAMESCREEN_H);
	int bx1 = x / SCREENBLOCK_W;
	int by1 = y / SCREENBLOCK_H;
	int bx2 = (x + w - 1) / SCREENBLOCK_W;
	int by2 = (y + h - 1) / SCREENBLOCK_H;
	assert(bx2 < GAMESCREEN_W / SCREENBLOCK_W && by2 < GAMESCREEN_H / SCREENBLOCK_H);
	for (; by1 <= by2; ++by1) {
		for (int i = bx1; i <= bx2; ++i) {
			_screenBlocks[by1 * (GAMESCREEN_W / SCREENBLOCK_W) + i] = 2;
		}
	}
}

void Video::updateScreen() {
	debug(DBG_VIDEO, "Video::updateScreen()");
//	_fullRefresh = true;
	if (_fullRefresh) {
		_stub->copyRect(0, 0, Video::GAMESCREEN_W, Video::GAMESCREEN_H, _frontLayer, 256);
		_stub->updateScreen(_shakeOffset);
		_fullRefresh = false;
	} else {
		int i, j;
		int count = 0;
		uint8 *p = _screenBlocks;
		for (j = 0; j < GAMESCREEN_H / SCREENBLOCK_H; ++j) {
			uint16 nh = 0;
			for (i = 0; i < GAMESCREEN_W / SCREENBLOCK_W; ++i) {
				if (p[i] != 0) {
					--p[i];
					++nh;
				} else if (nh != 0) {
					int16 x = (i - nh) * SCREENBLOCK_W;
					_stub->copyRect(x, j * SCREENBLOCK_H, nh * SCREENBLOCK_W, SCREENBLOCK_H, _frontLayer, 256);
					nh = 0;
					++count;
				}
			}
			if (nh != 0) {
				int16 x = (i - nh) * SCREENBLOCK_W;
				_stub->copyRect(x, j * SCREENBLOCK_H, nh * SCREENBLOCK_W, SCREENBLOCK_H, _frontLayer, 256);
				++count;
			}
			p += GAMESCREEN_W / SCREENBLOCK_W;
		}
		if (count != 0) {
			_stub->updateScreen(_shakeOffset);
		}
	}
	if (_shakeOffset != 0) {
		_shakeOffset = 0;
		_fullRefresh = true;
	}
}

void Video::fullRefresh() {
	debug(DBG_VIDEO, "Video::fullRefresh()");
	_fullRefresh = true;
	memset(_screenBlocks, 0, (GAMESCREEN_W / SCREENBLOCK_W) * (GAMESCREEN_H / SCREENBLOCK_H));
}

void Video::fadeOut() {
	debug(DBG_VIDEO, "Video::fadeOut()");
	_stub->fadeScreen();
//	fadeOutPalette();
}

void Video::fadeOutPalette() {
	for (int step = 16; step >= 0; --step) {
		for (int c = 0; c < 256; ++c) {
			Color col;
			_stub->getPaletteEntry(c, &col);
			col.r = col.r * step >> 4;
			col.g = col.g * step >> 4;
			col.b = col.b * step >> 4;
			_stub->setPaletteEntry(c, &col);
		}
		fullRefresh();
		updateScreen();
		_stub->sleep(50);
	}
}

void Video::setPaletteSlotBE(int palSlot, int palNum) {
	debug(DBG_VIDEO, "Video::setPaletteSlotBE()");
	const uint8 *p = _res->_pal + palNum * 0x20;
	for (int i = 0; i < 16; ++i) {
		uint16 color = READ_BE_UINT16(p); p += 2;
		uint8 t = (color == 0) ? 0 : 3;
		Color c;
		c.r = ((color & 0x00F) << 2) | t;
		c.g = ((color & 0x0F0) >> 2) | t;
		c.b = ((color & 0xF00) >> 6) | t;
		_stub->setPaletteEntry(palSlot * 0x10 + i, &c);
	}
}

void Video::setPaletteSlotLE(int palSlot, const uint8 *palData) {
	debug(DBG_VIDEO, "Video::setPaletteSlotLE()");
	for (int i = 0; i < 16; ++i) {
		uint16 color = READ_LE_UINT16(palData); palData += 2;
		Color c;
		c.b = (color & 0x00F) << 2;
		c.g = (color & 0x0F0) >> 2;
		c.r = (color & 0xF00) >> 6;
		_stub->setPaletteEntry(palSlot * 0x10 + i, &c);
	}
}

void Video::setTextPalette() {
	debug(DBG_VIDEO, "Video::setTextPalette()");
	setPaletteSlotLE(0xE, _textPal);
}

void Video::setPalette0xF() {
	debug(DBG_VIDEO, "Video::setPalette0xF()");
	const uint8 *p = _palSlot0xF;
	for (int i = 0; i < 16; ++i) {
		Color c;
		c.r = *p++ >> 2;
		c.g = *p++ >> 2;
		c.b = *p++ >> 2;
		_stub->setPaletteEntry(0xF0 + i, &c);
	}
}

static void PC_decodeMapHelper(int sz, const uint8 *src, uint8 *dst) {
	const uint8 *end = src + sz;
	while (src < end) {
		int16 code = (int8)*src++;
		if (code < 0) {
			const int len = 1 - code;
			memset(dst, *src++, len);
			dst += len;
		} else {
			++code;
			memcpy(dst, src, code);
			src += code;
			dst += code;
		}
	}
}

void Video::PC_decodeMap(int level, int room) {
	debug(DBG_VIDEO, "Video::PC_decodeMap(%d)", room);
	assert(room < 0x40);
	int32 off = READ_LE_UINT32(_res->_map + room * 6);
	if (off == 0) {
		error("Invalid room %d", room);
	}
	bool packed = true;
	if (off < 0) {
		off = -off;
		packed = false;
	}
	const uint8 *p = _res->_map + off;
	_mapPalSlot1 = *p++;
	_mapPalSlot2 = *p++;
	_mapPalSlot3 = *p++;
	_mapPalSlot4 = *p++;
	if (level == 4 && room == 60) {
		// workaround for wrong palette colors (fire)
		_mapPalSlot4 = 5;
	}
	if (packed) {
		uint8 *vid = _frontLayer;
		for (int i = 0; i < 4; ++i) {
			const int sz = READ_LE_UINT16(p); p += 2;
			PC_decodeMapHelper(sz, p, _res->_memBuf); p += sz;
			memcpy(vid, _res->_memBuf, 256 * 56);
			vid += 256 * 56;
		}
	} else {
		for (int i = 0; i < 4; ++i) {
			for (int y = 0; y < 224; ++y) {
				for (int x = 0; x < 64; ++x) {
					_frontLayer[i + x * 4 + 256 * y] = p[256 * 56 * i + x + 64 * y];
				}
			}
		}
	}
	memcpy(_backLayer, _frontLayer, Video::GAMESCREEN_W * Video::GAMESCREEN_H);
}

void Video::PC_setLevelPalettes() {
	debug(DBG_VIDEO, "Video::PC_setLevelPalettes()");
	if (_unkPalSlot2 == 0) {
		_unkPalSlot2 = _mapPalSlot3;
	}
	if (_unkPalSlot1 == 0) {
		_unkPalSlot1 = _mapPalSlot3;
	}
	setPaletteSlotBE(0x0, _mapPalSlot1);
	setPaletteSlotBE(0x1, _mapPalSlot2);
	setPaletteSlotBE(0x2, _mapPalSlot3);
	setPaletteSlotBE(0x3, _mapPalSlot4);
	if (_unkPalSlot1 == _mapPalSlot3) {
		setPaletteSlotLE(4, _conradPal1);
	} else {
		setPaletteSlotLE(4, _conradPal2);
	}
	// slot 5 is monster palette
	setPaletteSlotBE(0x8, _mapPalSlot1);
	setPaletteSlotBE(0x9, _mapPalSlot2);
	setPaletteSlotBE(0xA, _unkPalSlot2);
	setPaletteSlotBE(0xB, _mapPalSlot4);
	// slots 0xC and 0xD are cutscene palettes
	setTextPalette();
}

static void AMIGA_blit4pNxN(uint8 *dst, int x0, int y0, int w, int h, uint8 *src, uint8 *mask, int size) {
	dst += y0 * 256 + x0;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w * 2; ++x) {
			for (int i = 0; i < 8; ++i) {
				const int c_mask = 1 << (7 - i);
				int color = 0;
				for (int j = 0; j < 4; ++j) {
					if (mask[j * size] & c_mask) {
						color |= 1 << j;
					}
				}
				if (*src & c_mask) {
					const int px = x0 + 8 * x + i;
					const int py = y0 + y;
					if (px >= 0 && px < 256 && py >= 0 && py < 224) {
						dst[8 * x + i] = color;
					}
				}
			}
			++src;
			++mask;
		}
		dst += 256;
	}
}

static void AMIGA_decodeSgdHelper(uint8 *dst, const uint8 *src) {
	int code = READ_BE_UINT16(src) & 0x7FFF; src += 2;
	const uint8 *end = src + code;
	do {
		code = *src++;
		if ((code & 0x80) == 0) {
			++code;
			memcpy(dst, src, code);
			src += code;
		} else {
			code = 1 - ((int8)code);
			memset(dst, *src, code);
			++src;
		}
		dst += code;
	} while (src < end);
	assert(src == end);
}

static void AMIGA_decodeSgd(uint8 *dst, const uint8 *src, const uint8 *data) {
	int num = -1;
	uint8 buf[256 * 32];
	int count = READ_BE_UINT16(src) - 1; src += 2;
	do {
		int d2 = READ_BE_UINT16(src); src += 2;
		int d0 = READ_BE_UINT16(src); src += 2;
		int d1 = READ_BE_UINT16(src); src += 2;
		if (d2 != 0xFFFF) {
			d2 &= ~(1 << 15);
			const int offset = READ_BE_UINT32(data + d2 * 4);
			if (offset < 0) {
				const uint8 *ptr = data - offset;
				const int size = READ_BE_UINT16(ptr); ptr += 2;
				if (num != d2) {
					num = d2;
					assert(size < (int)sizeof(buf));
					memcpy(buf, ptr, size);
				}
                        } else {
				if (num != d2) {
					num = d2;
					const int size = READ_BE_UINT16(data + offset) & 0x7FFF;
					assert(size < (int)sizeof(buf));
					AMIGA_decodeSgdHelper(buf, data + offset);
				}
			}
		}
		const int w = (buf[0] + 1) >> 1;
		const int h = buf[1] + 1;
		const int planarSize = READ_BE_UINT16(buf + 2);
		AMIGA_blit4pNxN(dst, (int16)d0, (int16)d1, w, h, buf + 4, buf + 4 + planarSize, planarSize);
	} while (--count >= 0);
}

static const uint8 *AMIGA_mirrorY(const uint8 *a2) {
	static uint8 buf[32];

        a2 += 24;
	for (int j = 0; j < 4; ++j) {
		for (int i = 0; i < 8; ++i) {
			buf[31 - j * 8 - i] = *a2++;
		}
		a2 -= 16;
	}
	return buf;
}

static const uint8 *AMIGA_mirrorX(const uint8 *a2) {
	static uint8 buf[32];
	uint8 mask = 0;

	for (int i = 0; i < 32; ++i) {
		mask = 0;
		for (int bit = 0; bit < 8; ++bit) {
			if (a2[i] & (1 << bit)) {
				mask |= 1 << (7 - bit);
			}
		}
		buf[i] = mask;
	}
	return buf;
}

static void AMIGA_blit4p8x8(uint8 *dst, const uint8 *src, int colorKey) {
	for (int y = 0; y < 8; ++y) {
		for (int i = 0; i < 8; ++i) {
			const int mask = 1 << (7 - i);
			int color = 0;
			for (int bit = 0; bit < 4; ++bit) {
				if (src[8 * bit] & mask) {
					color |= 1 << bit;
				}
			}
			if (color != colorKey) {
				dst[i] = color;
			}
		}
		++src;
		dst += 256;
	}
}

static void AMIGA_decodeLevHelper(uint8 *dst, const uint8 *src, int offset10, int offset12, const uint8 *a5, bool sgdBuf) {
	if (offset10 != 0) {
		const uint8 *a0 = src + offset10;
		for (int y = 0; y < 224; y += 8) {
			for (int x = 0; x < 256; x += 8) {
				const int d3 = READ_BE_UINT16(a0); a0 += 2;
				const int d0 = d3 & 0x7FF;
				if (d0 != 0) {
					const uint8 *a2 = a5 + d0 * 32;
					if ((d3 & (1 << 12)) != 0) {
						a2 = AMIGA_mirrorY(a2);
					}
					if ((d3 & (1 << 11)) != 0) {
						a2 = AMIGA_mirrorX(a2);
					}
					AMIGA_blit4p8x8(dst + y * 256 + x, a2, 255);
				}
			}
		}
	}
	if (offset12 != 0) {
		const uint8 *a0 = src + offset12;
		for (int y = 0; y < 224; y += 8) {
			for (int x = 0; x < 256; x += 8) {
				const int d3 = READ_BE_UINT16(a0); a0 += 2;
				int d0 = d3 & 0x7FF;
				if (d0 != 0 && sgdBuf) {
					d0 -= 896;
				}
				if (d0 != 0) {
					const uint8 *a2 = a5 + d0 * 32;
					if ((d3 & (1 << 12)) != 0) {
						a2 = AMIGA_mirrorY(a2);
					}
					if ((d3 & (1 << 11)) != 0) {
						a2 = AMIGA_mirrorX(a2);
					}
					AMIGA_blit4p8x8(dst + y * 256 + x, a2, 0);
				}
			}
		}
	}
}

void Video::AMIGA_decodeLev(int level, int room) {
	uint8 *tmp = _res->_memBuf;
	const int offset = READ_BE_UINT32(_res->_lev + room * 4);
	if (!delphine_unpack(tmp, _res->_lev, offset)) {
		error("Bad CRC for level %d room %d", level, room);
	}
	uint16 offset10 = READ_BE_UINT16(tmp + 10);
	const uint16 offset12 = READ_BE_UINT16(tmp + 12);
	const uint16 offset14 = READ_BE_UINT16(tmp + 14);
	static const int kTempMbkSize = 512;
	uint8 *buf = (uint8 *)malloc(kTempMbkSize * 32);
	if (!buf) {
		error("Unable to allocate mbk temporary buffer");
	}
	int sz = 0;
	memset(buf, 0, 32);
	sz += 32;
	const uint8 *a1 = tmp + offset14;
	for (bool loop = true; loop;) {
		int d0 = READ_BE_UINT16(a1); a1 += 2;
		if (d0 & 0x8000) {
			d0 &= ~0x8000;
			loop = false;
		}
		const int d1 = READ_BE_UINT16(_res->_mbk + d0 * 6 + 4) & 0x7FFF;
		const uint8 *a6 = _res->findBankData(d0);
		if (!a6) {
			a6 = _res->loadBankData(d0);
		}
		const int d3 = *a1++;
		if (d3 == 255) {
			assert(sz / 32 + d1 < kTempMbkSize);
			memcpy(buf + sz, a6, d1 * 32);
			sz += d1 * 32;
		} else {
			for (int i = 0; i < d3 + 1; ++i) {
				const int d4 = *a1++;
				assert(sz / 32 + 1 < kTempMbkSize);
				memcpy(buf + sz, a6 + d4 * 32, 32);
				sz += 32;
			}
		}
	}
	memset(_frontLayer, 0, Video::GAMESCREEN_W * Video::GAMESCREEN_H);
	if (tmp[1] != 0) {
		assert(_res->_sgd);
		AMIGA_decodeSgd(_frontLayer, tmp + offset10, _res->_sgd);
		offset10 = 0;
	}
	AMIGA_decodeLevHelper(_frontLayer, tmp, offset10, offset12, buf, tmp[1] != 0);
	memcpy(_backLayer, _frontLayer, Video::GAMESCREEN_W * Video::GAMESCREEN_H);
	for (int i = 0; i < 2; ++i) {
		const int num = READ_BE_UINT16(tmp + i * 2);
		if (level == 0 && i == 0) {
			setPaletteSlotBE(i, 0);
		} else {
			setPaletteSlotBE(i, num);
		}
	}
}

void Video::drawSpriteSub1(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub1(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[i] != 0) {
				dst[i] = src[i] | colMask;
			}
		}
		src += pitch;
		dst += 256;
	}
}

void Video::drawSpriteSub2(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub2(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[-i] != 0) {
				dst[i] = src[-i] | colMask;
			}
		}
		src += pitch;
		dst += 256;
	}
}

void Video::drawSpriteSub3(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub3(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[i] != 0 && !(dst[i] & 0x80)) {
				dst[i] = src[i] | colMask;
			}
		}
		src += pitch;
		dst += 256;
	}
}

void Video::drawSpriteSub4(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub4(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[-i] != 0 && !(dst[i] & 0x80)) {
				dst[i] = src[-i] | colMask;
			}
		}
		src += pitch;
		dst += 256;
	}
}

void Video::drawSpriteSub5(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub5(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[i * pitch] != 0 && !(dst[i] & 0x80)) {
				dst[i] = src[i * pitch] | colMask;
			}
		}
		++src;
		dst += 256;
	}
}

void Video::drawSpriteSub6(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask) {
	debug(DBG_VIDEO, "Video::drawSpriteSub6(0x%X, 0x%X, 0x%X, 0x%X)", pitch, w, h, colMask);
	while (h--) {
		for (int i = 0; i < w; ++i) {
			if (src[-i * pitch] != 0 && !(dst[i] & 0x80)) {
				dst[i] = src[-i * pitch] | colMask;
			}
		}
		++src;
		dst += 256;
	}
}

void Video::drawChar(uint8 c, int16 y, int16 x) {
	debug(DBG_VIDEO, "Video::drawChar(0x%X, %d, %d)", c, y, x);
	y *= 8;
	x *= 8;
	const uint8 *src = _res->_fnt + (c - 32) * 32;
	uint8 *dst = _frontLayer + x + 256 * y;
	for (int h = 0; h < 8; ++h) {
		for (int i = 0; i < 4; ++i) {
			uint8 c1 = (*src & 0xF0) >> 4;
			uint8 c2 = (*src & 0x0F) >> 0;
			++src;

			if (c1 != 0) {
				if (c1 != 2) {
					*dst = _charFrontColor;
				} else {
					*dst = _charShadowColor;
				}
			} else if (_charTransparentColor != 0xFF) {
				*dst = _charTransparentColor;
			}
			++dst;

			if (c2 != 0) {
				if (c2 != 2) {
					*dst = _charFrontColor;
				} else {
					*dst = _charShadowColor;
				}
			} else if (_charTransparentColor != 0xFF) {
				*dst = _charTransparentColor;
			}
			++dst;
		}
		dst += 256 - 8;
	}
}

const char *Video::drawString(const char *str, int16 x, int16 y, uint8 col) {
	debug(DBG_VIDEO, "Video::drawString('%s', %d, %d, 0x%X)", str, x, y, col);
	int len = 0;
	int offset = y * 256 + x;
	uint8 *dst = _frontLayer + offset;
	while (1) {
		uint8 c = *str++;
		if (c == 0 || c == 0xB || c == 0xA) {
			break;
		}
		uint8 *dst_char = dst;
		const uint8 *src = _res->_fnt + (c - 32) * 32;
		for (int h = 0; h < 8; ++h) {
			for (int w = 0; w < 4; ++w) {
				uint8 c1 = (*src & 0xF0) >> 4;
				uint8 c2 = (*src & 0x0F) >> 0;
				++src;
				if (c1 != 0) {
					*dst_char = (c1 == 0xF) ? col : (0xE0 + c1);
				}
				++dst_char;
				if (c2 != 0) {
					*dst_char = (c2 == 0xF) ? col : (0xE0 + c2);
				}
				++dst_char;
			}
			dst_char += 256 - 8;
		}
		dst += 8; // character width
		++len;
	}
	markBlockAsDirty(x, y, len * 8, 8);
	return str - 1;
}
