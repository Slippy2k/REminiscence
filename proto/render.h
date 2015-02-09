
#ifndef RENDER_H__
#define RENDER_H__

#include "intern.h"
#include "game.h" // GfxImage
#include "resource_data.h" // ResourceData

static const int kW = 512;
static const int kH = 448;

struct TextureCache {

	// TODO: freeList
	struct TextureHash {
		const void *ptr;
		int num;
	};

	struct Texture {
		TextureHash hash;
		GLuint texId;
		GLfloat u, v;
		int refCount;
		struct timeval t0;
	} _texturesList[128];

	struct {
		int num;
		GLuint texId;
		GLfloat u, v;
	} _title;

	struct {
		int level;
		int room;
		GLuint texId;
		GLfloat u, v;
	} _background;

	struct {
		int charsCount;
		GLuint texId;
		GLfloat u, v;
		uint16_t color;
	} _font;

	struct {
		Texture *tex;
		int x, y, x2, y2;
		bool xflip;
		bool erase;
	} _gfxImagesQueue[192];
	int _gfxImagesCount;

	uint16_t _tex8to565[256];
	uint16_t _tex8to5551[256];
	uint16_t *_texScaleBuf;

	TextureCache();

	void updatePalette(Color *clut);
	uint16_t *rescaleTexture(DecodeBuffer *buf, int w, int h);

	void createTextureFont(ResourceData &res);
	void createTextureTitle(ResourceData &res, int num);
	void createTextureBackground(ResourceData &res, int level, int room);
	void createTextureGfxImage(ResourceData &res, const GfxImage *image);
	void queueGfxImageDraw(int pos, const GfxImage *image);
	void draw(bool menu, int w, int h);
	void drawBackground(GLuint texId, GLfloat u, GLfloat v);
	void drawGfxImage(int i);
	void drawText(int x, int y, int color, const uint8_t *dataPtr, int len);
};

#endif
