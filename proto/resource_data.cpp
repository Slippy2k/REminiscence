
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "decode.h"
#include "resource_data.h"
#include "resource_mac.h"

static const int levelsColorOffset[] = { 24, 28, 36, 40, 44 }; // red palette: 32
static const int levelsIntegerIndex[] = { 1, 2, 3, 4, 4, 5, 5 };
static const char *levelsStringIndex[] = { "1", "2", "3", "4-1", "4-2", "5-1", "5-2" };

void ResourceData::setupLevelClut(int level, Color *clut) {
	const int num = levelsIntegerIndex[level] - 1;
	const int offset = levelsColorOffset[num];
	for (int i = 0; i < 4; ++i) {
		copyClut16(clut, i, offset + i);
		copyClut16(clut, 8 + i, offset + i);
	}
	copyClut16(clut, 4, 0x30);
	// 5 is monster palette
	clearClut16(clut, 6);
	copyClut16(clut, 0xA, levelsColorOffset[0] + 2);
	copyClut16(clut, 0xC, 0x37);
	// patch text color
	setClut(&clut[0xC1], 0xEE, 0xAA, 0);
	copyClut16(clut, 0xD, 0x38);
#if 0
	static const uint16_t textPal[] = {
		0x000, 0x111, 0x222, 0xEEF, 0xF00, 0xFF0, 0xEA0, 0xFB0,
		0xEA0, 0xEA0, 0xAAA, 0x0F0, 0xCCC, 0xDDF, 0xEEE, 0xEEE
	};
	setAmigaClut16(clut, 0xE, textPal);
#else
	clearClut16(clut, 0xE);
#endif
	clearClut16(clut, 0xF);
}

uint8_t *ResourceData::decodeResourceData(const char *name, bool decompressLzss) {
	uint8_t *data = 0;
	const ResourceEntry *entry = _res.findEntry(name);
	if (entry) {
		_res._f.seek(_res._dataOffset + entry->dataOffset);
		_resourceDataSize = _res._f.readUint32BE();
		printf("decodeResourceData name '%s' dataSize %d\n", name, _resourceDataSize);
		if (decompressLzss) {
			data = decodeLzss(_res._f, _resourceDataSize);
		} else {
			data = (uint8_t *)malloc(_resourceDataSize);
			_res._f.read(data, _resourceDataSize);
		}
	}
	return data;
}

enum {
	kImageOffsetsCount = 2048
};

void ResourceData::clearClut16(Color *clut, uint8_t dest) {
	memset(&clut[dest * 16], 0, 16 * sizeof(Color));
}

void ResourceData::copyClut16(Color *clut, uint8_t dest, uint8_t src) {
	memcpy(&clut[dest * 16], &_clut[src * 16], 16 * sizeof(Color));
}

void ResourceData::setAmigaClut16(Color *clut, uint8_t dest, const uint16_t *data) {
	clut += dest * 16;
	for (int i = 0; i < 16; ++i) {
		uint8_t c[3];
		for (int j = 0; j < 3; ++j) {
			const uint8_t color = (data[i] >> (j * 4)) & 15;
			c[j] = color * 16 + color;
		}
		clut[i].r = c[2];
		clut[i].g = c[1];
		clut[i].b = c[0];
	}
}

void ResourceData::setClut(Color *clut, int r, int g, int b) {
	clut->r = r;
	clut->g = g;
	clut->b = b;
}

#if 0
void decodeImageData(ResourceData &resData, const char *name, const uint8_t *ptr) {
	Color clut[256];
	memset(clut, 0, sizeof(clut));
	if (strncmp(name, "Title", 5) == 0) {
		switch (name[6] - '1') {
		case 0:
			setGrayImageClut(clut);
			break;
		case 1:
			memcpy(clut, &resData._clut[192], 192 * sizeof(Color));
			break;
		case 2:
		case 3:
		case 4:
			memcpy(clut, &resData._clut[0], 192 * sizeof(Color));
			break;
		case 5:
			setGrayImageClut(clut);
			break;
		}
	} else if (strncmp(name, "Level", 5) == 0) {
		const int num = name[6] - '1';
		assert(num >= 0 && num < 6);
		const int offset = levelsColorOffset[num];
		for (int i = 0; i < 4; ++i) {
			resData.copyClut16(clut, i, offset + i);
			resData.copyClut16(clut, 8 + i, offset + i);
		}
		resData.copyClut16(clut, 0x4, 0x30);
		resData.copyClut16(clut, 0xD, 0x38);
	} else if (strncmp(name, "Objects", 7) == 0) {
		const int num = name[8] - '1';
		assert(num >= 0 && num < 6);
		const int offset = levelsColorOffset[num];
		for (int i = 0; i < 4; ++i) {
			resData.copyClut16(clut, i, offset + i);
		}
		resData.copyClut16(clut, 0x4, 0x30);
	} else if (strcmp(name, "Icons") == 0) {
		resData.copyClut16(clut, 0xA, levelsColorOffset[0] + 2);
		resData.copyClut16(clut, 0xC, 0x37);
		resData.copyClut16(clut, 0xD, 0x38);
	} else if (strcmp(name, "Font") == 0) {
		resData.copyClut16(clut, 0xC, 0x37);
	} else if (strcmp(name, "Alien") == 0) {
		resData.copyClut16(clut, 0x5, 0x36);
	} else if (strcmp(name, "Junky") == 0) {
		resData.copyClut16(clut, 0x5, 0x32);
	} else if (strcmp(name, "Mercenary") == 0) {
		resData.copyClut16(clut, 0x5, 0x34);
	} else if (strcmp(name, "Replicant") == 0) {
		resData.copyClut16(clut, 0x5, 0x35);
	} else if (strcmp(name, "Person") == 0) {
		resData.copyClut16(clut, 0x4, 0x30); // red: 0x31
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
			decodeC211(ptr + 4, imageData8, w, h, false);
			break;
		case 0xC103:
			printf("dim %d,%d\n", w, h);
			decodeC103(ptr, imageData8, w, w, h);
			break;
		}
		free(imageData8);
	}
}
#endif

