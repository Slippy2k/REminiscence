
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "unpack.h"

#define DECODE_BUFSIZE 65536

static unsigned char buf[2][DECODE_BUFSIZE];

static unsigned char *load_file(const char *filepath) {
	unsigned char *ptr = NULL;
	int sz;
	FILE *fp = fopen(filepath, "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		ptr = malloc(sz);
		if (ptr) {
			fread(ptr, sz, 1, fp);
		}
		fclose(fp);
		printf("loaded file '%s' size %d\n", filepath, sz);
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
	int i = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
	return i;
}

static int movew(const unsigned char *d) {
	int i = (d[0] << 16) | d[1];
	return i;
}

static void cutscene_decode_cmp(const unsigned char *d4) {
#if 0
	int d0;
	const unsigned char *a0, *d7;

	a0 = d4;
	d0 = movel(a0); a0 += 4;
	a0 += d0;
	d7 = a0 - 4;
	d0 = movel(a0); a0 += 4;
	if (d0 < 0) {
		d7 += 6 - d0;
	} else {
		a0 += d0;
		d7 = a0 - 4;
		d7 += 2;
	}

	a0 = d4;
	d0 = movel(a0); a0 += 4;
	a1 = a0 + d0;
	a4 = a1;
	d0 = movel(a4 - 4);
	a1 = a5 + d0;
	delphine_unpack // _cutsceneBufferPOL = a5;

	a1 = d0;
	d0 = (a1 + 1) & ~1;
	a5 = d0;
	d0 = movel(a0); a0 += 4;
	if (d0 < 0) {
		d0 = -d0 - 1;
		_cutsceneBufferCMD = a5;
	} else {
		a1 = a0 + d0;
		a4 = a1;
		delphine_unpack // _cutsceneBufferCMD = a5
	}
#endif
	const unsigned char *p = d4;
	int i, ret, packed_size, size;

	for (i = 0; i < 2; ++i) {
		packed_size = movel(p); p += 4;
		if (packed_size < 0) {
			size = packed_size = -packed_size;
			assert(size < DECODE_BUFSIZE);
			memcpy(buf[i], p, packed_size);
			ret = 1;
		} else {
			size = movel(p + packed_size - 4);
			assert(size < DECODE_BUFSIZE);
			assert((packed_size & 1) == 0);
			ret = delphine_unpack(p + packed_size - 4, buf[i]);
		}
		dump_file(i, buf[i], size);
		p += packed_size;
		printf("cmp index %d packed_size %4d size %4d ret %d\n", i, packed_size, size, ret);
	}
}

int main(int argc, char *argv[]) {
	if (argc > 1) {
		const char *filepath = argv[1];
		cutscene_decode_cmp(load_file(filepath));
	}
	return 0;
}

