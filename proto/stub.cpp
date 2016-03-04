
#ifdef _WIN32
#include <windows.h>
#define DYNLIB_SYMBOL DECLSPEC_EXPORT
#else
#define DYNLIB_SYMBOL
#endif
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

	bool init() {
		if (!_resData._res.isOpen()) {
			return false;
		}
		_texCache.init();
		_resData.loadClutData();
		_resData.loadIconData();
		_resData.loadFontData();
		_resData.loadPersoData();
		_texCache.createTextureFont(_resData);
		_game._paletteChanged = false;
		_game._backgroundChanged = false;
		return true;
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
		if (_game._pi.quit) {
			_game._pi.quit = false;
			_nextState = kStateMenu;
		}
	}

	void drawFrame(int w, int h) {
		_texCache.prepareFrameDraw(w, h);
		if (_game._paletteChanged) {
			_texCache.updatePalette(_game._palette);
		}
		if (_game._backgroundChanged) {
			_texCache.createTextureBackground(_resData, _game._currentLevel, _game._currentRoom);
		}
		for (int i = 0; i < _game._gfxImagesCount; ++i) {
			_texCache.createTextureGfxImage(_resData, &_game._gfxImagesList[i]);
		}
		_texCache.draw((_state == kStateMenu), w, h);
		for (int i = 0; i < _game._gfxTextsCount; ++i) {
			const GfxText *gt = &_game._gfxTextsList[i];
			_texCache.drawText(gt->x, gt->y, gt->color, gt->data, gt->len);
		}
		if (_texCache._framesPerSec != 0) {
			char buf[16];
			const int len = snprintf(buf, sizeof(buf), "%d fps", _texCache._framesPerSec);
			_texCache.drawText(kW - len * 16, kH - 16, 0xED, (const uint8_t *)buf, len);
		}
		_texCache.endFrameDraw();
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

	virtual bool init(const char *filePath, const char *savePath, int level) {
		_m = new Main(filePath, savePath);
		if (level != -1) {
			_m->_game._currentLevel = level;
		}
		return _m->init();
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

	virtual void drawGL(int w, int h) {
		_m->drawFrame(w, h);
	}

	virtual void setMixerImpl(Mixer *mix) {
		_m->_game._mix = mix;
	}
};

extern "C" {
	GameStub *GameStub_create() {
		return new GameStub_Flashback;
	}
}
