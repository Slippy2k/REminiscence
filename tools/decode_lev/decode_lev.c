
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <png.h>
#include "unpack.h"

static void write_png_image_data(const char *file_path, const unsigned char *image_data, const unsigned char *image_clut, int w, int h) {
	int x, y, x_offset;

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	assert(png_ptr);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	assert(info_ptr);

	FILE *fp = fopen(file_path, "wb");
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

static void print_lev_hdr(int room, const unsigned char *p) {
	printf("room %d : 0x%04X 0x%04X 0x%04X\n", room, movel(p + 0), movel(p + 4), movel(p + 8));
}

static void decode_lev_picture(int room, const unsigned char *p, const unsigned char *mbk) {
	int offset, d0, d1, d3, d4, d7, ret, len, size = 0;
	const unsigned char *a1, *a5, *a6;
	unsigned char *a0, *a4;
	int mbk_uncompressed;

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
printf("d0=%d d1(len)=%X offset=%X d7=%d\n", d0, movew(mbk + d0 * 6 + 4), movel(mbk + d0 * 6), d7);
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
printf("d3=%d\n", d3 );
		do {
			d4 = *a1++;
			d4 <<= 5;
//			a6 = a5 + d4;
//			copy a6 -> a3, 16 * BE16
//			a3 += 32;
// printf("copy d4 %d 16\n", d4);
			size += 32;
		} while (d3-- > 0);
	}
	if (d7 == 0) goto loc_E28E;
printf("size %d strip %d (%d, %d) src_len %d offset %d\n", size, size / 224, 256 / 2 * 224, size + 224 * 8 * 2, a1 - p, offset );
}

static void decode_lev(const unsigned char *a4, const unsigned char *mbk) {
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
				print_lev_hdr(i, buf);
				decode_lev_picture(i, buf, mbk);
			}
		}
		offset_prev = offsets[i];
	}
}

int main(int argc, char *argv[]) {
	unsigned char *lev_data, *mbk_data;
	if (argc == 3) {
		lev_data = load_file(argv[1]);
		mbk_data = load_file(argv[2]);
		decode_lev(lev_data, mbk_data);
	}
	return 0;
}

