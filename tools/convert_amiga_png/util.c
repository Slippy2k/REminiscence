
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <png.h>

void write_png_image_data(const char *file_path, const unsigned char *image_data, const unsigned char *image_clut, int w, int h) {
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

int _file_size = 0;

unsigned char *load_file(const char *file_path) {
	unsigned char *ptr = NULL;
	int sz;
	FILE *fp = fopen(file_path, "r");
	assert(fp);
	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ptr = malloc(sz);
	assert(ptr);
	fread(ptr, sz, 1, fp);
	fclose(fp);
	printf("loaded file '%s' size %d\n", file_path, sz);
	_file_size = sz;
	return ptr;
}

