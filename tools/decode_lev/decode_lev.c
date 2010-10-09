
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <png.h>
#include "unpack.h"

static void write_png_image_data(const char *file_path, const unsigned char *image_data, const unsigned char *image_clut, int w, int h) {
	int x, y, x_offset;
	FILE *fp;

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	assert(png_ptr);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	assert(info_ptr);

	fp = fopen(file_path, "wb");
	assert(fp);
	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	png_set_packing(png_ptr);

	png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * h);
	for (y = 0; y < h; ++y) {
		row_pointers[y] = (png_bytep)malloc(w * 3);
		x_offset = 0;
		for (x = 0; x < w; ++x) {
			const unsigned char color = image_data[y * w + x];
			row_pointers[y][x_offset++] = image_clut[color * 3];
			row_pointers[y][x_offset++] = image_clut[color * 3 + 1];
			row_pointers[y][x_offset++] = image_clut[color * 3 + 2];
		}
	}
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	for (y = 0; y < h; ++y) {
		free(row_pointers[y]);
	}
	free(row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
}


#define DECODE_BUFSIZE 65536
static unsigned char buf[DECODE_BUFSIZE];
static unsigned char pic[DECODE_BUFSIZE];
static unsigned char tmp[DECODE_BUFSIZE];

static unsigned char *load_file(const char *file_path) {
	unsigned char *ptr = NULL;
	int sz;
	FILE *fp = fopen(file_path, "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		ptr = malloc(sz);
		if (ptr) {
			fread(ptr, sz, 1, fp);
		}
		fclose(fp);
		printf("loaded file '%s' size %d\n", file_path, sz);
	}
	assert(ptr);
	return ptr;
}

static void dump_file(int i, const unsigned char *p, int size) {
	char filename[512];
	FILE *fp;
	int count;

	snprintf(filename, sizeof(filename), "%d.cmp.dump", i);
	fp = fopen(filename, "w");
	if (fp) {
		count = fwrite(p, 1, size, fp);
		assert(count == size);
		fclose(fp);
	}
}

static int movel(const unsigned char *d) {
	const int i = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
	return i;
}

static int movew(const unsigned char *d) {
	const int i = (d[0] << 8) | d[1];
	return i;
}

static void print_lev_hdr(int room, const unsigned char *p, int size) {
	int i;

	printf("room %d size %d :", room, size);
	for (i = 10; i < 16; i += 2) {
		printf(" %04X", movew(p + i));
	}
	printf("\n");
}

static int _rightMasksTable[] = { 1, 3, 7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF };
static int _leftMasksTable[] = { 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF0, 0xFFE0, 0xFFC0, 0xFF80, 0xFF00, 0xFE00, 0xFC00, 0xF800, 0xF000, 0xE000, 0xC000, 0x8000, 0x7FFF, 0x3FFF, 0x1FFF, 0xFFF, 0x7FF, 0x3FF, 0x1FF, 0xFF, 0x7F, 0x3F, 0x1F, 0xF, 7, 3, 1 };

static int _bitmapColorKey = 8;

static void blitBitmapBlock(unsigned char *dst, int x, int y, int w, int h, unsigned char *src, unsigned char *mask, int size) {
	int i, j, c;
	int planar_size;
//	static const int plane_size = 32 * 224;

	++w;
	++h;
	planar_size = w * 2 * h;
	if (planar_size != size) printf("planar_size %d size %d\n", planar_size, size);

	if (x < 0 || y < 0) return; /* TEMP */

	dst += y * 256 + x;

	for (y = 0; y < h; ++y) {
		for (x = 0; x < w * 2; ++x) {
			for (i = 0; i < 8; ++i) {
				const int c_mask = 1 << (7 - i);
				c = 0;
				for (j = 0; j < 4; ++j) {
					if (src[j * planar_size] & mask[j * planar_size] & c_mask) {
						c |= 1 << j;
					}
				}
if (c != _bitmapColorKey)
				dst[8 * x + i] = c;
			}
			++src;
			++mask;
		}
		dst += 256;
	}
}

