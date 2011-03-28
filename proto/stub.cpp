
#ifdef USE_GLES
#include <GLES/gl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#ifdef _WIN32
#include <windows.h>
#define DYNLIB_SYMBOL DECLSPEC_EXPORT
#else
#define DYNLIB_SYMBOL
#endif
#include <math.h>
#include <sys/time.h>
#include "game.h"
#include "resource_data.h"
#include "scaler.h"
#include "stub.h"

static const int kW = 512;
static const int kH = 448;

static const int _scalerFactor = 2;
static void (*_scalerProc)(uint16_t *, int, const uint16_t *, int, int, int) = scale2x;

static int roundPowerOfTwo(int x) {
	if ((x & (x - 1)) == 0) {
		return x;
	} else {
		int num = 1;
		while (num < x) {
			num <<= 1;
		}
		return num;
	}
}

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

	TextureCache() {
		memset(&_title, 0, sizeof(_title));
		_title.texId = -1;
		memset(&_background, 0, sizeof(_background));
		_background.texId = -1;
		memset(&_font, 0, sizeof(_font));
		_font.texId = -1;
		memset(&_texturesList, 0, sizeof(_texturesList));
		for (int i = 0; i < ARRAYSIZE(_texturesList); ++i) {
			_texturesList[i].texId = -1;
		}
		memset(&_gfxImagesQueue, 0, sizeof(_gfxImagesQueue));
		_gfxImagesCount = 0;
		if (_scalerFactor != 1) {
			const int w = roundPowerOfTwo(kW * _scalerFactor);
			const int h = roundPowerOfTwo(kH * _scalerFactor);
			_texScaleBuf = (uint16_t *)malloc(w * h * sizeof(uint16_t));
		}
	}

	void updatePalette(Color *clut) {
		for (int i = 0; i < 256; ++i) {
			_tex8to565[i] = ((clut[i].r >> 3) << 11) | ((clut[i].g >> 2) << 5) | (clut[i].b >> 3);
			_tex8to5551[i] = ((clut[i].r >> 3) << 11) | ((clut[i].g >> 3) << 6) | ((clut[i].b >> 3) << 1);
		}
	}

	uint16_t *rescaleTexture(DecodeBuffer *buf, int w, int h) {
		if (_scalerFactor == 1) {
			return (uint16_t *)buf->ptr;
		} else {
			_scalerProc(_texScaleBuf, w, (const uint16_t *)buf->ptr, buf->pitch, buf->w, buf->h);
			return _texScaleBuf;
		}
	}

	static void convertTextureFont(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
		const int offset = (y * buf->pitch + buf->x + x) * sizeof(uint16_t);
		switch (color) {
		case 0xC0:
			*(uint16_t *)(buf->ptr + offset) = 1;
			break;
		case 0xC1:
			*(uint16_t *)(buf->ptr + offset) = 0xFFFF;
			break;
		}
	}

	void createTextureFont(ResourceData &res) {
		_font.charsCount = 105;
		const int w = _font.charsCount * 16 * _scalerFactor;
		const int h = 16 * _scalerFactor;
		const int texW = roundPowerOfTwo(w);
		const int texH = roundPowerOfTwo(h);
		if (_font.texId == -1) {
			glGenTextures(1, &_font.texId);
			glBindTexture(GL_TEXTURE_2D, _font.texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
			_font.u = w / (GLfloat)texW;
			_font.v = h / (GLfloat)texH;
		}
		DecodeBuffer buf;
		memset(&buf, 0, sizeof(buf));
		if (_scalerFactor == 1) {
			buf.w = buf.pitch = texW;
			buf.h = texH;
		} else {
			buf.w = buf.pitch = _font.charsCount * 16;
			buf.h = 16;
		}
		buf.setPixel = convertTextureFont;
		buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
		if (buf.ptr) {
			for (int i = 0; i < _font.charsCount; ++i) {
				res.decodeImageData(res._fnt, i, &buf);
				buf.x += 16;
			}
			const uint16_t *texData = rescaleTexture(&buf, texW, texH);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texData);
			free(buf.ptr);
		}
	}

	static void convertTextureTitle(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
		const uint16_t *lut = (const uint16_t *)buf->lut;
		const int offset = (y * buf->pitch + x) * sizeof(uint16_t);
		*(uint16_t *)(buf->ptr + offset) = lut[color];
	}

	void createTextureTitle(ResourceData &res, int num) {
		const int w = kW * _scalerFactor;
		const int h = kH * _scalerFactor;
		const int texW = roundPowerOfTwo(w);
		const int texH = roundPowerOfTwo(h);
		if (_title.texId == -1) {
			glGenTextures(1, &_title.texId);
			glBindTexture(GL_TEXTURE_2D, _title.texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
			_title.u = w / (GLfloat)texW;
			_title.v = h / (GLfloat)texH;
		}
		DecodeBuffer buf;
		memset(&buf, 0, sizeof(buf));
		if (_scalerFactor == 1) {
			buf.w = buf.pitch = texW;
			buf.h = texH;
		} else {
			buf.w = buf.pitch = kW;
			buf.h = kH;
		}
		buf.lut = _tex8to565;
		buf.setPixel = convertTextureTitle;
		buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
		if (buf.ptr) {
			res.loadTitleImage(num, &buf);
			glBindTexture(GL_TEXTURE_2D, _title.texId);
			const uint16_t *texData = rescaleTexture(&buf, texW, texH);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, texData);
			free(buf.ptr);
		}
	}

	static void convertTextureBackground(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
		const uint16_t *lut = (const uint16_t *)buf->lut;
		const int offset = (y * buf->pitch + x) * sizeof(uint16_t);
		*(uint16_t *)(buf->ptr + offset) = lut[color] | (color >> 7);
	}

	void createTextureBackground(ResourceData &res, int level, int room) {
		const int w = kW * _scalerFactor;
		const int h = kH * _scalerFactor;
		const int texW = roundPowerOfTwo(w);
		const int texH = roundPowerOfTwo(h);
		if (_background.texId == -1) {
			glGenTextures(1, &_background.texId);
			glBindTexture(GL_TEXTURE_2D, _background.texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
			_background.u = w / (GLfloat)texW;
			_background.v = h / (GLfloat)texH;
		}
		if (level != _background.level || room != _background.room) {
			DecodeBuffer buf;
			memset(&buf, 0, sizeof(buf));
			if (_scalerFactor == 1) {
				buf.w = buf.pitch = texW;
				buf.h = texH;
			} else {
				buf.w = buf.pitch = kW;
				buf.h = kH;
			}
			buf.lut = _tex8to5551;
			buf.setPixel = convertTextureBackground;
			buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
			if (buf.ptr) {
				res.loadLevelRoom(level, room, &buf);
				glBindTexture(GL_TEXTURE_2D, _background.texId);
				const uint16_t *texData = rescaleTexture(&buf, texW, texH);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texData);
				free(buf.ptr);
			}
			_background.level = level;
			_background.room = room;
		}
	}

	static void convertTextureImage(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
		const uint16_t *lut = (const uint16_t *)buf->lut;
		const int offset = (y * buf->pitch + x) * sizeof(uint16_t);
		*(uint16_t *)(buf->ptr + offset) = lut[color] | 1;
	}

	void createTextureGfxImage(ResourceData &res, const GfxImage *image) {
		const int w = image->w * _scalerFactor;
		const int h = image->h * _scalerFactor;
		const int texW = roundPowerOfTwo(w);
		const int texH = roundPowerOfTwo(h);
		int pos = -1;
		for (int i = 0; i < ARRAYSIZE(_texturesList); ++i) {
			if (_texturesList[i].texId == -1) {
				pos = i;
			} else if (_texturesList[i].hash.ptr == image->dataPtr && _texturesList[i].hash.num == image->num) {
				queueGfxImageDraw(i, image);
				return;
			}
		}
		if (pos == -1) {
			struct timeval t0;
			gettimeofday(&t0, 0);
			int max = 0;
			for (int i = 0; i < ARRAYSIZE(_texturesList); ++i) {
				if (_texturesList[i].refCount == 0) {
					const int d = (t0.tv_sec - _texturesList[i].t0.tv_sec) * 1000 + (t0.tv_usec - _texturesList[i].t0.tv_usec) / 1000;
					if (i == 0 || d > max) {
						pos = i;
						max = d;
					}
				}
			}
			if (pos == -1) {
				printf("No free slot for texture cache\n");
				return;
			}
			glDeleteTextures(1, &_texturesList[pos].texId);
			memset(&_texturesList[pos], 0, sizeof(Texture));
			_texturesList[pos].texId = -1;
		}
		if (_texturesList[pos].texId == -1) {
			glGenTextures(1, &_texturesList[pos].texId);
			glBindTexture(GL_TEXTURE_2D, _texturesList[pos].texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
                }
		_texturesList[pos].u = w / (GLfloat)texW;
		_texturesList[pos].v = h / (GLfloat)texH;
		DecodeBuffer buf;
		memset(&buf, 0, sizeof(buf));
		if (_scalerFactor == 1) {
			buf.w = buf.pitch = texW;
			buf.h = texH;
		} else {
			buf.w = buf.pitch = image->w;
			buf.h = image->h;
		}
		buf.lut = _tex8to5551;
		buf.setPixel = convertTextureImage;
		buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
		if (buf.ptr) {
			res.decodeImageData(image->dataPtr, image->num, &buf);
			glBindTexture(GL_TEXTURE_2D, _texturesList[pos].texId);
			const uint16_t *texData = rescaleTexture(&buf, texW, texH);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texData);
			free(buf.ptr);
		}
		_texturesList[pos].hash.ptr = image->dataPtr;
		_texturesList[pos].hash.num = image->num;
		queueGfxImageDraw(pos, image);
	}

	void queueGfxImageDraw(int pos, const GfxImage *image) {
		++_texturesList[pos].refCount;
		gettimeofday(&_texturesList[pos].t0, 0);
		assert(_gfxImagesCount < ARRAYSIZE(_gfxImagesQueue));
		_gfxImagesQueue[_gfxImagesCount].x = image->x;
		_gfxImagesQueue[_gfxImagesCount].y = kH - image->y;
		_gfxImagesQueue[_gfxImagesCount].x2 = image->x + image->w;
		_gfxImagesQueue[_gfxImagesCount].y2 = kH - (image->y + image->h);
		_gfxImagesQueue[_gfxImagesCount].xflip = image->xflip;
		_gfxImagesQueue[_gfxImagesCount].erase = image->erase;
		_gfxImagesQueue[_gfxImagesCount].tex = &_texturesList[pos];
		++_gfxImagesCount;
	}

	void draw(bool menu, int w, int h) {
		if (menu) {
			glColor4f(.7, .7, .7, 1.);
			drawBackground(_title.texId, _title.u, _title.v);
			glColor4f(1., 1., 1., 1.);
			glAlphaFunc(GL_EQUAL, 1);
		} else {
			glAlphaFunc(GL_ALWAYS, 0);
			drawBackground(_background.texId, _background.u, _background.v);
			int i = 0;
			glAlphaFunc(GL_EQUAL, 1);
			for (; i < _gfxImagesCount && !_gfxImagesQueue[i].erase; ++i) {
				drawGfxImage(i);
			}
			drawBackground(_background.texId, _background.u, _background.v);
			for (; i < _gfxImagesCount; ++i) {
				drawGfxImage(i);
			}
			_gfxImagesCount = 0;
		}
	}

	void emitQuadTex(int x1, int y1, int x2, int y2, GLfloat u1, GLfloat v1, GLfloat u2, GLfloat v2) {
#ifdef USE_GLES
		const GLfloat textureVertices[] = {
			x1, y2, 0,
			x1, y1, 0,
			x2, y1, 0,
			x2, y2, 0
		};
		const GLfloat textureUv[] = {
			u1, v1,
			u1, v2,
			u2, v2,
			u2, v1
		};
		glVertexPointer(3, GL_FLOAT, 0, textureVertices);
		glTexCoordPointer(2, GL_FLOAT, 0, textureUv);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
		glBegin(GL_QUADS);
			glTexCoord2f(u1, v1);
			glVertex2i(x1, y2);
			glTexCoord2f(u2, v1);
			glVertex2i(x2, y2);
			glTexCoord2f(u2, v2);
			glVertex2i(x2, y1);
			glTexCoord2f(u1, v2);
			glVertex2i(x1, y1);
		glEnd();
#endif
	}

	void drawBackground(GLuint texId, GLfloat u, GLfloat v) {
		if (texId != -1) {
			glBindTexture(GL_TEXTURE_2D, texId);
			emitQuadTex(0, 0, kW, kH, 0, 0, u, v);
		}
	}

	void drawGfxImage(int i) {
		Texture *tex = _gfxImagesQueue[i].tex;
		if (tex) {
			glBindTexture(GL_TEXTURE_2D, tex->texId);
			if (_gfxImagesQueue[i].xflip) {
				emitQuadTex(_gfxImagesQueue[i].x2, _gfxImagesQueue[i].y2, _gfxImagesQueue[i].x, _gfxImagesQueue[i].y, 0, 0, tex->u, tex->v);
			} else {
				emitQuadTex(_gfxImagesQueue[i].x, _gfxImagesQueue[i].y2, _gfxImagesQueue[i].x2, _gfxImagesQueue[i].y, 0, 0, tex->u, tex->v);
			}
			--tex->refCount;
		}
	}

	void drawText(int x, int y, int color, const uint8_t *dataPtr, int len) {
		const uint16_t color5551 = _tex8to5551[color];
		const GLfloat r = ((color5551 >> 11) & 31) / 31.;
		const GLfloat g = ((color5551 >>  6) & 31) / 31.;
		const GLfloat b = ((color5551 >>  1) & 31) / 31.;
		glColor4f(r, g, b, 1.);
		glBindTexture(GL_TEXTURE_2D, _font.texId);
		y = kH - y;
		for (int i = 0; i < len; ++i) {
			const uint8_t code = dataPtr[i];
			const GLfloat texU1 = _font.u * (code - 32) / (GLfloat)_font.charsCount;
			const GLfloat texU2 = _font.u * (code - 31) / (GLfloat)_font.charsCount;
			emitQuadTex(x, y - 16, x + 16, y, texU1, 0., texU2, _font.v);
			x += 16;
		}
		glColor4f(1., 1., 1., 1.);
	}
};

