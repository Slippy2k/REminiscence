
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "resource.h"
#include "decode.h"

struct Color {
	int r, g, b;
};

enum {
	kClutSize = 1024
};

static Color _clut[kClutSize];
static int _clutSize;

static void readClut(const uint8_t *ptr) {
	ptr += 6; // seed+flags
	_clutSize = READ_BE_UINT16(ptr); ptr += 2;
	assert(_clutSize < kClutSize);
	for (int i = 0; i < _clutSize; ++i) {
		const int index = READ_BE_UINT16(ptr); ptr += 2;
		assert(i == index);
		// ignore lower bits
		_clut[i].r = ptr[0]; ptr += 2;
		_clut[i].g = ptr[0]; ptr += 2;
		_clut[i].b = ptr[0]; ptr += 2;
	}
	printf("_clutSize %d\n", _clutSize);
}

static uint8_t *decodeResourceData(ResourceMac &res, const char *name, bool decompressLzss) {
	uint8_t *data = 0;
	const ResourceEntry *entry = res.findEntry(name);
	if (entry) {
		res._f.seek(res._dataOffset + entry->dataOffset);
		const uint32_t dataSize = res._f.readUint32BE();
		printf("decodeResourceData name '%s' dataSize %d\n", name, dataSize);
		if (decompressLzss) {
			data = decodeLzss(res._f);
		} else {
			data = (uint8_t *)malloc(dataSize);
			res._f.read(data, dataSize);
		}
	}
	return data;
}

static void writePngImageData(const char *filePath, const uint8_t *imageData, Color *imageClut, int w, int h) {
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	assert(png_ptr);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	assert(info_ptr);

	FILE *fp = fopen(filePath, "wb");
	assert(fp);
	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	png_set_packing(png_ptr);

	png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * h);
	for (int y = 0; y < h; ++y) {
		row_pointers[y] = (png_bytep)malloc(w * 3);
		int xOffset = 0;
		for (int x = 0; x < w; ++x) {
			const uint8_t color = imageData[y * w + x];
			row_pointers[y][xOffset++] = imageClut[color].r;
			row_pointers[y][xOffset++] = imageClut[color].g;
			row_pointers[y][xOffset++] = imageClut[color].b;
		}
	}
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	for (int y = 0; y < h; ++y) {
		free(row_pointers[y]);
	}
	free(row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
}

static void dumpHistogram(const uint8_t *imageData, int imageLen) {
	int histogram16[16];
	memset(histogram16, 0, sizeof(histogram16));
	for (int i = 0; i < imageLen; ++i) {
		++histogram16[imageData[i] / 16];
	}
	printf("histogram '");
	for (int i = 0; i < 16; ++i) {
		if (histogram16[i] == 0) {
			printf(".");
		} else if (histogram16[i] < 16) {
			printf("%x", histogram16[i]);
		} else {
			printf("+");
		}
	}
	printf("'\n");
}

enum {
	kImageOffsetsCount = 2048
};

static void setImageClut(Color *imageClut, uint8_t dest, uint8_t src) {
	memcpy(&imageClut[dest * 16], &_clut[src * 16], 16 * sizeof(Color));
}

static void setGrayImageClut(Color *imageClut) {
	for (int i = 0; i < 256; ++i) {
		imageClut[i].r = imageClut[i].g = imageClut[i].b = i;
	}
}

