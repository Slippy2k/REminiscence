
#ifndef UTIL_H__
#define UTIL_H__

void write_png_image_data(const char *file_path, const unsigned char *image_data, const unsigned char *image_clut, int w, int h);

int _file_size;
unsigned char *load_file(const char *file_path);
void dump_file(int i, const unsigned char *p, int size);

static inline int movel(const unsigned char *d) {
	int i = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
	return i;
}

static inline int movew(const unsigned char *d) {
	int i = (d[0] << 8) | d[1];
	return i;
}

#endif

