
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

static bool _upscaleImageData2x = true;
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

static void scale2x(uint8_t *dst, int dstPitch, const uint8_t *src, int srcPitch, int w, int h) {
	const int dstPitch2 = dstPitch * 2;
	while (h--) {
		uint8_t *p = dst;
		uint8_t D = *(src - 1);
		uint8_t E = *(src);
		for (int i = 0; i < w; ++i, p += 2) {
			uint8_t B = *(src + i - srcPitch);
			uint8_t F = *(src + i + 1);
			uint8_t H = *(src + i + srcPitch);
			if (B != H && D != F) {
				*(p) = D == B ? D : E;
				*(p + 1) = B == F ? F : E;
				*(p + dstPitch) = D == H ? D : E;
				*(p + dstPitch + 1) = H == F ? F : E;
			} else {
				*(p) = E;
				*(p + 1) = E;
				*(p + dstPitch) = E;
				*(p + dstPitch + 1) = E;
			}
			D = E;
			E = F;
		}
		dst += dstPitch2;
		src += srcPitch;
	}
}

static void writePngImageData(const char *filePath, const uint8_t *imageData, Color *imageClut, int w, int h) {
	if (_upscaleImageData2x) {
		_upscaleImageDataBuf = (uint8_t *)malloc((w * 2) * (h * 2));
		if (_upscaleImageDataBuf) {
			uint8_t *imageDataDup = (uint8_t *)malloc((w + 2) * (h + 2));
			if (imageDataDup) {
				const int srcPitch = w + 2;
				memcpy(imageDataDup, imageData, w);
				for (int y = 0; y < h; ++y) {
					const int yOffset = (y + 1) * srcPitch;
					imageDataDup[yOffset] = imageData[y * w];
					memcpy(imageDataDup + yOffset + 1, imageData + y * w, w);
					imageDataDup[yOffset + w + 1] = imageData[y * w + w - 1];
				}
				memcpy(imageDataDup + (h + 1) * srcPitch, imageData + (h - 1) * w, w);
				scale2x(_upscaleImageDataBuf, w * 2, imageDataDup + srcPitch + 1, srcPitch, w, h);
				free(imageDataDup);
			}
			imageData = _upscaleImageDataBuf;
			w *= 2;
			h *= 2;
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
		printf("Sound %d size %d\n", i, dataSize);
		File f;
		char tmpPath[128];
		snprintf(tmpPath, sizeof(tmpPath), "DUMP/%03d.raw", i);
		f.open(tmpPath, "wb");
		f.write(data, dataSize);
		free(data);
		char path[128];
		snprintf(path, sizeof(path), "DUMP/%03d.wav", i);
		char cmd[256];
		const int rate = 3546897 / 650;
		snprintf(cmd, sizeof(cmd), "sox -r %d -e unsigned -b 8 -c 1 %s %s", rate, tmpPath, path);
		system(cmd);
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
	if (argc > 1) {
		ResourceMac res(argv[1]);
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

