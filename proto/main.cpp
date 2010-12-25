
#include <SDL.h>
#include <SDL_opengl.h>
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
	enum {
		kImageTexturesCount = 128
	};

	struct {
		int level;
		int room;
		GLuint _texId;
		GLfloat u, v;
	} _background;

	struct {
		const uint8_t *dataPtr;
		int num;
		GLuint _texId;
		GLfloat u, v;
		int counter;
		int x, y, x2, y2;
		bool xflip;
	} _images[kImageTexturesCount];

	uint16_t _tex8to565[256];
	uint16_t _tex8to5551[256];

	TextureCache() {
		memset(&_background, 0, sizeof(_background));
		_background._texId = -1;
		memset(&_images, 0, sizeof(_images));
		for (int i = 0; i < kImageTexturesCount; ++i) {
			_images[i]._texId = -1;
		}
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
printf("load level %d room %d\n", level, room);
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
		for (int i = 0; i < kImageTexturesCount; ++i) {
			if (_images[i].dataPtr == image->dataPtr && _images[i].num == image->num) {
				_images[i].counter = 2;
				pos = i;
				goto done;
			} else if (_images[i].counter == 0) {
				pos = i;
			}
		}
		if (pos == -1) {
			printf("No free slot for texture cache\n");
			return;
		}
		if (_images[pos]._texId == -1) {
			glGenTextures(1, &_images[pos]._texId);
			glBindTexture(GL_TEXTURE_2D, _images[pos]._texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0);
                }
		_images[pos].u = image->w / (GLfloat)texW;
		_images[pos].v = image->h / (GLfloat)texH;
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
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, buf.ptr);
			free(buf.ptr);
		}
done:
		_images[pos].dataPtr = image->dataPtr;
		_images[pos].num = image->num;
		_images[pos].counter = 2;
		_images[pos].x = image->x;
		_images[pos].y = gWindowH - image->y;
		_images[pos].x2 = image->x + image->w;
		_images[pos].y2 = gWindowH - (image->y + image->h);
		_images[pos].xflip = image->xflip;
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
		for (int i = 0; i < kImageTexturesCount; ++i) {
			if (_images[i].counter != 2) {
				continue;
			}
			glBindTexture(GL_TEXTURE_2D, _images[i]._texId);
			glBegin(GL_QUADS);
				if (_images[i].xflip) {
					glTexCoord2f(0., 0.);
					glVertex2i(_images[i].x2, _images[i].y);
					glTexCoord2f(_images[i].u, 0.);
					glVertex2i(_images[i].x, _images[i].y);
					glTexCoord2f(_images[i].u, _images[i].v);
					glVertex2i(_images[i].x, _images[i].y2);
					glTexCoord2f(0., _images[i].v);
					glVertex2i(_images[i].x2, _images[i].y2);
				} else {
					glTexCoord2f(0., 0.);
					glVertex2i(_images[i].x, _images[i].y);
					glTexCoord2f(_images[i].u, 0.);
					glVertex2i(_images[i].x2, _images[i].y);
					glTexCoord2f(_images[i].u, _images[i].v);
					glVertex2i(_images[i].x2, _images[i].y2);
					glTexCoord2f(0., _images[i].v);
					glVertex2i(_images[i].x, _images[i].y2);
				}
			glEnd();
			if (_images[i].counter >= 1) {
				--_images[i].counter;
				if (_images[i].counter == 0) {
					glDeleteTextures(1, &_images[i]._texId);
					_images[i].dataPtr = 0;
				}
			}
		}
	}
};

struct Test {
	ResourceData _resData;
	TextureCache _texCache;
	GLuint _textureId;
	int _w, _h;
	uint16_t *_texData;
	uint16_t _tex8to16[256];

	Test(const char *filePath)
		: _resData(filePath), _textureId(-1), _texData(0) {
	}
	~Test() {
		free(_texData);
		_texData = 0;
	}

