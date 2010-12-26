
#include <SDL.h>
#include <SDL_opengl.h>
#include <sys/time.h>
#include "game.h"
#include "resource_data.h"
#include "resource_mac.h"
#include "util.h"

static const char *gWindowTitle = "Flashback: The Quest For Identity";
static const int gWindowW = 512;
static const int gWindowH = 448;
static const int gTickDuration = 16;

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
		int level;
		int room;
		GLuint _texId;
		GLfloat u, v;
	} _background;

	struct {
		Texture *tex;
		int x, y, x2, y2;
		bool xflip;
	} _images[192];
	int _imagesCount;

	uint16_t _tex8to565[256];
	uint16_t _tex8to5551[256];

	TextureCache() {
		memset(&_background, 0, sizeof(_background));
		_background._texId = -1;
		memset(&_texturesList, 0, sizeof(_texturesList));
		for (int i = 0; i < ARRAYSIZE(_texturesList); ++i) {
			_texturesList[i].texId = -1;
		}
		memset(&_images, 0, sizeof(_images));
		_imagesCount = 0;
	}

	void updatePalette(Color *clut) {
		for (int i = 0; i < 256; ++i) {
			_tex8to565[i] = ((clut[i].r >> 3) << 11) | ((clut[i].g >> 2) << 5) | (clut[i].b >> 3);
			_tex8to5551[i] = ((clut[i].r >> 3) << 11) | ((clut[i].g >> 3) << 6) | ((clut[i].b >> 3) << 1) | 1;
		}
	}

	static void convertTexture(DecodeBuffer *buf, int x, int y, int w, int h, uint8_t color) {
		const uint16_t *lut = (const uint16_t *)buf->lut;
		const int offset = (y * buf->pitch + x) * sizeof(uint16_t);
		*(uint16_t *)(buf->ptr + offset) = lut[color];
	}

	void createTextureBackground(ResourceData &res, int level, int room) {
		const int w = 512;
		const int h = 448;
		if (_background._texId == -1) {
			glGenTextures(1, &_background._texId);
			glBindTexture(GL_TEXTURE_2D, _background._texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
		}
		_background.u = w / (GLfloat)512;
		_background.v = h / (GLfloat)512;
		if (level != _background.level || room != _background.room) {
			DecodeBuffer buf;
			memset(&buf, 0, sizeof(buf));
			buf.w = 512;
			buf.h = 512;
			buf.pitch = 512;
			buf.lut = _tex8to565;
			buf.setPixel = convertTexture;
			buf.ptr = (uint8_t *)calloc(512 * 512, sizeof(uint16_t));
			if (buf.ptr) {
				res.loadLevelRoom(level, room, &buf);
				glBindTexture(GL_TEXTURE_2D, _background._texId);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, buf.ptr);
				free(buf.ptr);
			}
			_background.level = level;
			_background.room = room;
		}
	}

	void createTextureImage(ResourceData &res, const Image *image) {
		const int texW = roundPowerOfTwo(image->w);
		const int texH = roundPowerOfTwo(image->h);
		int pos = -1;
		for (int i = 0; i < ARRAYSIZE(_texturesList); ++i) {
			if (_texturesList[i].texId == -1) {
				pos = i;
			} else if (_texturesList[i].hash.ptr == image->dataPtr && _texturesList[i].hash.num == image->num) {
				pos = i;
				goto done;
			}
		}
		if (pos == -1) {
			struct timeval t0;
			gettimeofday(&t0, 0);
			int max = 0;
			for (int i = 0; i < ARRAYSIZE(_texturesList); ++i) {
				if (_texturesList[i].refCount == 0) {
					int d = (t0.tv_sec - _texturesList[i].t0.tv_sec) * 1000 + (t0.tv_usec - _texturesList[i].t0.tv_usec) / 1000;
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
		buf.w = texW;
		buf.h = texH;
		buf.pitch = texW;
		buf.lut = _tex8to5551;
		buf.setPixel = convertTexture;
		buf.ptr = (uint8_t *)calloc(texW * texH, sizeof(uint16_t));
		if (buf.ptr) {
			res.decodeImageData(image->dataPtr, image->num, &buf);
			glBindTexture(GL_TEXTURE_2D, _texturesList[pos].texId);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, buf.ptr);
			free(buf.ptr);
		}
		_texturesList[pos].hash.ptr = image->dataPtr;
		_texturesList[pos].hash.num = image->num;
done:
		++_texturesList[pos].refCount;
		gettimeofday(&_texturesList[pos].t0, 0);
		assert(_imagesCount < ARRAYSIZE(_images));
		_images[_imagesCount].x = image->x;
		_images[_imagesCount].y = gWindowH - image->y;
		_images[_imagesCount].x2 = image->x + image->w;
		_images[_imagesCount].y2 = gWindowH - (image->y + image->h);
		_images[_imagesCount].xflip = image->xflip;
		_images[_imagesCount].tex = &_texturesList[pos];
		++_imagesCount;
	}

	void draw() {
		if (_background._texId != -1) {
			glBindTexture(GL_TEXTURE_2D, _background._texId);
			glBegin(GL_QUADS);
				glTexCoord2f(0., 0.);
				glVertex2i(0, 448);
				glTexCoord2f(_background.u, 0.);
				glVertex2i(512, 448);
				glTexCoord2f(_background.u, _background.v);
				glVertex2i(512, 0);
				glTexCoord2f(0., _background.v);
				glVertex2i(0, 0);
			glEnd();
		}
		for (int i = 0; i < _imagesCount; ++i) {
			Texture *tex = _images[i].tex;
			if (tex) {
				glBindTexture(GL_TEXTURE_2D, tex->texId);
				glBegin(GL_QUADS);
					if (_images[i].xflip) {
						glTexCoord2f(0., 0.);
						glVertex2i(_images[i].x2, _images[i].y);
						glTexCoord2f(tex->u, 0.);
						glVertex2i(_images[i].x, _images[i].y);
						glTexCoord2f(tex->u, tex->v);
						glVertex2i(_images[i].x, _images[i].y2);
						glTexCoord2f(0., tex->v);
						glVertex2i(_images[i].x2, _images[i].y2);
					} else {
						glTexCoord2f(0., 0.);
						glVertex2i(_images[i].x, _images[i].y);
						glTexCoord2f(tex->u, 0.);
						glVertex2i(_images[i].x2, _images[i].y);
						glTexCoord2f(tex->u, tex->v);
						glVertex2i(_images[i].x2, _images[i].y2);
						glTexCoord2f(0., tex->v);
						glVertex2i(_images[i].x, _images[i].y2);
					}
				glEnd();
				--tex->refCount;
			}
		}
		_imagesCount = 0;
	}
};

struct Main {
	ResourceData _resData;
	TextureCache _texCache;
	int _w, _h;
	struct timeval _t0;
	int _frameCounter;

	Main(const char *filePath)
		: _resData(filePath) {
	}

	void init(int w, int h) {
		gettimeofday(&_t0, 0);
		_frameCounter = 0;
		_resData.loadClutData();
		_resData.loadIconData();
		_resData.loadFontData();
		_resData.loadPersoData();
		_w = w;
		_h = h;
	}

	void doFrame(int w, int h) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		_texCache.draw();
	}

	void printFps() {
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
	// TODO: transform x,y if texture not blitted at 0,0
	pi.touch.x = x;
	pi.touch.y = y;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("%s datafile level", argv[0]);
		return 0;
	}
	SDL_Init(SDL_INIT_VIDEO);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
	SDL_WM_SetCaption(gWindowTitle, 0);
	glEnable(GL_TEXTURE_2D);

	Main m(argv[1]);
	m.init(gWindowW, gWindowH);
	Game game(m._resData);
	game._currentLevel = 0;
	if (argc >= 3) {
		game._currentLevel = atoi(argv[2]);
	}
	game.initGame();
	glViewport(0, 0, gWindowW, gWindowH);
	bool quitGame = false;
	while (!quitGame) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				quitGame = true;
				break;
			case SDL_KEYDOWN:
				updateKeyInput(ev.key.keysym.sym, true, game._pi);
				break;
			case SDL_KEYUP:
				updateKeyInput(ev.key.keysym.sym, false, game._pi);
				break;
			case SDL_MOUSEBUTTONUP:
				updateTouchInput(true, ev.button.x, ev.button.y, game._pi);
				break;
			case SDL_MOUSEBUTTONDOWN:
				updateTouchInput(false, ev.button.x, ev.button.y, game._pi);
				break;
			case SDL_MOUSEMOTION:
				if (ev.motion.state & SDL_BUTTON(1)) {
					updateTouchInput(false, ev.motion.x, ev.motion.y, game._pi);
				}
				break;
			}
		}
		if (game._textToDisplay != 0xFFFF) {
			game.doStoryTexts();
		} else {
			game.doHotspots();
	                if (game._pi.touch.press == PlayerInput::kTouchUp) {
				game._pi.touch.press = PlayerInput::kTouchNone;
			}
			game.clearHotspots();
			if (game._pi.backspace) {
				game._pi.backspace = false;
				game.initInventory();
			}
			game.clearImagesList();
			if (game._inventoryOn) {
				game.doInventory();
			} else {
				game.doTick();
			}
			game.drawHotspots();
		}
		if (game._paletteChanged) {
			m._texCache.updatePalette(game._palette);
		}
		if (game._backgroundChanged) {
//			TODO: stencil if color & 0x80
			m._texCache.createTextureBackground(m._resData, game._currentLevel, game._currentRoom);
		}
		for (int i = 0; i < game._imagesCount; ++i) {
			m._texCache.createTextureImage(m._resData, &game._imagesList[i]);
		}
		m.doFrame(gWindowW, gWindowH);
		m.printFps();
		SDL_GL_SwapBuffers();
		SDL_Delay(gTickDuration);
	}
	return 0;
}

