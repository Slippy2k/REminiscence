
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "decode.h"
#include "resource_data.h"
#include "resource_mac.h"

static const int levelsColorOffset[] = { 24, 28, 36, 40, 44 }; // red palette: 32
static const int levelsIntegerIndex[] = { 1, 2, 3, 4, 4, 5, 5 };
static const char *levelsStringIndex[] = { "1", "2", "3", "4-1", "4-2", "5-1", "5-2" };

void ResourceData::setupTitleClut(int num, Color *clut) {
	switch (num) {
	case 1:
		memcpy(clut, &_clut[192], 192 * sizeof(Color));
		break;
	case 2:
	case 3:
	case 4:
		memcpy(clut, &_clut[0], 192 * sizeof(Color));
		break;
	}
}

void ResourceData::setupRoomClut(int level, int room, Color *clut) {
	const int num = levelsIntegerIndex[level] - 1;
	int offset = levelsColorOffset[num];
	if (level == 1) {
		switch (room) {
		case 27:
		case 28:
		case 29:
		case 30:
		case 35:
		case 36:
		case 37:
		case 45:
		case 46:
			offset = 32;
			break;
		}
	}
	for (int i = 0; i < 4; ++i) {
		copyClut16(clut, i, offset + i);
		copyClut16(clut, 8 + i, offset + i);
	}
	copyClut16(clut, 4, 0x30);
	// 5 is monster palette
	clearClut16(clut, 6);
	copyClut16(clut, 0xA, levelsColorOffset[0] + 2);
	copyClut16(clut, 0xC, 0x37);
	copyClut16(clut, 0xD, 0x38);
}

void ResourceData::setupTextClut(Color *clut) {
	static const uint16_t textPal[] = {
		0x000, 0x111, 0x222, 0xEEF, 0xF00, 0xFF0, 0xEA0, 0xFB0,
		0xEA0, 0xEA0, 0xAAA, 0x0F0, 0xCCC, 0xDDF, 0xEEE, 0xEEE
	};
	setAmigaClut16(clut, 0xE, textPal);
	clearClut16(clut, 0xF);
}

