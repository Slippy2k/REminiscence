
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include "util.h"


typedef struct {
	uint8 b, g, r;
	uint8 reserved;
} rgb_quad_t;

typedef struct {
	uint16 type;
	uint32 size;
	uint16 reserved1;
	uint16 reserved2;
	uint32 off_bits;
} bmp_file_header_t;

typedef struct {
	uint32 size;
	uint32 w, h;
	uint16 planes;
	uint16 bit_count;
	uint32 compression;
	uint32 size_image;
	uint32 x_pels_per_meter;
	uint32 y_pels_per_meter;
	uint32 num_colors_used;
	uint32 num_colors_important;
} bmp_info_header_t;


static int file_size(FILE *fp) {
	int sz = 0;
	int pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	fseek(fp, pos, SEEK_SET);
	return sz;
}

static void file_write_uint16(FILE *fp, uint16 n) {
	fputc(n & 0xFF, fp);
	fputc(n >> 8, fp);
}

static void file_write_uint32(FILE *fp, uint32 n) {
	fputc(n & 0xFF, fp);
	fputc(n >> 8, fp);
	fputc(n >> 16, fp);
	fputc(n >> 24, fp);
}

static void file_write_rgb_quad(FILE *fp, const rgb_quad_t *quad) {
	fputc(quad->b, fp);
	fputc(quad->g, fp);
	fputc(quad->r, fp);
	fputc(quad->reserved, fp);
}

static void file_write_bmp_file_header(FILE *fp, const bmp_file_header_t *file_header) {
	file_write_uint16(fp, file_header->type);
	file_write_uint32(fp, file_header->size);
	file_write_uint16(fp, file_header->reserved1);
	file_write_uint16(fp, file_header->reserved2);
	file_write_uint32(fp, file_header->off_bits);
}

static void file_write_bmp_info_header(FILE *fp, const bmp_info_header_t *info_header) {
	file_write_uint32(fp, info_header->size);
	file_write_uint32(fp, info_header->w);
	file_write_uint32(fp, info_header->h);
	file_write_uint16(fp, info_header->planes);
	file_write_uint16(fp, info_header->bit_count);
	file_write_uint32(fp, info_header->compression);
	file_write_uint32(fp, info_header->size_image);
	file_write_uint32(fp, info_header->x_pels_per_meter);
	file_write_uint32(fp, info_header->y_pels_per_meter);
	file_write_uint32(fp, info_header->num_colors_used);
	file_write_uint32(fp, info_header->num_colors_important);
}

static uint8 img_decode_buffer[256 * 224];
static uint8 img_hflip_buffer[256 * 224];

static void img_hflip(const uint8 *src, uint8 *dst) {
	int y;
	src += 256 * 224;
	for (y = 0; y < 224; ++y) {
		src -= 256;
		memcpy(dst, src, 256);
		dst += 256;
	}
}

static void img_decode_menu_bitmap(const uint8 *src, uint8 *dst) {
	int i, y, x;
	for (i = 0; i < 4; ++i) {
		for (y = 0; y < 224; ++y) {
			for (x = 0; x < 64; ++x) {
				dst[i + x * 4 + 256 * y] = src[0x3800 * i + x + 64 * y];
			}
		}
	}
}

static void img_make_bitmap_menu(const uint8 *img, const uint8 *pal, FILE *fp) {
	int i;
	bmp_file_header_t file_header;
	bmp_info_header_t info_header;
	
	/* file header */
	memset(&file_header, 0, sizeof(file_header));
	file_header.type = 0x4D42;
	file_header.size = 14 + 40 + 4 * 256 + 256 * 224;
	file_header.reserved1 = 0;
	file_header.reserved2 = 0;
	file_header.off_bits = 14 + 40 + 4 * 256;
	file_write_bmp_file_header(fp, &file_header);
	
	/* info header */
	memset(&info_header, 0, sizeof(info_header));
	info_header.size = 40;
	info_header.w = 256;
	info_header.h = 224;
	info_header.planes = 1;
	info_header.bit_count = 8;
	info_header.compression = 0;
	info_header.size_image = 256 * 224;
	info_header.x_pels_per_meter = 0;
	info_header.y_pels_per_meter = 0;
	info_header.num_colors_used = 0;
	info_header.num_colors_important = 0;
	file_write_bmp_info_header(fp, &info_header);
	
	/* palette */
	for (i = 0; i < 256; ++i) {
		rgb_quad_t quad;
		memset(&quad, 0, sizeof(quad));
		quad.r = *pal++;
		quad.g = *pal++;
		quad.b = *pal++;
		file_write_rgb_quad(fp, &quad);
	}
	
	img_decode_menu_bitmap(img, img_decode_buffer);	
	
	img_hflip(img_decode_buffer, img_hflip_buffer);
	fwrite(img_hflip_buffer, 1, sizeof(img_hflip_buffer), fp);
}

