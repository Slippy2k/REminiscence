
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

	int _clutSize;
	Color _clut[kClutSize];
	int _pgeNum;
	InitPGE _pgeInit[256];
	int _numObjectNodes;
	ObjectNode *_objectNodesMap[255];
	int8 _ctData[0x1D00];
	uint8_t *_ani;
	uint8_t *_icn;
	uint8_t *_perso;
	uint8_t *_spc;

	ResourceData(const char *filePath)
		: _res(filePath) {
	}

	int getSpriteFrame(int i) const {
// FIXME
		if (i < 0x230) return i;
		if (i < 0x28E) return i - 0x22F;
		return i - 0x28E + 0x230;
	}
	const uint8_t *getAniData(int i) const {
		const uint32_t offset = READ_BE_UINT16(_ani + 2 + i * 2);
		return _ani + offset;
	}

	void setupLevelClut(int num, Color *clut);
	uint8_t *decodeResourceData(const char *name, bool decompressLzss);
	void copyClut16(Color *imageClut, uint8_t dest, uint8_t src);
	void decodeDataPGE(const uint8_t *ptr);
	void decodeDataOBJ(const uint8_t *ptr, int unpackedSize);
	void decodeDataCLUT(const uint8_t *ptr);
	void loadClutData();
	void loadIconData();
	void loadPersoData();
	void loadLevelData(int level);
	void loadLevelRoom(int level, int i, uint8_t *dst, int dstPitch);
	void loadLevelObjects(int level);
	const uint8_t *getImageData(const uint8_t *ptr, int i);
	void decodeImageData(const uint8_t *ptr, int i, uint8_t *dst, int dstPitch);
};

void decodeImageData(ResourceData &resData, const char *name, const uint8_t *ptr);

#endif

