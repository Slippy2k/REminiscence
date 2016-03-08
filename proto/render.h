
#ifndef RENDER_H__
#define RENDER_H__

#ifdef USE_GLES
#include <GLES/gl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#include <sys/time.h>
#include "intern.h"
#include "game.h" // GfxImage
#include "resource_data.h" // ResourceData

static const int kW = 512;
static const int kH = 448;

struct TextureCache {

	static const int kTexturesListSize = 128;
	static const int kImagesListSize = 192;

	struct TextureHash {
		const void *ptr;
		int num;
	};

	struct Texture {
		TextureHash hash;
		GLuint texId;
		GLfloat u, v;
		int refCount;
		struct timeval timestamp;
	} _texturesList[kTexturesListSize];

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
	} _imagesList[kImagesListSize];
	int _imagesCount;

	uint16_t _tex8to565[256];
	uint16_t _tex8to5551[256];
	uint16_t *_texScaleBuf;
	struct timeval _timestamp;
	int _frameCounter;
	int _framesPerSec;

	bool _npotTex;

	TextureCache();

	void init();
	void fini();

	void updatePalette(Color *clut);
	uint16_t *rescaleTexture(DecodeBuffer *buf, int w, int h);

	void createTextureFont(ResourceData &res);
	void createTextureTitle(ResourceData &res, int num);
	void createTextureBackground(ResourceData &res, int level, int room);
	void createTextureImage(ResourceData &res, const GfxImage *image);
	void queueImageDraw(int pos, const GfxImage *image);
	void draw(bool menu, int w, int h);
	void drawImage(int i);
	void drawText(int x, int y, int color, const uint8_t *dataPtr, int len);
	void prepareFrameDraw(int w, int h);
	void endFrameDraw();
};

#endif
