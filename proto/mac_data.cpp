
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <SDL_opengl.h>
#include "decode.h"
#include "mac_data.h"
#include "resource_mac.h"

enum {
	kClutSize = 1024
};

static Color _clut[kClutSize];
static int _clutSize;

Color *getClut() {
	return _clut;
}

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

uint8_t *decodeResourceData(ResourceMac &res, const char *name, bool decompressLzss) {
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

int decodeImageData(const char *name, const uint8_t *ptr) {
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
	int ret = -1;
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
		if (count == 1 && i == 0) {
			GLuint textureId;
			glGenTextures(1, &textureId);
			uint8_t *texData = (uint8_t *)malloc(w * h * 3);
			for (int y = 0; y < h; ++y) {
				for (int x = 0; x < w; ++x) {
					const Color &c = imageClut[imageData8[y * w + x]];
					texData[(y * w + x) * 3] = c.r;
					texData[(y * w + x) * 3 + 1] = c.g;
					texData[(y * w + x) * 3 + 2] = c.b;
				}
			}
			glBindTexture(GL_TEXTURE_2D, textureId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
			ret = textureId;
			free(texData);
		}
		free(imageData8);
	}
	return ret;
}

void checkAmigaData(ResourceMac &res) {
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

void decodeClutData(ResourceMac &res) {
	uint8_t *ptr = decodeResourceData(res, "Flashback colors", false);
	readClut(ptr);
	free(ptr);
}

#if 0
int main(int argc, char *argv[]) {
	if (argc > 1) {
		ResourceMac res(argv[1]);
		decodeSoundData(res);
		checkAmigaData(res);
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
#endif