uint8_t *ResourceData::decodeResourceData(const char *name, bool decompressLzss) {
	uint8_t *data = 0;
	const ResourceEntry *entry = _res.findEntry(name);
	if (entry) {
		_res._f.seek(_res._dataOffset + entry->dataOffset);
		_resourceDataSize = _res._f.readUint32BE();
//		fprintf(stdout, "decodeResourceData name '%s' dataSize %d\n", name, _resourceDataSize);
		if (decompressLzss) {
			data = decodeLzss(_res._f, _resourceDataSize);
		} else {
			data = (uint8_t *)malloc(_resourceDataSize);
			if (data) {
				_res._f.read(data, _resourceDataSize);
			}
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

void ResourceData::decodeDataPGE(const uint8_t *ptr) {
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
}

void ResourceData::decodeDataOBJ(const uint8_t *ptr, int unpackedSize) {
	uint32_t offsets[256];
	int tmpOffset = 0;
	_numObjectNodes = READ_BE_UINT16(ptr); ptr += 2;
	for (int i = 0; i < _numObjectNodes; ++i) {
		offsets[i] = READ_BE_UINT32(ptr + tmpOffset); tmpOffset += 4;
	}
	offsets[_numObjectNodes] = unpackedSize;
	int numObjectsCount = 0;
	uint16_t objectsCount[256];
	for (int i = 0; i < _numObjectNodes; ++i) {
		const int diff = offsets[i + 1] - offsets[i];
		if (diff != 0) {
			objectsCount[numObjectsCount] = (diff - 2) / 0x12;
			++numObjectsCount;
		}
	}
	uint32_t prevOffset = 0;
	ObjectNode *prevNode = 0;
	int iObj = 0;
	for (int i = 0; i < _numObjectNodes; ++i) {
		if (prevOffset != offsets[i]) {
			ObjectNode *on = (ObjectNode *)malloc(sizeof(ObjectNode));
			assert(on);
			const uint8_t *objData = ptr - 2 + offsets[i];
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

void ResourceData::loadTitleImage(int i, DecodeBuffer *buf) {
	char name[64];
	snprintf(name, sizeof(name), "Title %d", i);
	uint8_t *ptr = decodeResourceData(name, (i == 6));
	if (ptr) {
		decodeImageData(ptr, 0, buf);
		free(ptr);
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
	_ctData = 0;
	free(_tbn);
	_tbn = 0;
	free(_str);
	_str = 0;
	for (int i = 0; i < ARRAYSIZE(_sounds); ++i) {
		free(_sounds[i]);
		_sounds[i] = 0;
	}
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
	_str = decodeResourceData("Flashback strings", false);
}

void ResourceData::loadLevelRoom(int level, int i, DecodeBuffer *buf) {
	char name[64];
	snprintf(name, sizeof(name), "Level %d Room %d", levelsIntegerIndex[level], i);
	uint8_t *ptr = decodeResourceData(name, true);
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

static uint8_t *readFile(const char *path) {
	uint8_t *buf = 0;
	File f;
	if (f.open(path, "rb")) {
		const int fileSize = f.size();
		buf = (uint8_t *)malloc(fileSize);
		if (buf) {
			f.read(buf, fileSize);
		}
	}
	return buf;
}

static void LE32(uint8_t *p, uint32_t value) {
	for (int i = 0; i < 4; ++i) {
		p[i] = (value >> (8 * i)) & 255;
	}
}

static uint8_t *convertToWav(const uint8_t *data, int freq, int size) {
	static const uint8_t kHeaderData[] = {
		0x52, 0x49, 0x46, 0x46, 0x24, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, // RIFF$...WAVEfmt 
		0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x7d, 0x00, 0x00, // .........}...}..
		0x01, 0x00, 0x08, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00                          // ....data....
	};
	uint8_t *out = (uint8_t *)malloc(sizeof(kHeaderData) + size);
	if (out) {
		memcpy(out, kHeaderData, sizeof(kHeaderData));
		LE32(out + 4, 36 + size); // 'RIFF' size
		LE32(out + 24, freq); // 'fmt ' sample rate
		LE32(out + 28, freq); // 'fmt ' bytes per second
		LE32(out + 40, size); // 'data' size
		memcpy(out + sizeof(kHeaderData), data, size);
	}
	return out;
}

uint8_t *ResourceData::getSoundData(int i, int *freq, uint32_t *size) {
	if (!_sounds[i]) {
		char name[16];
		snprintf(name, sizeof(name), "s%02d.wav", i);
		_sounds[i] = readFile(name);
	}
	static const bool kConvertToWav = true;
	static const int kHeaderSize = 0x24;
	assert(i >= 0 && i < ARRAYSIZE(_sounds));
	if (!_sounds[i]) {
		static const int kSoundType = 4;
		assert(i >= 0 && i < _res._types[kSoundType].count);
		const ResourceEntry *entry = &_res._entries[kSoundType][i];
		_res._f.seek(_res._dataOffset + entry->dataOffset);
		const int dataSize = _res._f.readUint32BE();
		assert(dataSize > kHeaderSize);
		uint8_t *p = (uint8_t *)malloc(dataSize);
		if (p) {
			_res._f.read(p, dataSize);
			if (kConvertToWav) {
				const int len = READ_BE_UINT32(p + 0x12);
				const int rate = READ_BE_UINT16(p + 0x16);
				_sounds[i] = convertToWav(p + kHeaderSize, rate, len);
				free(p);
			} else {
				_sounds[i] = p;
			}
		}
	}
	uint8_t *p = _sounds[i];
	if (p && memcmp(p, "RIFF", 4) != 0) {
		if (freq) {
			*freq = READ_BE_UINT16(p + 0x16);
		}
		if (size) {
			*size = READ_BE_UINT32(p + 0x12);
		}
		p += kHeaderSize;
	}
	return p;
}

const char *ResourceData::getSfxName(int num) const {
	switch (num) {
	case 69:
		num = 68;
		break;
	case 71:
		num = 70;
		break;
	}
	static char name[16];
	snprintf(name, sizeof(name), "soundfx%d.ogg", num);
	return name;
}

uint8_t *ResourceData::getSfxData(int num) {
	switch (num) {
	case 69:
		num = 68;
		break;
	case 71:
		num = 70;
		break;
	}
	char name[16];
	snprintf(name, sizeof(name), "soundfx%d.wav", num);
	return readFile(name);
}

uint8_t *ResourceData::getVoiceSegment(int num, int segment) {
	char name[16];
	snprintf(name, sizeof(name), "v%02d_%02d.wav", num, segment);
	return readFile(name);
}
