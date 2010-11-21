
#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "intern.h"

struct Resource;
struct SystemStub;

struct Video {
	enum {
		GAMESCREEN_W = 256,
		GAMESCREEN_H = 224,
		SCREENBLOCK_W = 8,
		SCREENBLOCK_H = 8,
		CHAR_W = 8,
		CHAR_H = 8
	};

	static const uint8 _conradPal1[];
	static const uint8 _conradPal2[];
	static const uint8 _textPal[];
	static const uint8 _palSlot0xF[];

	Resource *_res;
	SystemStub *_stub;

	uint8 *_frontLayer;
	uint8 *_backLayer;
	uint8 *_tempLayer;
	uint8 *_tempLayer2;
	uint8 _unkPalSlot1, _unkPalSlot2;
	uint8 _mapPalSlot1, _mapPalSlot2, _mapPalSlot3, _mapPalSlot4;
	uint8 _charFrontColor;
	uint8 _charTransparentColor;
	uint8 _charShadowColor;
	uint8 *_screenBlocks;
	bool _fullRefresh;
	uint8 _shakeOffset;

	Video(Resource *res, SystemStub *stub);
	~Video();

	void markBlockAsDirty(int16 x, int16 y, uint16 w, uint16 h);
	void updateScreen();
	void fullRefresh();
	void fadeOut();
	void fadeOutPalette();
	void setPaletteSlotBE(int palSlot, int palNum);
	void setPaletteSlotLE(int palSlot, const uint8 *palData);
	void setTextPalette();
	void setPalette0xF();
	void copyLevelMap(int level, int room);
	void decodeLevelMap(uint16 sz, const uint8 *src, uint8 *dst);
	void setLevelPalettes();
	void drawSpriteSub1(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask);
	void drawSpriteSub2(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask);
	void drawSpriteSub3(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask);
	void drawSpriteSub4(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask);
	void drawSpriteSub5(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask);
	void drawSpriteSub6(const uint8 *src, uint8 *dst, int pitch, int h, int w, uint8 colMask);
	void drawChar(uint8 c, int16 y, int16 x);
	const char *drawString(const char *str, int16 x, int16 y, uint8 col);
};

#endif // __VIDEO_H__
