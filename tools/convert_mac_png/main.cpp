
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <png.h>
#include "resource.h"
#include "decode.h"
extern "C" {
#include "scalebit.h"
}

struct Scaler {
	const char *name;
	int scale;
} _scalers[] = {
	{ "scale2x", 2 },
	{ "scale3x", 3 },
	{ "scale4x", 4 },
	{ 0, 0 }
};

struct Color {
	int r, g, b;
};

enum {
	kClutSize = 1024
};

static Color _clut[kClutSize];
static int _clutSize;

static int _upscaleImageData = 1;
static uint8_t *_upscaleImageDataBuf;

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

static uint32_t getResourceDataSize(ResourceMac &res, const ResourceEntry *entry, bool lzss) {
	res._f.seek(res._dataOffset + entry->dataOffset);
	uint32_t size = res._f.readUint32BE();
	if (lzss) {
		size = res._f.readUint32BE();
	}
	return size;
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
	if (_upscaleImageData != 1) {
		const int scaledW = w * _upscaleImageData;
		const int scaledH = h * _upscaleImageData;
		_upscaleImageDataBuf = (uint8_t *)malloc(scaledW * scaledH);
		if (_upscaleImageDataBuf) {
			scale(_upscaleImageData, _upscaleImageDataBuf, scaledW, imageData, w, 1, w, h);
			imageData = _upscaleImageDataBuf;
			w = scaledW;
			h = scaledH;
		}
	}
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
	if (_upscaleImageDataBuf) {
		free(_upscaleImageDataBuf);
		_upscaleImageDataBuf = 0;
	}
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
			printf("dim %d,%d offset %d,%d\n", w, h, (int16_t)READ_BE_UINT16(ptr), (int16_t)READ_BE_UINT16(ptr + 2));
			decodeC211(ptr + 4, imageData8, w, h);
			break;
		case 0xC103:
			printf("dim %d,%d\n", w, h);
			decodeC103(ptr, imageData8, w, h);
			break;
		}
		char path[128];
		if (count != 1) {
			snprintf(path, sizeof(path), "DUMP/%s-%03d.png", name, i);
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

static void checkAmigaData(ResourceMac &res) {
	static const char *levels[] = { "1", "2", "3", "4-1", "4-2", "5-1", "5-2" };
	char name[64];
	for (int i = 0; i < 7; ++i) {
		// .PGE
		snprintf(name, sizeof(name), "Level %s objects", levels[i]);
		uint8_t *ptr = decodeResourceData(res, name, true);
		assert(ptr);
		static const int pgeCount[] = { 0x6B, 0x100, 0xB0, 0xBE, 0x5A, 0x6D, 0xA9 };
		assert(READ_BE_UINT16(ptr) == pgeCount[i]);
		free(ptr);
		// .ANI
		snprintf(name, sizeof(name), "Level %s sequences", levels[i]);
		ptr = decodeResourceData(res, name, true);
		assert(ptr);
		assert(READ_BE_UINT16(ptr) == 0x48D);
		free(ptr);
		// .OBJ
		snprintf(name, sizeof(name), "Level %s conditions", levels[i]);
		ptr = decodeResourceData(res, name, true);
		assert(ptr);
		assert(READ_BE_UINT16(ptr) == 0xE6);
		free(ptr);
	}
	for (int i = 0; i < 5; ++i) {
		// .CT
		snprintf(name, sizeof(name), "Level %d map", i + 1);
		uint8_t *ptr = decodeResourceData(res, name, true);
		assert(ptr);
//		assert(ptrDataSize == 0x1D00);
		free(ptr);
	}
}

static void decodeSoundData(ResourceMac &res) {
	const unsigned char id[4] = { 's','n','d',' ' };
	const int type = res.findTypeById(id);
	for (int i = 0; i < res._types[type].count; ++i) {
		const ResourceEntry *entry = &res._entries[type][i];
		res._f.seek(res._dataOffset + entry->dataOffset);
		const uint32_t dataSize = res._f.readUint32BE();
		uint8_t *data = (uint8_t *)malloc(dataSize);
		res._f.read(data, dataSize);
		static const int kHeaderSize = 0x24;
		printf("Sound %d size %d (%d)\n", i, dataSize, dataSize - kHeaderSize);
		assert(dataSize > kHeaderSize + 2);
		assert(READ_BE_UINT32(data) == 0x20000);
		assert(READ_BE_UINT32(data + 4) == 98384);
		assert(READ_BE_UINT32(data + 8) == 0);
		assert(READ_BE_UINT16(data + 0xC) == 0xE);
		assert(READ_BE_UINT32(data + 0xE) == 0);
		const int unk12 = READ_BE_UINT32(data + 0x12);
		const int rate = READ_BE_UINT16(data + 0x16);
		assert(READ_BE_UINT16(data + 0x18) == 0);
		const int sampleLen = READ_BE_UINT32(data + 0x1A);
		const int unk1E = READ_BE_UINT32(data + 0x1E);
		const int unk22 = READ_BE_UINT16(data + 0x22);
		if (rate != 11025 && rate != 22050) {
			assert(unk12 == unk1E);
			assert(READ_BE_UINT16(data + 0x22) == 60);
		}
		if (i <= 58) { // dump game sounds
			File f;
			char tmpPath[128];
			snprintf(tmpPath, sizeof(tmpPath), "DUMP/%03d.raw", i);
			if (f.open(tmpPath, "wb")) {
				f.write(data + kHeaderSize, sampleLen);
				f.close();
			}
			char path[128];
			snprintf(path, sizeof(path), "DUMP/s%02d.wav", i);
			char cmd[256];
			snprintf(cmd, sizeof(cmd), "sox -r %d -e unsigned -b 8 -c 1 %s %s rate 11025", rate, tmpPath, path);
			system(cmd);
			unlink(tmpPath);
		}
		free(data);
	}
}

static void checkCutsceneData(ResourceMac &res) {
	static const struct {
		const char *name;
		uint16_t cmdSize;
		uint16_t polSize;
	} fileSizes[] = {
		// file sizes from the DOS version
		{ "intro1", 3720, 57813 },
		{ "intro2", 11099, 32699 },
		{ "holoseq", 5807, 7280 },
		{ "voyage", 21881, 26275 },
		{  0, 0, 0 }
	};
	for (int i = 0; fileSizes[i].name; ++i) {
		char name[32];
		snprintf(name, sizeof(name), "%s movie", fileSizes[i].name);
		const ResourceEntry *entry = res.findEntry(name);
		assert(entry && getResourceDataSize(res, entry, true) == fileSizes[i].cmdSize + 2);
		snprintf(name, sizeof(name), "%s polygons", fileSizes[i].name);
		entry = res.findEntry(name);
		assert(entry && getResourceDataSize(res, entry, true) == fileSizes[i].polSize);
	}
}

int main(int argc, char *argv[]) {
	while (1) {
		static struct option options[] = {
			{ "scaler", required_argument, 0, 's' },
			{ 0, 0, 0, 0 }
		};
		int index;
		const int c = getopt_long(argc, argv, "", options, &index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 's':
			for (int i = 0; _scalers[i].name; ++i) {
				if (strcmp(_scalers[i].name, optarg) == 0) {
					_upscaleImageData = _scalers[i].scale;
					break;
				}
			}
			break;
		}
	}
	if (optind < argc) {
		ResourceMac res(argv[optind]);
		// decodeSoundData(res);
		checkAmigaData(res);
		checkCutsceneData(res);
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
				snprintf(name, sizeof(name), "Level %d Room %02d", level, i);
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