	void init(int w, int h) {
		_resData.loadClutData();
		_resData.loadIconData();
		_resData.loadFontData();
		_resData.loadPersoData();
		_w = w;
		_h = h;
		_texData = (uint16_t *)calloc(w * h, sizeof(uint16_t));
	}

	void doFrame(int w, int h) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		if (_textureId != -1) {
			glBindTexture(GL_TEXTURE_2D, _textureId);
			glBegin(GL_QUADS);
				glTexCoord2f(0., 0.);
				glVertex2i(0, 448);
				glTexCoord2f(1., 0.);
				glVertex2i(512, 448);
				glTexCoord2f(1., 1.);
				glVertex2i(512, 0);
				glTexCoord2f(0., 1.);
				glVertex2i(0, 0);
			glEnd();
		}
	}

	void doFrameTextureCache(int w, int h) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		_texCache.draw();
	}

	void updatePalette(const Color *clut) {
		for (int i = 0; i < 256; ++i) {
			_tex8to16[i] = ((clut[i].r >> 3) << 11) | ((clut[i].g >> 2) << 5) | (clut[i].b >> 3);
		}
	}

	void uploadTexture(const uint8_t *imageData, int w, int h, const uint8_t *dirtyMask) {
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				_texData[y * w + x] = _tex8to16[*imageData++];
			}
		}
		if (_textureId == -1) {
			glGenTextures(1, &_textureId);
			glBindTexture(GL_TEXTURE_2D, _textureId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, _texData);
		} else {
			glBindTexture(GL_TEXTURE_2D, _textureId);
			if (!dirtyMask) {
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, _texData);
			} else {
				glPixelStorei(GL_UNPACK_ROW_LENGTH, w);
				static const int kMaskSize = Game::kDirtyMaskSize;
				for (int y = 0; y < h; y += kMaskSize) {
					const int imageY = y;
					const int imageH = kMaskSize;
					int imageW = 0;
					for (int x = 0; x < w; x += kMaskSize) {
						const uint8_t code = *dirtyMask++;
						if (code != 0) {
							imageW += kMaskSize;
						} else if (imageW != 0) {
							const int imageX = x - imageW;
							const int texOffset = 3 * (imageY * w + imageX);
							glTexSubImage2D(GL_TEXTURE_2D, 0, imageX, imageY, imageW, imageH, GL_RGB, GL_UNSIGNED_BYTE, _texData + texOffset);
							imageW = 0;
						}
					}
					if (imageW != 0) {
						const int imageX = w - imageW;
						const int texOffset = 3 * (imageY * w + imageX);
						glTexSubImage2D(GL_TEXTURE_2D, 0, imageX, imageY, imageW, imageH, GL_RGB, GL_UNSIGNED_BYTE, _texData + texOffset);
					}
				}
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

	Test t(argv[1]);
	t.init(gWindowW, gWindowH);
	Game game(t._resData);
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
			if (game._inventoryOn) {
				game.doInventory();
			} else {
				game.doTick();
			}
			game.drawHotspots();
		}
		if (game._paletteChanged) {
			t.updatePalette(game._palette);
			t._texCache.updatePalette(game._palette);
		}
		if (game._backgroundChanged) {
//			_res.loadLevelRoom(_currentLevel, _currentRoom, &buf);
//			TODO: stencil if color & 0x80
			t._texCache.createTextureBackground(t._resData, game._currentLevel, game._currentRoom);
		}
		for (int i = 0; i < game._imagesCount; ++i) {
			t._texCache.createTextureImage(t._resData, &game._imagesList[i]);
		}
		// const uint8_t *maskLayer = game._invalidatedDirtyMaskLayer ? 0 : game._dirtyMaskLayer;
//		t.uploadTexture(game._frontLayer, Game::kScreenWidth, Game::kScreenHeight, 0);
//		t.doFrame(gWindowW, gWindowH);
		t.doFrameTextureCache(gWindowW, gWindowH);
		game.updateDirtyMaskLayer();
		SDL_GL_SwapBuffers();
		SDL_Delay(gTickDuration);
	}
	return 0;
}