struct PadInput {

	enum {
		kIdle,
		kDirectionPressed,
		kButtonPressed
	};

	int _state;
	int _refX0, _refY0, _dirX, _dirY;

	PadInput()
		: _state(kIdle) {
	}

	static int squareDist(int dy, int dx) {
		return dy * dy + dx * dx;
	}

	bool feed(int x, int y, bool pressed, PlayerInput &pi) {
		switch (_state) {
		case kIdle:
			if (pressed) {
				if (x > 256 && x < 512 && y > 96 && y < 448) {
					_state = kButtonPressed;
					_refX0 = x;
					_refY0 = y;
					pi.shift = true;
					return true;
				}
				if (x > 0 && x < 256 && y > 0 && y < 448) {
					_state = kDirectionPressed;
					_refX0 = _dirX = x;
					_refY0 = _dirY = y;
					return true;
				}
			}
			return false;
		case kDirectionPressed:
			pi.dirMask = 0;
			if (!pressed) {
				_state = kIdle;
			} else {
				_dirX = x;
				_dirY = y;
				const int dist = squareDist(_dirY - _refY0, _dirX - _refX0);
				if (dist >= 1024) {
					int deg = (int)(atan2(_dirY - _refY0, _dirX - _refX0) * 180 / M_PI);
					if (deg > 0 && deg <= 180) {
						deg = 360 - deg;
					} else {
						deg = -deg;
					}
					if (deg >= 45 && deg < 135) {
						pi.dirMask = PlayerInput::kDirectionUp;
					} else if (deg >= 135 && deg < 225) {
						pi.dirMask = PlayerInput::kDirectionLeft;
					} else if (deg >= 225 && deg < 315) {
						pi.dirMask = PlayerInput::kDirectionDown;
					} else {
						pi.dirMask = PlayerInput::kDirectionRight;
					}
				}
			}
			return true;
		case kButtonPressed:
			if (!pressed) {
				_state = kIdle;
			} else {
				_refX0 = x;
				_refY0 = y;
			}
			pi.shift = pressed;
			return true;
		}
		return false;
	}
};