void ResourceData::decodeDataPGE(const uint8_t *ptr) {
	const uint8_t *startPtr = ptr;
	_pgeNum = READ_BE_UINT16(ptr); ptr += 2;
	memset(_pgeInit, 0, sizeof(_pgeInit));
	for (int i = 0; i < _pgeNum; ++i) {
		InitPGE *pge = &_pgeInit[i];
		pge->type = READ_BE_UINT16(ptr); ptr += 2;
		pge->pos_x = READ_BE_UINT16(ptr); ptr += 2;
		pge->pos_y = READ_BE_UINT16(ptr); ptr += 2;
		pge->obj_node_number = READ_BE_UINT16(ptr); ptr += 2;
		pge->life = READ_BE_UINT16(ptr); ptr += 2;
		for (int lc = 0; lc < 4; ++lc) {
			pge->counter_values[lc] = READ_BE_UINT16(ptr); ptr += 2;
		}
		pge->object_type = *ptr++;
		pge->init_room = *ptr++;
		pge->room_location = *ptr++;
		pge->init_flags = *ptr++;
		pge->colliding_icon_num = *ptr++;
		pge->icon_num = *ptr++;
		pge->object_id = *ptr++;
		pge->skill = *ptr++;
		pge->mirror_x = *ptr++;
		pge->flags = *ptr++;
		pge->unk1C = *ptr++;
		++ptr;
		pge->text_num = READ_BE_UINT16(ptr); ptr += 2;
	}
	fprintf(stdout, "decodeDataPGE %d\n", ptr - startPtr);
}

void ResourceData::decodeDataOBJ(const uint8_t *ptr, int unpackedSize) {
	uint32 offsets[256];
	int tmpOffset = 0;
	_numObjectNodes = READ_BE_UINT16(ptr); ptr += 2;
	for (int i = 0; i < _numObjectNodes; ++i) {
		offsets[i] = READ_BE_UINT32(ptr + tmpOffset); tmpOffset += 4;
	}
	offsets[_numObjectNodes] = unpackedSize;
	int numObjectsCount = 0;
	uint16 objectsCount[256];
	for (int i = 0; i < _numObjectNodes; ++i) {
		const int diff = offsets[i + 1] - offsets[i];
		if (diff != 0) {
			objectsCount[numObjectsCount] = (diff - 2) / 0x12;
			++numObjectsCount;
		}
	}
	uint32 prevOffset = 0;
	ObjectNode *prevNode = 0;
	int iObj = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (prevOffset != offsets[i]) {
			ObjectNode *on = (ObjectNode *)malloc(sizeof(ObjectNode));
			assert(on);
			const uint8 *objData = ptr - 2 + offsets[i];
			on->last_obj_number = READ_BE_UINT16(objData); objData += 2;
			on->num_objects = objectsCount[iObj];
			on->objects = (Object *)malloc(sizeof(Object) * on->num_objects);
			for (int j = 0; j < on->num_objects; ++j) {
				Object *obj = &on->objects[j];
				obj->type = READ_BE_UINT16(objData); objData += 2;
				obj->dx = *objData++;
				obj->dy = *objData++;
				obj->init_obj_type = READ_BE_UINT16(objData); objData += 2;
				obj->opcode2 = *objData++;
				obj->opcode1 = *objData++;
				obj->flags = *objData++;
				obj->opcode3 = *objData++;
				obj->init_obj_number = READ_BE_UINT16(objData); objData += 2;
				obj->opcode_arg1 = READ_BE_UINT16(objData); objData += 2;
				obj->opcode_arg2 = READ_BE_UINT16(objData); objData += 2;
				obj->opcode_arg3 = READ_BE_UINT16(objData); objData += 2;
//				fprintf(stdout, "obj_node=%d obj=%d op1=0x%X op2=0x%X op3=0x%X\n", i, j, obj->opcode2, obj->opcode1, obj->opcode3);
			}
			++iObj;
			prevOffset = offsets[i];
			prevNode = on;
		}
		_objectNodesMap[i] = prevNode;
	}
}