static void img_decode_level_bitmap_plane(uint16 sz, const uint8 *src, uint8 *dst) {
	const uint8 *end = src + sz;
	while (src < end) {
		int16 code = (int8)*src++;
		if (code < 0) {
			int len = 1 - code;
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

static uint8 img_plane_buffer[256 * 56];

static void img_decode_level_bitmap(const uint8 *src, uint8 *dst) {
	int i;
	for (i = 0; i < 4; ++i) {
		uint16 sz = read_uint16LE(src); src += 2;
		img_decode_level_bitmap_plane(sz, src, img_plane_buffer); src += sz;
		memcpy(dst, img_plane_buffer, 256 * 56);
		dst += 256 * 56;
	}
}

static void img_write_palette(const uint8 *pal, int pal_num, FILE *fp) {
	int i;
	const uint8 *p = pal + pal_num * 32;
	for (i = 0; i < 16; ++i) {
		uint8 r, g, b;
		rgb_quad_t quad;
		uint16 color = read_uint16BE(p);
		memset(&quad, 0, sizeof(quad));
		r = (color & 0x00F) >> 0;
		g = (color & 0x0F0) >> 4;
		b = (color & 0xF00) >> 8;
		quad.r = (r << 4) | r;
		quad.g = (g << 4) | g;
		quad.b = (b << 4) | b;
		file_write_rgb_quad(fp, &quad);
		p += 2;
	}
}

static void img_write_black_palette(FILE *fp) {
	int i;
	rgb_quad_t quad;
	memset(&quad, 0, sizeof(quad));
	for (i = 0; i < 16; ++i) {
		file_write_rgb_quad(fp, &quad);
	}
}

static void img_make_bitmap_level(const uint8 *img, const uint8 *pal, FILE *fp) {
	bmp_file_header_t file_header;
	bmp_info_header_t info_header;
	uint8 palSlot1, palSlot2, palSlot3, palSlot4;
	
	/* file header */
	memset(&file_header, 0, sizeof(file_header));
	file_header.type = 0x4D42;
	file_header.size = 14 + 40 + 4 * 256 + 256 * 224;
	file_header.reserved1 = 0;
	file_header.reserved2 = 0;
	file_header.off_bits = 14 + 40 + 4 * 256;
	file_write_bmp_file_header(fp, &file_header);
	
	/* info header */
	memset(&info_header, 0, sizeof(info_header));
	info_header.size = 40;
	info_header.w = 256;
	info_header.h = 224;
	info_header.planes = 1;
	info_header.bit_count = 8;
	info_header.compression = 0;
	info_header.size_image = 256 * 224;
	info_header.x_pels_per_meter = 0;
	info_header.y_pels_per_meter = 0;
	info_header.num_colors_used = 0;
	info_header.num_colors_important = 0;
	file_write_bmp_info_header(fp, &info_header);
	
	palSlot1 = *img++;
	palSlot2 = *img++;
	palSlot3 = *img++;
	palSlot4 = *img++;
	img_write_palette(pal, palSlot1, fp);
	img_write_palette(pal, palSlot2, fp);
	img_write_palette(pal, palSlot3, fp);
	img_write_palette(pal, palSlot4, fp);
	img_write_black_palette(fp); // conrad
	img_write_black_palette(fp); // monsters
	img_write_black_palette(fp);
	img_write_black_palette(fp);
	img_write_palette(pal, palSlot1, fp);
	img_write_palette(pal, palSlot2, fp);
	img_write_palette(pal, palSlot3, fp);
	img_write_palette(pal, palSlot4, fp);
	img_write_black_palette(fp); // cutscene
	img_write_black_palette(fp); // cutscene
	img_write_black_palette(fp);
	img_write_black_palette(fp); // text

	img_decode_level_bitmap(img, img_decode_buffer);	
	
	img_hflip(img_decode_buffer, img_hflip_buffer);
	fwrite(img_hflip_buffer, 1, sizeof(img_hflip_buffer), fp);
}

static void make_bmp_filename(const char *out, const char *img_file, char *bmp_file) {
	char *ext;
	const char *sep = strrchr(img_file, '/');
	snprintf(bmp_file, MAXPATHLEN, "%s%s", out, sep ? (sep + 1) : img_file);
	ext = strrchr(bmp_file, '.');
	if (ext) {
		strcpy(ext + 1, "bmp");
	}
}

static void make_bmp_level_filename(const char *out, const char *img_file, int room, char *bmp_file) {
	char *ext;
	const char *sep = strrchr(img_file, '/');
	snprintf(bmp_file, MAXPATHLEN, "%s%s", out, sep ? (sep + 1) : img_file);
	ext = strrchr(bmp_file, '.');
	if (ext) {
		sprintf(ext, "-%02d.bmp", room);
	}
}

static void img_convert(const char *filename, const uint8 *img, const uint8 *pal, int menu_type, const char *out) {
	if (menu_type) {
		FILE *fp;
		char bitmap_filename[MAXPATHLEN];
		make_bmp_filename(out, filename, bitmap_filename);
		fp = fopen(bitmap_filename, "wb");
		if (fp) {
			img_make_bitmap_menu(img, pal, fp);
			fclose(fp);
		}
	} else {
		FILE *fp;
		char bitmap_filename[MAXPATHLEN];
		int room;
		for (room = 0; room < 64; ++room) {
			int32 off = read_uint32LE(img + room * 6);
			if (off == 0) {
				continue;
			}
			assert(off > 0); /* packed bitmap */
			make_bmp_level_filename(out, filename, room, bitmap_filename);
			fp = fopen(bitmap_filename, "wb");
			if (fp) {
				img_make_bitmap_level(img + off, pal, fp);
				fclose(fp);
			}
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc == 4) {
		int sz, menu_type;
		FILE *fp;
		uint8 *img, *pal;
		
		fp = fopen(argv[1], "rb");
		assert(fp);
		sz = file_size(fp);
		img = (uint8 *)malloc(sz);
		fread(img, 1, sz, fp);
		fclose(fp);
		
		menu_type = 0;
		if (sz == 256 * 224) {
			menu_type = 1;
		}
		
		fp = fopen(argv[2], "rb");
		assert(fp);
		sz = file_size(fp);
		pal = (uint8 *)malloc(sz);
		fread(pal, 1, sz, fp);
		fclose(fp);

		img_convert(argv[1], img, pal, menu_type, argv[3]);

		free(img);
		free(pal);
	}
	return 0;
}
