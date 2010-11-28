
#ifndef RESOURCE_DATA_H__
#define RESOURCE_DATA_H__

#include "intern.h"
#include "decode.h"
#include "resource_mac.h"

struct Color {
	int r, g, b;
};

struct ResourceData {
	enum {
		kClutSize = 1024
	};

	ResourceMac _res;

	uint32_t _resourceDataSize;
	int _clutSize;
	Color _clut[kClutSize];
	int _pgeNum;
	InitPGE _pgeInit[256];
	int _numObjectNodes;
	ObjectNode *_objectNodesMap[255];
	int8 *_ctData;
	uint8_t *_ani;
	uint8_t *_icn;
	uint8_t *_fnt;
	uint8_t *_perso;
	uint8_t *_spc;
	uint8_t *_monster;
	uint8_t *_tbn;

	ResourceData(const char *filePath)
		: _res(filePath) {
		_pgeNum = 0;
		memset(_pgeInit, 0, sizeof(_pgeInit));
		_numObjectNodes = 0;
		memset(_objectNodesMap, 0, sizeof(_objectNodesMap));
		_ani = 0;
		_icn = 0;
		_fnt = 0;
		_perso = 0;
		_spc = 0;
		_monster = 0;
		_tbn = 0;
	}
	~ResourceData() {
	}

	int getPersoFrame(int anim) const {
		static const int data[] = {
			0x000, 0x22E,
			0x28E, 0x2E9,
			0x4E9, 0x506,
			-1
		};
		int offset = 0;
		for (int i = 0; data[i] != -1; i += 2) {
			if (anim >= data[i] && anim <= data[i + 1]) {
				return offset + anim - data[i];
			}
			const int count = data[i + 1] + 1 - data[i];
			offset += count;
		}
		assert(0);
		return 0;
	}
	int getMonsterFrame(int anim) const {
		static const int data[] = {
			0x22F, 0x28D, // junky - 94
			0x2EA, 0x387, // mercenai - 156
			0x387, 0x42F, // replican - 169
			0x430, 0x4E8, // glue - 185
			-1
		};
		for (int i = 0; data[i] != -1; i += 2) {
			if (anim >= data[i] && anim <= data[i + 1]) {
				return anim - data[i];
			}
		}
		assert(0);
		return 0;
	}
	const uint8_t *getAniData(int i) const {
		const uint32_t offset = READ_BE_UINT16(_ani + 2 + i * 2);
		return _ani + offset;
	}
	const uint8_t *getStringData(int i) {
		const int count = READ_BE_UINT16(_tbn);
		assert(i < count);
		return _tbn + READ_BE_UINT16(_tbn + 2 + i * 2);
	}

	void setupLevelClut(int num, Color *clut);
	uint8_t *decodeResourceData(const char *name, bool decompressLzss);
	void clearClut16(Color *clut, uint8_t dest);
	void copyClut16(Color *clut, uint8_t dest, uint8_t src);
	void setAmigaClut16(Color *clut, uint8_t dest, const uint16_t *data);
	void setClut(Color *clut, int r, int g, int b);
	void decodeDataPGE(const uint8_t *ptr);
	void decodeDataOBJ(const uint8_t *ptr, int unpackedSize);
	void decodeDataCLUT(const uint8_t *ptr);
	void loadClutData();
	void loadIconData();
	void loadFontData();
	void loadPersoData();
	void loadMonsterData(const char *name, Color *clut);
	void unloadLevelData();
	void loadLevelData(int level);
	void loadLevelRoom(int level, int i, DecodeBuffer *dst);
	void loadLevelObjects(int level);
	const uint8_t *getImageData(const uint8_t *ptr, int i);
	void decodeImageData(const uint8_t *ptr, int i, DecodeBuffer *dst);
};

#endif