static void copySGD(unsigned char *a4, unsigned char *a5) {
	const unsigned char *a6;
	int d6, i;

	d6 = movew(a4); a4 += 2;
	d6 &= 0x7FFF;
	a6 = a4 + d6;
	do {
		d6 = *a4++;
		if ((d6 & 0x80) == 0) {
//			d6 = -(d6 - 0x7F) * 2; // (256 - i) / 2
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
}

static int _sgdLoopCount, _sgdDecodeLen;
static unsigned char *_sgdData, *_sgdDecodeBuf, *_roomBitmapBuffer;

static void loadSGD(unsigned char *a1) {
	int d4, d3, d2, d1, d0, i;
	unsigned char *a2, *a3, *a4, *a5, *a0;

//	word_2A31A = 1;
	_sgdLoopCount = movew(a1); a1 += 2;
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
				}
			} else {
				a4 += d3;
				if (_sgdDecodeLen != d2) {
					_sgdDecodeLen = d2;
					a5 = _sgdDecodeBuf;
					copySGD(a4, a5);
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
			blitBitmapBlock(_roomBitmapBuffer, (short)d0, (short)d1, d2, d3, a5, a2, d4);
		}
		--_sgdLoopCount;
	} while (_sgdLoopCount >= 0);
}

static void decode_lev_picture(int room, unsigned char *p, unsigned char *mbk) {
	int offset, d0, d1, d3, d4, d7, ret, len, size = 0;
	unsigned char *a1, *a5, *a6;
	unsigned char *a0, *a4;
	int i, mbk_uncompressed;

	offset = movew(p + 12);
	// copy unk_2801A, 224 * 4 * BE16
	if (p[1] == 0) {
		// copy unk_2871A, 224 * 4 * BE16
	}
	offset = movew(p + 14);
	// decode to unk_26124
//	a2 = _levelDataFileName_mbk;
//	a3 = _objectData; // tmpBuf
//	memset(a3, 0, 8 * 4);
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
//		d2 = d1 * 2;
//		copy a6 -> a3, d1 * BE16
//		a3 += d2;
//		byte_303BE = a3;
// printf("copy d1 %d\n", d1);
		size += d1 * 2;
	} else {
		assert(!mbk_uncompressed);
// printf("d3=%d\n", d3 );
		for (i = 0; i < d3 + 1; ++i) {
//		do {
			d4 = *a1++;
			d4 <<= 5;
//			a6 = a5 + d4;
//			copy a6 -> a3, 16 * BE16
//			a3 += 32;
// printf("copy d4 %d 16\n", d4);
			size += 32;
//		} while (d3-- > 0);
		}
	}
	if (d7 == 0) goto loc_E28E;
printf("size %d strip %d (%d, %d) src_len %d offset %d\n", size, size / 224, 256 / 2 * 224, size + 224 * 8 * 2, a1 - p, offset );
	if (p[1] != 0) {
memset(_roomBitmapBuffer, 0, 256 * 224);
		offset = movew(p + 10);
		loadSGD(p + offset);
//		word_2A31A = 1;
{
	int i;
	char name[64];
	unsigned char pal[256 * 3];

	snprintf(name, sizeof(name), "room_%02d.png", room);
	for (i = 0; i < 256; ++i) {
		pal[i * 3] = pal[i * 3 + 1] = pal[i * 3 + 2] = rand() & 255;
	}
	write_png_image_data(name, _roomBitmapBuffer, pal, 256, 224);
}
	}
//	a1 = _sgdDecodeBuf + 6;
//	memset(a1, 0xFF, 224 * 8 * 8);
//	loadLevelMapHelper();
}

static void decode_lev(const unsigned char *a4, unsigned char *mbk) {
	int i, offsets[64], offset_prev, ret, size;

	for (i = 0; i < 64; ++i) {
		offsets[i] = movel(a4 + 4 * i);
		printf("room %2d offset 0x%X\n", i, offsets[i]);
	}
	offset_prev = 0;
	for (i = 0; i < 64; ++i) {
		if (offset_prev != 0) {
			size = offsets[i] - offset_prev;
			if (size != 0) {
				ret = delphine_unpack(a4 + offsets[i] - 4, buf);
				assert(ret);
				print_lev_hdr(i, buf, size);
				decode_lev_picture(i, buf, mbk);
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

int main(int argc, char *argv[]) {
	unsigned char *lev_data, *mbk_data;
	if (argc == 4) {
		lev_data = load_file(argv[1]);
		mbk_data = load_file(argv[2]);
		_sgdData = load_file_sgd(load_file(argv[3]));
		_sgdDecodeBuf = malloc(7174);
		_roomBitmapBuffer = malloc(43014 * 16);
		decode_lev(lev_data, mbk_data);
		printf("done\n");
	}
	return 0;
}

