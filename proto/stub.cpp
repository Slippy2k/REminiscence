
#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>
#include <sys/time.h>
#include "game.h"
#include "resource_data.h"
#include "resource_mac.h"
#include "stub.h"

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
		uint16_t color;
	} _font;

	struct {
		unsigned char buf[64];
		int x, y;
		int len;
		uint8_t color;
	} _gfxTextsQueue[16];
	int _gfxTextsCount;

	struct {
		Texture *tex;
		int x, y, x2, y2;
		bool xflip;
		bool erase;
	} _gfxImagesQueue[192];
	int _gfxImagesCount;

	uint16_t _tex8to565[256];
	uint16_t _tex8to5551[256];

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
		memset(&_gfxTextsQueue, 0, sizeof(_gfxTextsQueue));
		_gfxTextsCount = 0;
		memset(&_gfxImagesQueue, 0, sizeof(_gfxImagesQueue));
		_gfxImagesCount = 0;
	}

	void updatePalette(Color *clut) {
		for (int i = 0; i < 256; ++i) {
			_tex8to565[i] = ((clut[i].r >> 3) << 11) | ((clut[i].g >> 2) << 5) | (clut[i].b >> 3);
			_tex8to5551[i] = ((clut[i].r >> 3) << 11) | ((clut[i].g >> 3) << 6) | ((clut[i].b >> 3) << 1);
		}
	}

	static void convertTextureFont(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
		const int offset = (y * buf->pitch + buf->x + x) * sizeof(uint16_t);
		*(uint16_t *)(buf->ptr + offset) = (color == 0xC1) ? 0xFFFF : 1;
	}

	void createTextureFont(ResourceData &res) {
		_font.charsCount = 105;
		if (_font.texId == -1) {
			glGenTextures(1, &_font.texId);
			glBindTexture(GL_TEXTURE_2D, _font.texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _font.charsCount * 16, 16, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
		}
		DecodeBuffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.w = buf.pitch = _font.charsCount * 16;
		buf.h = 16;
		buf.setPixel = convertTextureFont;
		buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
		if (buf.ptr) {
			for (int i = 0; i < _font.charsCount; ++i) {
				res.decodeImageData(res._fnt, i, &buf);
				buf.x += 16;
			}
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buf.w, buf.h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, buf.ptr);
			free(buf.ptr);
		}
	}

	static void convertTextureTitle(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
		const uint16_t *lut = (const uint16_t *)buf->lut;
		const int offset = (y * buf->pitch + x) * sizeof(uint16_t);
		*(uint16_t *)(buf->ptr + offset) = lut[color] | 1;
	}

	void createTextureTitle(ResourceData &res, int num) {
		const int w = 512;
		const int h = 448;
		if (_title.texId == -1) {
			glGenTextures(1, &_title.texId);
			glBindTexture(GL_TEXTURE_2D, _title.texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
		}
		_title.u = w / (GLfloat)512;
		_title.v = h / (GLfloat)512;
		DecodeBuffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.w = buf.pitch = 512;
		buf.h = 512;
		buf.lut = _tex8to5551;
		buf.setPixel = convertTextureTitle;
		buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
		if (buf.ptr) {
			res.loadTitleImage(num, &buf);
			glBindTexture(GL_TEXTURE_2D, _title.texId);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, buf.ptr);
			free(buf.ptr);
		}
	}

	static void convertTextureBackground(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
		const uint16_t *lut = (const uint16_t *)buf->lut;
		const int offset = (y * buf->pitch + x) * sizeof(uint16_t);
		*(uint16_t *)(buf->ptr + offset) = lut[color] | (color >> 7);
	}

	void createTextureBackground(ResourceData &res, int level, int room) {
		const int w = 512;
		const int h = 448;
		if (_background.texId == -1) {
			glGenTextures(1, &_background.texId);
			glBindTexture(GL_TEXTURE_2D, _background.texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
		}
		_background.u = w / (GLfloat)512;
		_background.v = h / (GLfloat)512;
		if (level != _background.level || room != _background.room) {
			DecodeBuffer buf;
			memset(&buf, 0, sizeof(buf));
			buf.w = buf.pitch = 512;
			buf.h = 512;
			buf.lut = _tex8to5551;
			buf.setPixel = convertTextureBackground;
			buf.ptr = (uint8_t *)calloc(buf.w * buf.h, sizeof(uint16_t));
			if (buf.ptr) {
				res.loadLevelRoom(level, room, &buf);
				glBindTexture(GL_TEXTURE_2D, _background.texId);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, buf.ptr);
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
		const int texW = roundPowerOfTwo(image->w);
		const int texH = roundPowerOfTwo(image->h);
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
		_texturesList[pos].u = image->w / (GLfloat)texW;
		_texturesList[pos].v = image->h / (GLfloat)texH;
		DecodeBuffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.w = buf.pitch = texW;
		buf.h = texH;
		buf.lut = _tex8to5551;
		buf.setPixel = convertTextureImage;
		buf.ptr = (uint8_t *)calloc(texW * texH, sizeof(uint16_t));
		if (buf.ptr) {
			res.decodeImageData(image->dataPtr, image->num, &buf);
			glBindTexture(GL_TEXTURE_2D, _texturesList[pos].texId);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, buf.ptr);
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
		_gfxImagesQueue[_gfxImagesCount].y = 448 - image->y;
		_gfxImagesQueue[_gfxImagesCount].x2 = image->x + image->w;
		_gfxImagesQueue[_gfxImagesCount].y2 = 448 - (image->y + image->h);
		_gfxImagesQueue[_gfxImagesCount].xflip = image->xflip;
		_gfxImagesQueue[_gfxImagesCount].erase = image->erase;
		_gfxImagesQueue[_gfxImagesCount].tex = &_texturesList[pos];
		++_gfxImagesCount;
	}

	void queueGfxTextDraw(const GfxText *gt) {
		assert(_gfxTextsCount < ARRAYSIZE(_gfxTextsQueue));
		const int len = gt->len > 64 ? 64 : gt->len;
		_gfxTextsQueue[_gfxTextsCount].x = gt->x;
		_gfxTextsQueue[_gfxTextsCount].y = gt->y;
		_gfxTextsQueue[_gfxTextsCount].len = len;
		_gfxTextsQueue[_gfxTextsCount].color = gt->color;
		memcpy(_gfxTextsQueue[_gfxTextsCount].buf, gt->dataPtr, len);
		++_gfxTextsCount;
	}

	void draw(bool menu, int w, int h) {
		if (menu) {
			glColor3f(.7, .7, .7);
			drawBackground(_title.texId, _title.u, _title.v);
			glColor3f(1., 1., 1.);
			glAlphaFunc(GL_EQUAL, 1);
		} else {
			glAlphaFunc(GL_ALWAYS, 0);
			drawBackground(_background.texId, _background.u, _background.v);
			int i = 0;
			glAlphaFunc(GL_EQUAL, 1);
			for (; i < _gfxImagesCount && !_gfxImagesQueue[i].erase; ++i) {
				drawGfxImage(i);
			}
			glAlphaFunc(GL_EQUAL, 1);
			drawBackground(_background.texId, _background.u, _background.v);
			glAlphaFunc(GL_EQUAL, 1);
			for (; i < _gfxImagesCount; ++i) {
				drawGfxImage(i);
			}
			_gfxImagesCount = 0;
		}
		for (int i = 0; i < _gfxTextsCount; ++i) {
			drawText(i);
		}
		_gfxTextsCount = 0;
	}

	void drawBackground(GLuint texId, GLfloat u, GLfloat v) {
		if (texId != -1) {
			glBindTexture(GL_TEXTURE_2D, texId);
			glBegin(GL_QUADS);
				glTexCoord2f(0., 0.);
				glVertex2i(0, 448);
				glTexCoord2f(u, 0.);
				glVertex2i(512, 448);
				glTexCoord2f(u, v);
				glVertex2i(512, 0);
				glTexCoord2f(0., v);
				glVertex2i(0, 0);
			glEnd();
		}
	}

	void drawGfxImage(int i) {
		Texture *tex = _gfxImagesQueue[i].tex;
		if (tex) {
			glBindTexture(GL_TEXTURE_2D, tex->texId);
			glBegin(GL_QUADS);
				if (_gfxImagesQueue[i].xflip) {
					glTexCoord2f(0., 0.);
					glVertex2i(_gfxImagesQueue[i].x2, _gfxImagesQueue[i].y);
					glTexCoord2f(tex->u, 0.);
					glVertex2i(_gfxImagesQueue[i].x, _gfxImagesQueue[i].y);
					glTexCoord2f(tex->u, tex->v);
					glVertex2i(_gfxImagesQueue[i].x, _gfxImagesQueue[i].y2);
					glTexCoord2f(0., tex->v);
					glVertex2i(_gfxImagesQueue[i].x2, _gfxImagesQueue[i].y2);
				} else {
					glTexCoord2f(0., 0.);
					glVertex2i(_gfxImagesQueue[i].x, _gfxImagesQueue[i].y);
					glTexCoord2f(tex->u, 0.);
					glVertex2i(_gfxImagesQueue[i].x2, _gfxImagesQueue[i].y);
					glTexCoord2f(tex->u, tex->v);
					glVertex2i(_gfxImagesQueue[i].x2, _gfxImagesQueue[i].y2);
					glTexCoord2f(0., tex->v);
					glVertex2i(_gfxImagesQueue[i].x, _gfxImagesQueue[i].y2);
				}
			glEnd();
			--tex->refCount;
		}
	}

	void drawText(int num) {
		const uint16_t color = _tex8to5551[_gfxTextsQueue[num].color];
		const GLfloat r = ((color >> 11) & 31) / 31.;
		const GLfloat g = ((color >>  6) & 31) / 31.;
		const GLfloat b = ((color >>  1) & 31) / 31.;
		glBindTexture(GL_TEXTURE_2D, _font.texId);
		glColor3f(r, g, b);
		int x = _gfxTextsQueue[num].x;
		const int y = 448 - _gfxTextsQueue[num].y;
		for (int i = 0; i < _gfxTextsQueue[num].len; ++i) {
			const uint8_t code = _gfxTextsQueue[num].buf[i];
			const GLfloat texU1 = (code - 32) / (GLfloat)_font.charsCount;
			const GLfloat texU2 = (code - 31) / (GLfloat)_font.charsCount;
			glBegin(GL_QUADS);
				glTexCoord2f(texU1, 0.);
				glVertex2i(x, y);
				glTexCoord2f(texU2, 0.);
				glVertex2i(x + 16, y);
				glTexCoord2f(texU2, 1.);
				glVertex2i(x + 16, y - 16);
				glTexCoord2f(texU1, 1.);
				glVertex2i(x, y - 16);
			glEnd();
			x += 16;
		}
		glColor3f(1., 1., 1.);
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

	bool feed(int x, int y, int released, PlayerInput &pi) {
		switch (_state) {
		case kIdle:
			if (!released) {
				if (x > 256 && x < 512 && y > 96 && y < 448) {
					_state = kButtonPressed;
					_refX0 = x;
					_refY0 = y;
					pi.shift = true;
					return true;
				}
				if (x > 0 && x < 256 && y > 96 && y < 448) {
					_state = kDirectionPressed;
					_refX0 = _dirX = x;
					_refY0 = _dirY = y;
					return true;
				}
			}
			return false;
		case kDirectionPressed:
			pi.dirMask = 0;
			if (released) {
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
			pi.shift = (released == 0);
			_refX0 = x;
			_refY0 = y;
			if (released) {
				_state = kIdle;
			}
			return true;
		}
		return false;
	}

	void draw() {
		switch (_state) {
		case kDirectionPressed:
			glDisable(GL_TEXTURE_2D);
			glColor3f(1., 1., 1.);
			glBegin(GL_LINES);
				glVertex2i(_refX0, 448 - _refY0);
				glVertex2i(_dirX, 448 - _dirY);
			glEnd();
			glColor3f(1., 1., 1.);
			break;
		case kButtonPressed:
			glDisable(GL_TEXTURE_2D);
			glColor3f(1., 1., 1.);
			glBegin(GL_QUADS);
				glVertex2i(_refX0 - 16, 448 - (_refY0 - 16));
				glVertex2i(_refX0 + 16, 448 - (_refY0 - 16));
				glVertex2i(_refX0 + 16, 448 - (_refY0 + 16));
				glVertex2i(_refX0 - 16, 448 - (_refY0 + 16));
			glEnd();
			glColor3f(1., 1., 1.);
			break;
		}
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
	int _state, _nextState;
	bool _gameInit;
	bool _menuInit;
	int _w, _h;

	Main(const char *filePath)
		: _resData(filePath), _game(_resData) {
		_state = _nextState = kStateMenu;
		_gameInit = _menuInit = false;
		_w = 512;
		_h = 448;
	}

	void init() {
		gettimeofday(&_t0, 0);
		_frameCounter = 0;
		_resData.loadClutData();
		_resData.loadIconData();
		_resData.loadFontData();
		_resData.loadPersoData();
		_texCache.createTextureFont(_resData);
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
		for (int i = 0; i < _game._gfxTextsCount; ++i) {
			_texCache.queueGfxTextDraw(&_game._gfxTextsList[i]);
		}
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
		for (int i = 0; i < _game._gfxTextsCount; ++i) {
			_texCache.queueGfxTextDraw(&_game._gfxTextsList[i]);
		}
	}

	void drawFrame(int w, int h) {
		glEnable(GL_ALPHA_TEST);
		glEnable(GL_TEXTURE_2D);
		glViewport(0, 0, w, h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glPushMatrix();
		glScalef(w / 512., h / 448., 1.);
		_texCache.draw((_state == kStateMenu), w, h);
		glPopMatrix();
		++_frameCounter;
		if ((_frameCounter & 31) == 0) {
			struct timeval t1;
			gettimeofday(&t1, 0);
			const int msecs = (t1.tv_sec - _t0.tv_sec) * 1000 + (t1.tv_usec - _t0.tv_usec) / 1000;
			_t0 = t1;
			if (msecs != 0) {
				printf("fps %f\n", 1000. * 32 / msecs);
			}
		}
	}
};

static void updateKeyInput(int keyCode, bool pressed, PlayerInput &pi) {
	switch (keyCode) {
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
	}
}

static void updateTouchInput(bool released, int x, int y, PlayerInput &pi) {
	pi.touch.press = released ? PlayerInput::kTouchUp : PlayerInput::kTouchDown;
	pi.touch.x = x;
	pi.touch.y = y;
}

static void transformXY(int &x, int &y, int w, int h) {
	if (w != 0) {
		x = x * 512 / w;
	}
	if (h != 0) {
		y = y * 448 / h;
	}
}

static Main *gMain;

void stubInit(const char *filePath, int level) {
	gMain = new Main(filePath);
	if (level != -1) {
		gMain->_game._currentLevel = level;
	}
	gMain->init();
}

void stubQuit() {
	delete gMain;
	gMain = 0;
}

void stubQueueKeyInput(int keycode, int pressed) {
	updateKeyInput(keycode, pressed != 0, gMain->_game._pi);
}

void stubQueueTouchInput(int num, int x, int y, int released) {
	transformXY(x, y, gMain->_w, gMain->_h);
	if (gMain->_game._inventoryOn || gMain->_state == 0 || !gMain->_padInput[num].feed(x, y, released, gMain->_game._pi)) {
		updateTouchInput(released != 0, x, y, gMain->_game._pi);
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
	for (int i = 0; i < 2; ++i) {
		gMain->_padInput[i].draw();
	}
}

extern "C" {
	struct Stub g_stub = {
		stubInit,
		stubQuit,
		stubQueueKeyInput,
		stubQueueTouchInput,
		stubDoTick,
		stubInitGL,
		stubDrawGL
	};
}

