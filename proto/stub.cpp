
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
#include "input.h"
#include "render.h"
#include "stub.h"

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

	void doSoundMix(int8_t *buf, int size) {
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
				const int pcm = *buf + (((int8_t)(sfx->dataPtr[pos] ^ 0x80)) >> sfx->volume);
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
	case kKeyCodeLeft:
		if (!pressed) {
			pi.dirMask &= ~PlayerInput::kDirectionLeft;
		} else {
			pi.dirMask |= PlayerInput::kDirectionLeft;
		}
		break;
	case kKeyCodeRight:
		if (!pressed) {
			pi.dirMask &= ~PlayerInput::kDirectionRight;
		} else {
			pi.dirMask |= PlayerInput::kDirectionRight;
		}
		break;
	case kKeyCodeUp:
		if (!pressed) {
			pi.dirMask &= ~PlayerInput::kDirectionUp;
		} else {
			pi.dirMask |= PlayerInput::kDirectionUp;
		}
		break;
	case kKeyCodeDown:
		if (!pressed) {
			pi.dirMask &= ~PlayerInput::kDirectionDown;
		} else {
			pi.dirMask |= PlayerInput::kDirectionDown;
		}
		break;
	case kKeyCodeSpace:
		pi.space = pressed;
		break;
	case kKeyCodeShift:
		pi.shift = pressed;
		break;
	case kKeyCodeReturn:
		pi.enter = pressed;
		break;
	case kKeyCodeBackspace:
		pi.backspace = pressed;
		break;
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

struct GameStub_Flashback: GameStub {

	Main *_m;

	virtual void init(const char *filePath, const char *savePath, int level) {
		_m = new Main(filePath, savePath);
		if (level != -1) {
			_m->_game._currentLevel = level;
		}
		_m->init();
	}

	virtual void quit() {
		delete _m;
		_m = 0;
	}

	virtual void save() {
		_m->save();
	}

	virtual void queueKeyInput(int keycode, int pressed) {
		updateKeyInput(keycode, pressed != 0, _m->_game._pi);
	}

	virtual void queueTouchInput(int num, int x, int y, int pressed) {
		transformXY(x, y, _m->_w, _m->_h);
		if (_m->_game._inventoryOn || _m->_state == 0 || !_m->_padInput[num].feed(x, y, pressed != 0, _m->_game._pi)) {
			updateTouchInput(pressed, x, y, _m->_game._pi);
		}
	}

	virtual void doTick() {
		_m->doTick();
	}

	virtual void initGL(int w, int h) {
		_m->_w = w;
		_m->_h = h;
	}

	void drawGL(int w, int h) {
		_m->drawFrame(w, h);
	}

	static void mixProc(void *data, uint8_t *buf, int size) {
		memset(buf, 0, size);
		((Main *)data)->doSoundMix((int8_t *)buf, size);
	}

	virtual StubMixProc getMixProc(int rate, int fmt) {
		_m->_mixRate = rate;
		StubMixProc mix;
		mix.proc = &mixProc;
		mix.data = _m;
		return mix;
	}
};

extern "C" {
	GameStub *GameStub_create() {
		return new GameStub_Flashback;
	}
}
