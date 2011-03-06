
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "unpack.h"

//#define WITH_MASK
//#define WITH_DEPTH5

#define DECODE_BUFSIZE (65536 * 8)
static unsigned char _decodeLevBuf[DECODE_BUFSIZE];
static unsigned char pic[DECODE_BUFSIZE];
static unsigned char tmp[DECODE_BUFSIZE];
static unsigned char _tmpBuf[DECODE_BUFSIZE];

static void print_lev_hdr(int room, const unsigned char *p, int size) {
	int i;

	printf("room %d size %d :", room, size);
	for (i = 10; i < 16; i += 2) {
		printf(" %04X", movew(p + i));
	}
	printf("\n");
}

static void blit_sgd(unsigned char *dst, int x, int y, int w, int h, unsigned char *src, unsigned char *mask, int size) {
	int i, j, color, planar_size;
	const int x0 = x;
	const int y0 = y;

	printf("blit_sgd pos %d,%d dim %d,%d size %d\n", x, y, w, h, size);

	++w;
	++h;
	planar_size = w * 2 * h;
	assert(planar_size == size);

	dst += y * 256 + x;

	for (y = 0; y < h; ++y) {
		for (x = 0; x < w * 2; ++x) {
			for (i = 0; i < 8; ++i) {
				const int c_mask = 1 << (7 - i);
				color = 0;
				for (j = 0; j < 4; ++j) {
					if (mask[j * planar_size] & c_mask) {
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

static void blit_4v_8x8(unsigned char *dst, int x, int y, unsigned char *src, int shift) {
	int i, color, mask, bit;

	dst += (y * 256 + x) * 8;
	for (y = 0; y < 8; ++y) {
		for (i = 0; i < 8; ++i) {
			mask = 1 << (7 - i);
			color = 0;
			for (bit = 0; bit < 4; ++bit) {
				if (src[8 * bit] & mask) {
					color |= 1 << bit;
				}
			}
			dst[i] = (color << shift);
		}
		++src;
		dst += 256;
	}
}

static void blit_mask_4v_8x8(unsigned char *dst, int x, int y, unsigned char *src, unsigned char *src_mask, int shift) {
	int i, color, mask, bit, tmp;

	dst += (y * 256 + x) * 8;
	for (y = 0; y < 8; ++y) {
		for (i = 0; i < 8; ++i) {
			mask = 1 << (7 - i);
			color = 0;
			for (bit = 0; bit < 4; ++bit) {
//				tmp = (dst[i] & src_mask[i]) | src[8 * bit];
//				if (tmp & mask) {
//					color |= 1 << bit;
//				}
				if (src[8 * bit] & mask) {
					color |= 1 << bit;
				}
			}
			if (color != 0) {
				dst[i] = (color << shift);
			}
		}
		++src;
		dst += 256;
	}
}

static void clear_1v_8x8(unsigned char *dst, int x, int y, int shift) {
	int i;

	dst += (y * 256 + x) * 8;
	for (y = 0; y < 8; ++y) {
		for (i = 0; i < 8; ++i) {
			dst[i] &= ~(1 << shift);
		}
		dst += 256;
	}
}

static void blit_mask_1v_8x8(unsigned char *dst, int x, int y, unsigned char *src_mask, int shift, int or_mask) {
	int i, mask, tmp;

	dst += (y * 256 + x) * 8;
	for (y = 0; y < 8; ++y) {
		for (i = 0; i < 8; ++i) {
			mask = 1 << (7 - i);
			tmp = dst[i] & src_mask[i] & mask;
			if (tmp == 0) {
				dst[i] &= ~(1 << shift);
			}
			if (or_mask && tmp == 0) {
				dst[i] |= (1 << shift);
			}
		}
		dst += 256;
	}
}

static void blit_not_1v_8x8(unsigned char *dst, int x, int y, unsigned char *src, int shift) {
	int i, color, mask;

	dst += (y * 256 + x) * 8;
	for (y = 0; y < 8; ++y) {
		color = src[8] | src[16] | src[24];
		color |= *src++;
		for (i = 0; i < 8; ++i) {
			mask = 1 << (7 - i);
			if (~color & mask) {
				dst[i] |= 1 << shift;
			} else {
				dst[i] &= ~(1 << shift);
			}
		}
		dst += 256;
	}
}

static void blit_mask_not_1v_8x8(unsigned char *dst, int x, int y, unsigned char *src_mask, int shift) {
	int i, color, mask, d3;

	dst += (y * 256 + x) * 8;
	for (y = 0; y < 8; ++y) {
		for (i = 0; i < 8; ++i) {
			mask = 1 << (7 - i);
			color = dst[i];
			d3 = (~color | ~src_mask[i]);
			if (~d3 & mask) {
				dst[i] |= 1 << shift;
			} else {
				dst[i] &= ~(1 << shift);
			}
		}
		dst += 256;
	}
}

int copySGD(unsigned char *a4, unsigned char *a5) {
	const unsigned char *a6, *a5_start = a5;
	int d6, i;

	d6 = movew(a4); a4 += 2;
	d6 &= 0x7FFF;
	a6 = a4 + d6;
	do {
		d6 = *a4++;
		if ((d6 & 0x80) == 0) {
			for (i = 0; i < d6 + 1; ++i) {
				*a5++ = *a4++;
			}
		} else {
			d6 = -((signed char)d6);
			for (i = 0; i < d6 + 1; ++i) {
				*a5++ = *a4;
			}
			++a4;
		}
	} while (a4 < a6);
	assert(a4 == a6);
	return a5 - a5_start;
}

static int _sgdLoopCount, _sgdDecodeLen, _sgdRoomBuf;
static unsigned char *_sgdData, *_sgdDecodeBuf, *_roomBitmapBuf;
static unsigned char _roomPalBuf[256 * 3];
static unsigned char _roomOffset10DataBuf[224 * 8], _roomOffset12DataBuf[224 * 8];
static unsigned char byte_28012[8];
static unsigned char _tileTable[32];
static unsigned char _tileLookupTable[256];
static int byte_31D30 = 255;
static int _currentLevel;

static unsigned char *sub_EC94(unsigned char *a2) { // mirror_y
	int i, j;
	unsigned char *a0 = &_tileTable[32];

	a2 += 24;
	for (j = 0; j < 4; ++j) {
		for (i = 0; i < 8; ++i) {
			--a0;
			*a0 = *a2++;
		}
		a2 -= 16;
	}
	assert(a0 == _tileTable);
	return _tileTable;
}

static unsigned char *sub_ED06(unsigned char *a2) { // mirror_x
	int i;
	unsigned char *a0 = &_tileTable[0];
	unsigned char *a1 = &_tileLookupTable[0];

	for (i = 0; i < 32; ++i) {
		*a0++ = a1[*a2];
		++a2;
	}
	return _tileTable;
}

static void loadLevelMapHelper() {
	unsigned char *a0, *a1, *a2, *a3, *a4, *a5, *a6, *d7;
	int x, y, d0, d3, d5, n, i, j;

	if (_sgdRoomBuf == 0) {
		a0 = _roomOffset10DataBuf;
		a1 = _sgdDecodeBuf + 6;
		d7 = a1;
		a1 += 224 * 32;
		a5 = _tmpBuf;
		for (y = 0; y < 224 / 8; ++y) { // d2
			a4 = a1;
			for (x = 0; x < 256 / 8; ++x) { // d1
				d0 = movew(a0); a0 += 2;
				a3 = a1;
				d3 = d0;
				d0 &= 0x7FF;
				if (d0 != 0) {
					a2 = a5 + d0 * 32;
					if ((d3 & (1 << 12)) != 0) {
						a2 = sub_EC94(a2);
					}
					if ((d3 & (1 << 11)) != 0) {
						a2 = sub_ED06(a2);
					}
#if 0
					for (n = 0; n < 4; ++n) {
						for (i = 0; i < 8; ++i) {
							a1[32 * i] = *a2++;
						}
						a1 += 224 * 32;
					}
#else
					blit_4v_8x8(_roomBitmapBuf, x, y, a2, 0);
					a2 += 32;
#endif
					if (_currentLevel != 0) {
						d3 &= 0xDFFF;
					}
					d0 = d3;
					d0 &= 0x6000;
#ifdef WITH_MASK
					if (d0 != 0) {
#if 0
						for (i = 0; i < 8; ++i) {
							a1[32 * i] = 0;
						}
#else
						clear_1v_8x8(_roomBitmapBuf, x, y, 4);
#endif
					}
#endif
#ifdef WITH_DEPTH5
					if ((d3 & (1 << 15)) != 0) {
#if 0
//						a6 = a0 - 2 - _roomOffset10DataBuf + _roomOffset12DataBuf; // 287A1 - 280A1
//						d0 = movew(a6);
						a2 -= 32;
						a1 = d7;
						for (i = 0; i < 8; ++i) {
							d5 = a2[8] | a2[16] | a2[24];
							d5 |= *a2++;
							*a1 = ~d5;
							a1 += 32;
						}
#else
						a2 -= 32;
						blit_not_1v_8x8(_roomBitmapBuf, x, y, a2, 4);
#endif
					}
#endif
				}
				++d7; // 32
				a1 = a3 + 1;
			}
			d7 += 224;
			a1 = a4 + 256;
		}
	}
//loc_E84E:
	a0 = _roomOffset12DataBuf;
	a1 = _sgdDecodeBuf + 6;
	d7 = a1;
	a1 += 7168; // 224 * 32
	a5 = _tmpBuf;
	for (y = 0; y < 224 / 8; ++y) { // d2
		a4 = a1;
		for (x = 0; x < 256 / 8; ++x) { // d1
			d0 = movew(a0); a0 += 2;
			a3 = a1;
			d3 = d0;
			d0 &= 0x7FF;
			if (d0 != 0 && _sgdRoomBuf != 0) {
				d0 -= 896;
			}
			if (d0 != 0) {
				a2 = a5 + d0 * 32;
				if (d3 & (1 << 12)) {
					a2 = sub_EC94(a2);
				}
				if (d3 & (1 << 11)) {
					a2 = sub_ED06(a2);
				}
				if (_currentLevel != 0) {
					d3 &= 0xDFFF;
				}
				a6 = byte_28012;
				for (i = 0; i < 8; ++i) {
					d5 = a2[8] | a2[16] | a2[24];
					d5 |= *a2++;
					*a6++ = ~d5;
				}
				a2 -= 8;
#if 0
				for (j = 0; j < 4; ++j) {
					a6 = byte_28012;
					for (i = 0; i < 8; ++i) {
						d0 = *a6++;
						a1[i * 32] &= d0;
						d0 = *a2++;
						a1[i * 32] |= d0;
					}
					a1 += 7168;
				}
#else
				a6 = byte_28012;
				blit_mask_4v_8x8(_roomBitmapBuf, x, y, a2, a6, 0);
#endif
				d0 = d3;
#ifdef WITH_MASK
				if ((d0 & 0x6000) == 0) {
					a6 = byte_28012;
/*					for (i = 0; i < 8; ++i) {
						d0 = *a6++;
						a1[i * 32] &= d0;
						a1[i * 32] |= ~d0;
					}*/
					blit_mask_1v_8x8(_roomBitmapBuf, x, y, a6, 4, 1);
				} else {
					a6 = byte_28012;
/*					for (i = 0; i < 8; ++i) {
						d0 = *a6++;
						a1[i * 32] &= d0;
					}*/
					blit_mask_1v_8x8(_roomBitmapBuf, x, y, a6, 4, 0);
				}
#endif
#ifdef WITH_DEPTH5
				if ((d3 & (1 << 15)) != 0) {
					a6 = byte_28012;
/*					a1 = d7;
					for (i = 0; i < 8; ++i) {
						d0 = *a6++;
						d3 = a1[i * 32];
						d3 = ~d3 | ~d0;
						a1[i * 32] = ~d3;
					}*/
					blit_mask_not_1v_8x8(_roomBitmapBuf, x, y, a6, 4);
				}
#endif
			}
			d7 += 1;
			a1 = a3 + 1;
		}
		d7 += 224;
		a1 = a4 + 256;
	}
}

static void loadSGD(unsigned char *a1) {
	int d4, d3, d2, d1, d0, i, len;
	unsigned char *a2, *a3, *a4, *a5, *a0;

//	word_2A31A = 1;
	_sgdLoopCount = movew(a1); a1 += 2;
printf("loadSGD _sgdLoopCount %d\n", _sgdLoopCount );
	--_sgdLoopCount;
	do {
		d2 = movew(a1); a1 += 2;
		d0 = movew(a1); a1 += 2;
		d1 = movew(a1); a1 += 2;
// printf("loadSGD d2=0x%X d1=0x%X d0=0x%X _sgdLoopCount %d\n", d2, d0, d1, _sgdLoopCount);
		if (d2 != 0xFFFF) {
			d2 &= ~(1 << 15);
			a4 = _sgdData;
			d3 = movel(a4 + d2 * 4);
			if (d3 < 0) {
				d3 = -d3;
				a4 += d3;
				d3 = movew(a4); a4 += 2;
				if (_sgdDecodeLen != d2) {
					_sgdDecodeLen = d2;
					a2 = _sgdDecodeBuf;
					for (i = 0; i < d3; ++i) {
						a2[i] = a4[i];
					}
					a0 = _sgdDecodeBuf;
					len = d3;
					printf("SGD %d len %d\n", _sgdDecodeLen, len);
				}
			} else {
				a4 += d3;
				if (_sgdDecodeLen != d2) {
					_sgdDecodeLen = d2;
					a5 = _sgdDecodeBuf;
					len = copySGD(a4, a5);
					printf("SGD %d len %d\n", _sgdDecodeLen, len);
				}
			}
			a0 = _sgdDecodeBuf;
			d2 = 0;
			d3 = 0;
			d2 = a0[0];
			++d2;
			d2 >>= 1;
			--d2;
			d3 = a0[1];
			a5 = a0 + 4; // src
			d4 = movew(a0 + 2);
			a2 = a0 + d4 + 4; // mask
			assert(len == d4 * 5 + 4);
			blit_sgd(_roomBitmapBuf, (short)d0, (short)d1, d2, d3, a5, a2, d4);
		}
		--_sgdLoopCount;
	} while (_sgdLoopCount >= 0);
}

static int convert_amiga_color(unsigned char *p, int offset, int color) {
	static const int t[] = { 0, 4, 8 };
	int i, j = offset * 3;

//	printf("convert_amiga_color color 0x%03X offset %d\n", color, offset);
	for (i = 0; i < 3; ++i) {
		p[j + i] = (color >> t[i]) & 15;
		p[j + i] = (p[j + i] << 4) + p[j + i];
	}
}

static void loadLevelMap(int level, int room, unsigned char *p, unsigned char *mbk, unsigned char *pal) {
	int offset, d0, d1, d2, d3, d4, d7, ret, len, size = 0;
	unsigned char *a0, *a1, *a3, *a4, *a5, *a6;
	int i, mbk_uncompressed;
	char name[64];

	_sgdRoomBuf = 0;

	offset = movew(p + 12);
	memcpy(_roomOffset12DataBuf, p + offset, 896 * 2);
	if (p[1] == 0) {
		offset = movew(p + 10);
		memcpy(_roomOffset10DataBuf, p + offset, 896 * 2);
	}
	offset = movew(p + 14);
	// decode to unk_26124
//	a2 = _levelDataFileName_mbk;
	a3 = _tmpBuf;
	memset(a3, 0, 8 * 4);
	a3 += 32;
	a0 = pic;
	a1 = p + offset;
	d7 = 0;
loc_E28E:
	d0 = movew(a1); a1 += 2;
	if (d0 & 0x8000) {
		d0 &= ~0x8000;
		d7 = 1; //st d7
	}
//	move.w  d0,(a0)+
//	d1 = a3;
//	move.l  d1,(a0)+
//	d0 = d0 * 6;
//	move.w  4(a2,d0.w),d1
	d1 = movew(mbk + d0 * 6 + 4);
// printf("d0=%d d1(len)=%X offset=%X d7=%d\n", d0, movew(mbk + d0 * 6 + 4), movel(mbk + d0 * 6), d7);
	if ((d1 & 0x8000) != 0) { // uncompressed
		mbk_uncompressed = 1;
//		neg.w d1
//		move.l  (a2,d0.w),d0
//		lea     (a2,d0.l),a6
		d1 = -(short)d1;
		len = movel(mbk + d0 * 6);
// printf("d1 %d offset %X\n", d1, len);
		a6 = mbk + len;
	} else {
		mbk_uncompressed = 0;
//		move.l  (a2,d0.w),d0
//		lea     (a2,d0.l),a4
		len = movel(mbk + d0 * 6);
// printf("d1 %d offset %X\n", d1, len);
		ret = delphine_unpack(mbk + len - 4, tmp);
		assert(ret);
		a6 = a5 = tmp;
	}
	d3 = *a1++;
	if (d3 == 255) {
		d1 <<= 4;
		d2 = d1 * 2;
		memcpy(a3, a6, d2);
		a3 += d2;
//		dword_303BE = a3;
	} else {
//		assert(!mbk_uncompressed);
		for (i = 0; i < d3 + 1; ++i) {
			d4 = *a1++;
			d4 <<= 5;
			a6 = a5 + d4;
			memcpy(a3, a6, 16 * 2);
			a3 += 32;
		}
//		dword_303BE = a3;
	}
	if (d7 == 0) goto loc_E28E;
	memset(_roomBitmapBuf, 0, 256 * 224);
	if (p[1] != 0) {
		offset = movew(p + 10);
		loadSGD(p + offset);
		_sgdRoomBuf = 1;
	}
	a1 = _sgdDecodeBuf + 6;
	memset(a1, 0xFF, 224 * 8 * 8);
//	for (i = 0; i < 256 * 224; ++i) _roomBitmapBuf[i] = 0x1F;
	loadLevelMapHelper();
	a0 = p + 2;
	d1 = movew(a0); a0 += 2;
printf("pal %d %d %d\n", d1, movew(a0), movew(a0 + 2));
	assert(d1 >= 0 && d1 < 6);
	a3 = pal + d1 * 32;
	for (i = 0; i < 16; ++i) {
		d2 = movew(a3); a3 += 2;
		convert_amiga_color(_roomPalBuf, 16 + i, d2);
	}
	d1 = movew(a0); a0 += 2;
	if ((byte_31D30 & 0x80) == 0) {
		d1 = movew(a0); a0 += 2;
	}
	assert(d1 >= 0 && d1 < 6);
if (_currentLevel == 0) {
	d1 = 0;
}
	a3 = pal + d1 * 32;
	for (i = 0; i < 16; ++i) {
		d2 = movew(a3); a3 += 2;
		convert_amiga_color(_roomPalBuf, i, d2);
	}
#if 1
	// output palette (debug)
	for (i = 0; i < 16; ++i) {
		fill_image_data(_roomBitmapBuf, i * 8, 0, 8, 4, i);
		fill_image_data(_roomBitmapBuf, i * 8, 6, 8, 4, 16 + i);
	}
#endif
if (_currentLevel != 0) {
	_roomPalBuf[0] = _roomPalBuf[1] = _roomPalBuf[2] = 0;
}
	snprintf(name, sizeof(name), "DUMP/level_%d_room_%02d.png", level, room);
	write_png_image_data(name, _roomBitmapBuf, _roomPalBuf, 256, 224);
}

static void decode_lev(int level, const unsigned char *a4, unsigned char *mbk, unsigned char *pal) {
	int i, offsets[64], offset_prev, ret, size;

//	byte_31D30 = level;
	_currentLevel = level;
	for (i = 0; i < 64; ++i) {
		offsets[i] = movel(a4 + 4 * i);
		printf("room %2d offset 0x%X\n", i, offsets[i]);
	}
	offset_prev = 0;
	for (i = 0; i < 64; ++i) {
		if (offset_prev != 0) {
			size = offsets[i] - offset_prev;
			if (size != 0) {
				ret = delphine_unpack(a4 + offsets[i] - 4, _decodeLevBuf);
				assert(ret);
//				print_lev_hdr(i, _decodeLevBuf, size);
				loadLevelMap(level, i, _decodeLevBuf, mbk, pal);
			}
		}
		offset_prev = offsets[i];
	}
}

static unsigned char *load_file_sgd(unsigned char *a1) {
	int d0;
	unsigned char *a4, *a5;

	d0 = movel(a1);
	printf("SGD length %d\n", d0);
	a4 = a1 + d0;
	a1 += 256;
//	a5 = a1;
	a5 = malloc(74935);
	delphine_unpack(a4, a5);
	return a5;
}

static const char *lev_names[] = {
	"level1.lev", "level2_1.lev", "dt.LEV", "level3_1.lev", "level3_1.lev", "level4_1.LEV", "level4_2.lev"
};

static const char *pal_names[] = {
	"level1.pal", "level2.PAL", "dt.PAL", "level3_1.pal", "level3_1.pal", "level4_1.PAL", "level4_2.pal"
};

static const char *mbk_names[] = {
	"level1.mbk", "level2.MBK", "dt.MBK", "level3_1.mbk", "level3_1.mbk", "level4_1.MBK", "level4_2.mbk"
};

static const char *spc_names[] = {
	"level1.spc", "level2.spc", "dt.spc", "level3_1.spc", "level3_1.spc", "level4_1.spc", "level4_2.spc"
};

static const char *rpc_names[] = {
	"level1.rpc", "level2.rpc", "dt.rpc", "level3_1.rpc", "level3_1.rpc", "level4_1.rpc", "level4_2.rpc"
};

static const char *sgd_name = "level1.sgd";

int main(int argc, char *argv[]) {
	int i, b, mask;
	char path[1024];
	unsigned char *lev_data, *pal_data, *mbk_data;

	for (i = 0; i < 256; ++i) {
		mask = 0;
		for (b = 0; b < 8; ++b) {
			if (i & (1 << b)) {
				mask |= 1 << (7 - b);
			}
		}
		_tileLookupTable[i] = mask;
	}
	if (argc == 2) {
		if (strstr(argv[1], ".cmp")) {
			decode_cmp(argv[1]);
		} else if (strstr(argv[1], ".spr")) {
			unsigned char *ptr = load_file(argv[1]);
			decode_spr(ptr, _file_size);
		} else if (strstr(argv[1], ".spm")) {
			unsigned char *ptr = load_file(argv[1]);
			decode_spm(argv[1], ptr, _file_size);
		} else if (strstr(argv[1], ".spc")) {
			unsigned char *spc_data = load_file(argv[1]);
			// load .rpc .mbk
			// lookup from .spc
			decode_spc(spc_data, _file_size);
		} else if (strstr(argv[1], "flashback")) {
			unsigned char *ptr = load_file(argv[1]);
			printf("spmDataOffsets[1278] = {\n\t");
			for (i = 0; i < 1278; ++i) {
				printf("0x%05X, ", movel(ptr + 0x2ABD8 + i * 4) );
				if ((i & 7) == 7) {
					printf("\n\t");
				}
			}
		}
	} else if (argc == 3) {
		i = atoi(argv[2]);
		snprintf(path, sizeof(path), "%s/%s", argv[1], lev_names[i]);
		lev_data = load_file(path);
		snprintf(path, sizeof(path), "%s/%s", argv[1], pal_names[i]);
		pal_data = load_file(path);
		snprintf(path, sizeof(path), "%s/%s", argv[1], mbk_names[i]);
		mbk_data = load_file(path);
		snprintf(path, sizeof(path), "%s/%s", argv[1], sgd_name);
		_sgdData = load_file_sgd(load_file(path));
		_sgdDecodeBuf = malloc(7174 * 16);
		_roomBitmapBuf = malloc(43014 * 16);
		decode_lev(i, lev_data, mbk_data, pal_data);
		if (i == 1) {
			free(lev_data);
			snprintf(path, sizeof(path), "%s/level2_2.lev", argv[1]);
			lev_data = load_file(path);
			decode_lev(i, lev_data, mbk_data, pal_data);
		}
		printf("done\n");
	}
	return 0;
}