struct Main {

	enum {
		kStateMenu,
		kStateGame
	};

	ResourceData _resData;
	Game _game;
	TextureCache _texCache;
	PadInput _padInput[2];
	struct timeval _t0;
	int _frameCounter;
	int _framesPerSec;
	int _state, _nextState;
	bool _gameInit;
	bool _menuInit;
	int _w, _h;
	int _mixRate;

	Main(const char *filePath, const char *savePath)
		: _resData(filePath), _game(_resData, savePath) {
		_state = _nextState = kStateMenu;
		_gameInit = _menuInit = false;
		_w = kW;
		_h = kH;
	}

	void init() {
		gettimeofday(&_t0, 0);
		_frameCounter = 0;
		_framesPerSec = 0;
		_resData.loadClutData();
		_resData.loadIconData();
		_resData.loadFontData();
		_resData.loadPersoData();
		_texCache.createTextureFont(_resData);
	}

	void save() {
		if (_state == kStateGame) {
			if (!_game._inventoryOn && !_game._gameOver) {
				_game.saveState();
			}
		}
	}

	void doTick() {
		if (_state != _nextState) {
			_gameInit = _menuInit = false;
			_state = _nextState;
		}
		switch (_state) {
		case kStateMenu:
			if (!_menuInit) {
				doMenuInit();
				_menuInit = true;
			}
			doMenuTick();
			break;
		case kStateGame:
			if (!_gameInit) {
				_game.initGame();
				_game.loadState();
				_game.resetGameState();
				_gameInit = true;
			}
			doGameTick();
			break;
		}
	}

