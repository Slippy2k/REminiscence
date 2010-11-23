
#ifndef RESOURCE_DATA_H__
#define RESOURCE_DATA_H__

#include "intern.h"
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
	uint8_t *_perso;
	uint8_t *_spc;
	uint8_t *_monster;

	ResourceData(const char *filePath)
		: _res(filePath) {
		_pgeNum = 0;
		memset(_pgeInit, 0, sizeof(_pgeInit));
		_numObjectNodes = 0;
		memset(_objectNodesMap, 0, sizeof(_objectNodesMap));
		_ani = 0;
		_icn = 0;
		_perso = 0;
		_spc = 0;
		_monster = 0;
	}
	~ResourceData() {
	}

	int getPersoFrame(int anim) const {
		static const int data[] = {
			0x000, 0x22E,
			0x28E, 0x2E9,
			0x386, 0x386,
			0x4E9, 0x506,
			-1
		};
		return findAniFrame(data, anim);
	}
	int getMonsterFrame(int anim) const {
		static const int data[] = {
			0x22F, 0x28D, // junky
			0x2EA, 0x30D, // mercenai
			0x387, 0x42F, // replican
			0x430, 0x4E8, // glue
			-1
		};
		return findAniFrame(data, anim);
	}
	static int findAniFrame(const int *data, int anim) {
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

	void setupLevelClut(int num, Color *clut);
	uint8_t *decodeResourceData(const char *name, bool decompressLzss);
	void clearClut16(Color *clut, uint8_t dest);
	void copyClut16(Color *clut, uint8_t dest, uint8_t src);
	void decodeDataPGE(const uint8_t *ptr);
	void decodeDataOBJ(const uint8_t *ptr, int unpackedSize);
	void decodeDataCLUT(const uint8_t *ptr);
	void loadClutData();
	void loadIconData();
	void loadPersoData();
	void loadMonsterData(const char *name, Color *clut);
	void unloadLevelData();
	void loadLevelData(int level);
	void loadLevelRoom(int level, int i, uint8_t *dst, int dstPitch);
	void loadLevelObjects(int level);
	const uint8_t *getImageData(const uint8_t *ptr, int i);
	void decodeImageData(const uint8_t *ptr, int i, uint8_t *dst, int dstPitch, bool xflip = false);
};

void decodeImageData(ResourceData &resData, const char *name, const uint8_t *ptr);

#endif