void ResourceData::decodeDataCLUT(const uint8_t *ptr) {
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
}

void ResourceData::loadClutData() {
	uint8_t *ptr = decodeResourceData("Flashback colors", false);
	decodeDataCLUT(ptr);
	free(ptr);
}

void ResourceData::loadIconData() {
	_icn = decodeResourceData("Icons", true);
}

void ResourceData::loadFontData() {
	_fnt = decodeResourceData("Font", true);
}

void ResourceData::loadPersoData() {
	_perso = decodeResourceData("Person", true);
}

void ResourceData::loadMonsterData(const char *name, Color *clut) {
	static const struct {
		const char *id;
		const char *name;
		int index;
	} data[] = {
		{ "junky", "Junky", 0x32 },
		{ "mercenai", "Mercenary", 0x34 },
		{ "replican", "Replicant", 0x35 },
		{ "glue", "Alien", 0x36 },
		{ 0, 0, 0 }
	};
	free(_monster);
	_monster = 0;
	for (int i = 0; data[i].id; ++i) {
		if (strcmp(data[i].id, name) == 0) {
			_monster = decodeResourceData(data[i].name, true);
			assert(_monster);
			copyClut16(clut, 5, data[i].index);
			break;
		}
	}
}

void ResourceData::unloadLevelData() {
	free(_ani);
	_ani = 0;
	ObjectNode *prevNode = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (prevNode != _objectNodesMap[i]) {
			free(_objectNodesMap[i]);
			prevNode = _objectNodesMap[i];
		}
	}
	_numObjectNodes = 0;
	free(_ctData);
	free(_tbn);
}

void ResourceData::loadLevelData(int i) {
	char name[64];
	// .PGE
	snprintf(name, sizeof(name), "Level %s objects", levelsStringIndex[i]);
	uint8_t *ptr = decodeResourceData(name, true);
	decodeDataPGE(ptr);
	free(ptr);
	// .ANI
	snprintf(name, sizeof(name), "Level %s sequences", levelsStringIndex[i]);
	_ani = decodeResourceData(name, true);
	assert(READ_BE_UINT16(_ani) == 0x48D);
	// .OBJ
	snprintf(name, sizeof(name), "Level %s conditions", levelsStringIndex[i]);
	ptr = decodeResourceData(name, true);
	assert(READ_BE_UINT16(ptr) == 0xE6);
	decodeDataOBJ(ptr, _resourceDataSize);
	free(ptr);
	// .CT
	snprintf(name, sizeof(name), "Level %c map", levelsStringIndex[i][0]);
	_ctData = (int8_t *)decodeResourceData(name, true);
	assert(_resourceDataSize == 0x1D00);
	// .TBN
	snprintf(name, sizeof(name), "Level %s names", levelsStringIndex[i]);
	_tbn = decodeResourceData(name, false);
}

void ResourceData::loadLevelRoom(int level, int i, DecodeBuffer *buf) {
	char name[64];
	snprintf(name, sizeof(name), "Level %d Room %d", levelsIntegerIndex[level], i);
	uint8_t *ptr = decodeResourceData(name, true);
	assert(ptr);
	if (ptr) {
		decodeImageData(ptr, 0, buf);
		free(ptr);
	}
}

void ResourceData::loadLevelObjects(int level) {
	char name[64];
	snprintf(name, sizeof(name), "Objects %d", levelsIntegerIndex[level]);
	_spc = decodeResourceData(name, true);
	assert(_spc);
}

const uint8_t *ResourceData::getImageData(const uint8_t *ptr, int i) {
	const uint8_t *basePtr = ptr;
	const uint16_t sig = READ_BE_UINT16(ptr); ptr += 2;
	assert(sig == 0xC211);
	const int count = READ_BE_UINT16(ptr); ptr += 2;
	assert(i < count);
	ptr += 4;
	const uint32_t offset = READ_BE_UINT32(ptr + i * 4);
	return (offset != 0) ? basePtr + offset : 0;
}

void ResourceData::decodeImageData(const uint8_t *ptr, int i, DecodeBuffer *dst) {
	const uint8_t *basePtr = ptr;
	const uint16_t sig = READ_BE_UINT16(ptr); ptr += 2;
	assert(sig == 0xC211 || sig == 0xC103);
	const int count = READ_BE_UINT16(ptr); ptr += 2;
	assert(i < count);
	ptr += 4;
	const uint32_t offset = READ_BE_UINT32(ptr + i * 4);
	if (offset != 0) {
		ptr = basePtr + offset;
		const int w = READ_BE_UINT16(ptr); ptr += 2;
		const int h = READ_BE_UINT16(ptr); ptr += 2;
		switch (sig) {
		case 0xC211:
			decodeC211(ptr + 4, w, h, dst);
			break;
		case 0xC103:
			decodeC103(ptr, w, h, dst);
			break;
		}
	}
}

