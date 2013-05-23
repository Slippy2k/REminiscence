
#define DYNLIB_SYMBOL
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "game.h"
#include "mixer.h"
#include "stub.h"
#include "util.h"
#include "video.h"

char *g_err;

static int detectVersion(const char *dataPath) {
	static const struct {
		const char *filename;
		int type;
		const char *name;
	} table[] = {
		{ "LEVEL1.MAP", kResourceTypePC, "PC" },
		{ "LEVEL1.LEV", kResourceTypeAmiga, "Amiga" },
		{ 0, -1 }
	};
	for (int i = 0; table[i].filename; ++i) {
		File f;
		if (f.open(table[i].filename, "rb", dataPath)) {
			debug(DBG_INFO, "Detected %s version", table[i].name);
			return table[i].type;
		}
	}
	return -1;
}

static Language detectLanguage(const char *dataPath) {
	static const struct {
		const char *filename;
		Language language;
	} table[] = {
		// PC
		{ "ENGCINE.TXT", LANG_EN },
		{ "FR_CINE.TXT", LANG_FR },
		{ "GERCINE.TXT", LANG_DE },
		{ "SPACINE.TXT", LANG_SP },
		{ "ITACINE.TXT", LANG_IT },
		// Amiga
		{ "FRCINE.TXT", LANG_FR },
		{ 0, LANG_EN }
	};
	for (int i = 0; table[i].filename; ++i) {
		File f;
		if (f.open(table[i].filename, "rb", dataPath)) {
			return table[i].language;
		}
	}
	return LANG_EN;
}

enum {
	kStateGame = 0,
	kStateInventory,
	kStateStoryTexts,
	kStateCutscene,
};

struct GameStub_Flashback : GameStub {

	Game *_g;
	int _newState, _state;

	virtual int init(int argc, char *argv[], char *errBuf) {
		const char *dataPath = argc > 0 ? argv[0] : "DATA";
		const int levelNum = argc > 1 ? atoi(argv[1]) : 0;
		g_err = errBuf;
		const int version = detectVersion(dataPath);
		if (version < 0) {
			fprintf(stderr, "Unable to find data files, check that all required files are present");
			return -1;
		}
		Language lang = detectLanguage(dataPath);
		_g = new Game(dataPath, ".", levelNum, (ResourceType)version, lang);
		_g->init();
		_g->loadLevelData();
		_g->resetGameState();
		_state = -1;
		_newState = kStateCutscene;
		return 0;
	}
	virtual void quit() {
		_g->saveState();
		delete _g;
	}
	virtual StubBackBuffer getBackBuffer() {
		StubBackBuffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.w = Video::GAMESCREEN_W;
		buf.h = Video::GAMESCREEN_H;
		buf.ptr = _g->_vid._frontLayer;
		buf.pal = _g->_vid._palBuf;
		return buf;
	}
	virtual StubMixProc getMixProc(int rate, int fmt, void (*lock)(int)) {
		StubMixProc mix;
		memset(&mix, 0, sizeof(mix));
		mix.proc = Mixer::mixCallback;
		mix.data = &_g->_mix;
		_g->_mix.setFormat(rate, fmt);
		_g->_mix._lock = lock;
		return mix;
	}
	virtual void eventKey(int keycode, int pressed) {
		switch (keycode) {
		case kKeyLeft:
			if (pressed) {
				_g->_pi.dirMask |= PlayerInput::DIR_LEFT;
			} else {
				_g->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
			}
			break;
		case kKeyRight:
			if (pressed) {
				_g->_pi.dirMask |= PlayerInput::DIR_RIGHT;
			} else {
				_g->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
			}
			break;
		case kKeyUp:
			if (pressed) {
				_g->_pi.dirMask |= PlayerInput::DIR_UP;
			} else {
				_g->_pi.dirMask &= ~PlayerInput::DIR_UP;
			}
			break;
		case kKeyDown:
			if (pressed) {
				_g->_pi.dirMask |= PlayerInput::DIR_DOWN;
			} else {
				_g->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			}
			break;
		case kKeyBackspace:
			_g->_pi.backspace = pressed;
			break;
		case kKeyShift:
			_g->_pi.shift = pressed;
			break;
		case kKeyEscape:
			_g->_pi.escape = pressed;
			break;
		case kKeySpace:
			_g->_pi.space = pressed;
			break;
		case kKeyEnter:
			_g->_pi.enter = pressed;
			break;
		}
	}
	virtual int doTick() {
		if (_newState != _state) {
			switch (_state) { // fini
			}
			_state = _newState;
			switch (_state) { // init
			case kStateGame:
				break;
			case kStateInventory:
				_g->initInventory();
				break;
			case kStateStoryTexts:
				_g->initStoryTexts();
				break;
			case kStateCutscene:
				_g->_cut.initCutscene();
				break;
			}
		}
		switch (_state) {
		case kStateGame:
			_g->doGame();
			if (_g->_pgeLive[0].life > 0 && _g->_pgeLive[0].current_inventory_PGE != 0xFF) {
				if (_g->_pi.backspace) {
					_g->_pi.backspace = false;
					_newState = kStateInventory;
				}
			}
			if (_g->_textToDisplay != 0xFFFF) {
				_newState = kStateStoryTexts;
			}
			if (_g->_cut._id != 0xFFFF) {
				_newState = kStateCutscene;
			}
			break;
		case kStateInventory:
			_g->handleInventory();
			if (_g->_pi.backspace) {
				_g->_pi.backspace = false;
				_newState = kStateGame;
			}
			break;
		case kStateStoryTexts:
			_g->handleStoryTexts();
			if (_g->_textToDisplay == 0xFFFF) {
				_newState = kStateGame;
				break;
			}
		case kStateCutscene:
			if (_g->_cut._id == 0xFFFF) {
				_newState = kStateGame;
				break;
			}
			_g->_cut.playCutscene();
			if (_g->_cut._interrupted || _g->_cut._stop) {
				int id = _g->getNextCutscene(_g->_cut._id);
				if (id != 0xFFFF) {
					_g->_cut._id = id;
					_g->_cut.initCutscene();
				} else {
					_g->_cut._id = 0xFFFF;
					if (_g->_gameOver) {
						_g->loadLevelData();
						_g->resetGameState();
					}
					_newState = kStateGame;
				}
			}
			break;
		}
		return _g->_pi.quit;
	}
};

extern "C" {
	GameStub *GameStub_create() {
		return new GameStub_Flashback;
	}
};