	void doMenuInit() {
		static const int kNum = 3;
		_resData.setupTitleClut(kNum, _game._palette);
		_texCache.updatePalette(_game._palette);
		_texCache.createTextureTitle(_resData, kNum);
	}

	void doMenuTick() {
		_game.doHotspots();
		_game.clearHotspots();
		_game.clearGfxList();
		_game.doTitle();
		if (_game._pi.enter) {
			_game._pi.enter = false;
			_nextState = kStateGame;
		}
	}

	void doGameTick() {
		if (_game._textToDisplay != 0xFFFF) {
			_game.doStoryTexts();
		} else {
			_game.doHotspots();
			_game.clearHotspots();
			if (_game._pi.backspace) {
				_game._pi.backspace = false;
				_game.initInventory();
			}
			_game.clearGfxList();
			if (_game._inventoryOn) {
				_game.doInventory();
			} else {
				_game.doTick();
				if (_game._gameOver) {
					_nextState = kStateMenu;
				}
			}
			_game.drawHotspots();
		}
		if (_game._paletteChanged) {
			_texCache.updatePalette(_game._palette);
		}
		if (_game._backgroundChanged) {
			_texCache.createTextureBackground(_resData, _game._currentLevel, _game._currentRoom);
		}
		for (int i = 0; i < _game._gfxImagesCount; ++i) {
			_texCache.createTextureGfxImage(_resData, &_game._gfxImagesList[i]);
		}
		if (_game._pi.quit) {
			_game._pi.quit = false;
			_nextState = kStateMenu;
		}
	}