static void decodeImageData(const char *name, const uint8_t *ptr) {
	static const int levelsColorOffset[] = { 24, 28, 36, 40, 44 }; // red palette: 32
	Color imageClut[256];
	memset(imageClut, 0, sizeof(imageClut));
	if (strncmp(name, "Title", 5) == 0) {
		switch (name[6] - '1') {
		case 0:
			setGrayImageClut(imageClut);
			break;
		case 1:
			memcpy(imageClut, &_clut[192], 192 * sizeof(Color));
			break;
		case 2:
		case 3:
		case 4:
			memcpy(imageClut, &_clut[0], 192 * sizeof(Color));
			break;
		case 5:
			setGrayImageClut(imageClut);
			break;
		}
	} else if (strncmp(name, "Level", 5) == 0) {
		const int num = name[6] - '1';
		assert(num >= 0 && num < 6);
		const int offset = levelsColorOffset[num];
		for (int i = 0; i < 4; ++i) {
			setImageClut(imageClut, i, offset + i);
			setImageClut(imageClut, 8 + i, offset + i);
		}
		setImageClut(imageClut, 0x4, 0x30);
		setImageClut(imageClut, 0xD, 0x38);
	} else if (strncmp(name, "Objects", 7) == 0) {
		const int num = name[8] - '1';
		assert(num >= 0 && num < 6);
		const int offset = levelsColorOffset[num];
		for (int i = 0; i < 4; ++i) {
			setImageClut(imageClut, i, offset + i);
		}
		setImageClut(imageClut, 0x4, 0x30);
	} else if (strcmp(name, "Icons") == 0) {
		setImageClut(imageClut, 0xA, levelsColorOffset[0] + 2);
		setImageClut(imageClut, 0xC, 0x37);
		setImageClut(imageClut, 0xD, 0x38);
	} else if (strcmp(name, "Font") == 0) {
		setImageClut(imageClut, 0xC, 0x37);
	} else if (strcmp(name, "Alien") == 0) {
		setImageClut(imageClut, 0x5, 0x36);
	} else if (strcmp(name, "Junky") == 0) {
		setImageClut(imageClut, 0x5, 0x32);
	} else if (strcmp(name, "Mercenary") == 0) {
		setImageClut(imageClut, 0x5, 0x34);
	} else if (strcmp(name, "Replicant") == 0) {
		setImageClut(imageClut, 0x5, 0x35);
	} else if (strcmp(name, "Person") == 0) {
		setImageClut(imageClut, 0x4, 0x30); // red: 0x31
	}
	const uint8_t *basePtr = ptr;
	const uint16_t sig = READ_BE_UINT16(ptr); ptr += 2;
	assert(sig == 0xC211 || sig == 0xC103);
	const int count = READ_BE_UINT16(ptr); ptr += 2;
	assert(count < kImageOffsetsCount);
	ptr += 4;
	uint32_t imageOffsets[kImageOffsetsCount];
	for (int i = 0; i < count; ++i) {
		imageOffsets[i] = READ_BE_UINT32(ptr); ptr += 4;
	}
	for (int i = 0; i < count; ++i) {
		printf("image %d/%d offset 0x%X\n", 1 + i, count, imageOffsets[i]);
		ptr = basePtr + imageOffsets[i];
		if (imageOffsets[i] == 0) {
			// occurs with Conrad frames
			continue;
		}
		const int w = READ_BE_UINT16(ptr); ptr += 2;
		const int h = READ_BE_UINT16(ptr); ptr += 2;
		uint8_t *imageData8 = (uint8_t *)calloc(1, w * h);
		switch (sig) {
		case 0xC211:
			printf("dim %d,%d bounds %d,%d,%d,%d\n", w, h, ptr[0], ptr[1], ptr[2], ptr[3]);
			decodeC211(ptr + 4, imageData8, w, h);
			break;
		case 0xC103:
			printf("dim %d,%d\n", w, h);
			decodeC103(ptr, imageData8, w, h);
			break;
		}
		char path[128];
		if (count != 1) {
			snprintf(path, sizeof(path), "DUMP/%s-%d.png", name, i);
		} else {
			snprintf(path, sizeof(path), "DUMP/%s.png", name);
		}
		for (char *p = path; *p; ++p) {
			if (*p == ' ') *p = '_';
		}
		writePngImageData(path, imageData8, imageClut, w, h);
		dumpHistogram(imageData8, w * h);
		free(imageData8);
	}
}

int main(int argc, char *argv[]) {
	if (argc > 1) {
		ResourceMac res(argv[1]);
		uint8_t *ptr = decodeResourceData(res, "Flashback colors", false);
		readClut(ptr);
		free(ptr);
		char name[64];
		for (int i = 1; i <= 6; ++i) {
			snprintf(name, sizeof(name), "Title %d", i);
			uint8_t *ptr = decodeResourceData(res, name, i == 6);
			assert(ptr);
			decodeImageData(name, ptr);
			free(ptr);
		}
		for (int level = 1; level <= 5; ++level) {
			for (int i = 0; i <= 64; ++i) {
				snprintf(name, sizeof(name), "Level %d Room %d", level, i);
				uint8_t *ptr = decodeResourceData(res, name, true);
				if (ptr) {
					decodeImageData(name, ptr);
					free(ptr);
				}
			}
			snprintf(name, sizeof(name), "Objects %d", level);
			uint8_t *ptr = decodeResourceData(res, name, true);
			assert(ptr);
			decodeImageData(name, ptr);
			free(ptr);
		}
		static const char *objects[] = { "Icons", "Font", "Alien", "Junky", "Mercenary", "Replicant", "Person", 0 };
		for (int i = 0; objects[i]; ++i) {
			uint8_t *ptr = decodeResourceData(res, objects[i], true);
			assert(ptr);
			decodeImageData(objects[i], ptr);
			free(ptr);
		}
	}
	return 0;
}