	void drawFrame(int w, int h) {
#ifdef USE_GLES
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
		glEnable(GL_ALPHA_TEST);
		glEnable(GL_TEXTURE_2D);
		glViewport(0, 0, w, h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
#ifdef USE_GLES
		glOrthof(0, w, 0, h, 0, 1);
#else
		glOrtho(0, w, 0, h, 0, 1);
#endif
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glPushMatrix();
		glScalef(w / (GLfloat)kW, h / (GLfloat)kH, 1.);
		_texCache.draw((_state == kStateMenu), w, h);
		for (int i = 0; i < _game._gfxTextsCount; ++i) {
			const GfxText *gt = &_game._gfxTextsList[i];
			_texCache.drawText(gt->x, gt->y, gt->color, gt->data, gt->len);
		}
		++_frameCounter;
		if ((_frameCounter & 31) == 0) {
			struct timeval t1;
			gettimeofday(&t1, 0);
			const int msecs = (t1.tv_sec - _t0.tv_sec) * 1000 + (t1.tv_usec - _t0.tv_usec) / 1000;
			_t0 = t1;
			if (msecs != 0) {
//				printf("fps %f\n", 1000. * 32 / msecs);
				_framesPerSec = (int)(1000. * 32 / msecs);
			}
		}
		if (_framesPerSec != 0) {
			char buf[16];
			const int len = snprintf(buf, sizeof(buf), "%d fps", _framesPerSec);
			_texCache.drawText(kW - len * 16, kH - 16, 0xED, (const uint8_t *)buf, len);
		}
		glPopMatrix();
	}

	void doSoundMix(int8 *buf, int size) {
		static const int kFrac = 16;
		for (; size != 0; --size) {
			for (int i = 0; i < 16; ++i) {
				Sfx *sfx = &_game._sfxList[i];
				if (!sfx->dataPtr) {
					continue;
				}
				const int pos = sfx->playOffset >> kFrac;
				if (pos >= sfx->dataSize) {
					free(sfx->dataPtr);
					memset(sfx, 0, sizeof(Sfx));
					continue;
				}
				const int pcm = *buf + (((int8)(sfx->dataPtr[pos] ^ 0x80)) >> sfx->volume);
				if (pcm > 127) {
					*buf = 127;
				} else if (pcm < -128) {
					*buf = -128;
				} else {
					*buf = pcm;
				}
				const int inc = (sfx->freq << kFrac) / _mixRate;
				sfx->playOffset += inc;
			}
			++buf;
		}
	}
};

static void updateKeyInput(int keyCode, bool pressed, PlayerInput &pi) {
	switch (keyCode) {
#ifdef USE_GLES
	case 82: // back
		pi.quit = true;
		break;
	case 84: // search
		pi.backspace = true;
		break;
#else
	case SDLK_LEFT:
		if (!pressed) {
			pi.dirMask &= ~PlayerInput::kDirectionLeft;
		} else {
			pi.dirMask |= PlayerInput::kDirectionLeft;
		}
		break;
	case SDLK_RIGHT:
		if (!pressed) {
			pi.dirMask &= ~PlayerInput::kDirectionRight;
		} else {
			pi.dirMask |= PlayerInput::kDirectionRight;
		}
		break;
	case SDLK_UP:
		if (!pressed) {
			pi.dirMask &= ~PlayerInput::kDirectionUp;
		} else {
			pi.dirMask |= PlayerInput::kDirectionUp;
		}
		break;
	case SDLK_DOWN:
		if (!pressed) {
			pi.dirMask &= ~PlayerInput::kDirectionDown;
		} else {
			pi.dirMask |= PlayerInput::kDirectionDown;
		}
		break;
	case SDLK_SPACE:
		pi.space = pressed;
		break;
	case SDLK_RSHIFT:
	case SDLK_LSHIFT:
		pi.shift = pressed;
		break;
	case SDLK_RETURN:
		pi.enter = pressed;
		break;
	case SDLK_BACKSPACE:
		pi.backspace = pressed;
		break;
#endif
	}
}

static void updateTouchInput(bool pressed, int x, int y, PlayerInput &pi) {
	pi.touch.press = pressed ? PlayerInput::kTouchDown : PlayerInput::kTouchUp;
	pi.touch.x = x;
	pi.touch.y = y;
}

static void transformXY(int &x, int &y, int w, int h) {
	if (w != 0) {
		x = x * kW / w;
	}
	if (h != 0) {
		y = y * kH / h;
	}
}

static Main *gMain;

void stubInit(const char *filePath, const char *savePath, int level) {
	gMain = new Main(filePath, savePath);
	if (level != -1) {
		gMain->_game._currentLevel = level;
	}
	gMain->init();
}

void stubQuit() {
	delete gMain;
	gMain = 0;
}

void stubSave() {
	gMain->save();
}

void stubQueueKeyInput(int keycode, int pressed) {
	updateKeyInput(keycode, pressed != 0, gMain->_game._pi);
}

void stubQueueTouchInput(int num, int x, int y, int pressed) {
	transformXY(x, y, gMain->_w, gMain->_h);
	if (gMain->_game._inventoryOn || gMain->_state == 0 || !gMain->_padInput[num].feed(x, y, pressed != 0, gMain->_game._pi)) {
		updateTouchInput(pressed, x, y, gMain->_game._pi);
	}
}

void stubDoTick() {
	gMain->doTick();
}

void stubInitGL(int w, int h) {
	gMain->_w = w;
	gMain->_h = h;
}

void stubDrawGL(int w, int h) {
	gMain->drawFrame(w, h);
}

static void mixProc(void *data, uint8 *buf, int size) {
	memset(buf, 0, size);
	gMain->doSoundMix((int8 *)buf, size);
}

StubMixProc stubGetMixProc(int rate, int fmt) {
	gMain->_mixRate = rate;
	StubMixProc mix;
	mix.proc = &mixProc;
	mix.data = gMain;
	return mix;
}

extern "C" {
	DYNLIB_SYMBOL struct Stub g_stub = {
		stubInit,
		stubQuit,
		stubSave,
		stubQueueKeyInput,
		stubQueueTouchInput,
		stubDoTick,
		stubInitGL,
		stubDrawGL,
		stubGetMixProc
	};
}

