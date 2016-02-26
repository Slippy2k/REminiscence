
#include <SDL.h>
#include <SDL_opengl.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "mixer.h"
#include "stub.h"

static const int kDefaultW = 512;
static const int kDefaultH = 448;
static const char *kCaption = "Flashback: The Quest For Identity";
static const float gScale = 2;
static int gWindowW = (int)(kDefaultW * gScale);
static int gWindowH = (int)(kDefaultH * gScale);
static const int gTickDuration = 16;

static const bool gUseDynLib = false;
static const char *gFbSoSym = "GameStub_create";
#ifdef _WIN32
static const char *gFbSoName = "fb.dll";
struct DynLib_impl {
	HINSTANCE _dlso;

	DynLib_impl()
		: _dlso(0) {
	}
	~DynLib_impl() {
		if (_dlso) {
			FreeLibrary(_dlso);
			_dlso = 0;
		}
	}
	void *open(const char *name) {
		_dlso = LoadLibrary(name);
		return _dlso;
	}
	void *getSymbol(const char *name) {
		return (void *)GetProcAddress(_dlso, name);
	}

};
#else
static const char *gFbSoName = "libfb.so";
struct DynLib_impl {
	void *_dlso;

	DynLib_impl()
		: _dlso(0) {
	}
	~DynLib_impl() {
		if (_dlso) {
			dlclose(_dlso);
			_dlso = 0;
		}
	}
	void *open(const char *name) {
		_dlso = dlopen(name, RTLD_NOW);
		return _dlso;
	}
	void *getSymbol(const char *name) {
		return dlsym(_dlso, name);
	}
};
#endif

static GameStub *getGameStub(DynLib_impl *lib) {
	if (lib->open(gFbSoName)) {
		void *proc = lib->getSymbol(gFbSoSym);
		if (proc) {
			typedef GameStub *(*createProc)();
			return ((createProc)proc)();
		}
	}
	return 0;
}

enum {
	kScaleUp = 1,
	kScaleDown = -1
};

static void rescaleWindowDim(int &w, int &h, int type) {
	static const float f[] = { 1., 1.5, 2., 2.5, 3., 3.5, 4. };
	int index = -1;
	int min = 0;
	for (int i = 0; i < 7; ++i) {
		const int x = (int)(kDefaultW * f[i]) - w;
		const int y = (int)(kDefaultH * f[i]) - h;
		int m = x * x + y * y;
		if (i == 0 || m < min) {
			min = m;
			index = i;
		}
	}
	if (index != -1) {
		index += type;
		if (index >= 0 && index <= 6) {
			w = (int)(kDefaultW * f[index]);
			h = (int)(kDefaultH * f[index]);
//			fprintf(stdout, "Set window dim to %d,%d (f=%f)\n", w, h, f[index]);
		}
	}
}

static void queueKeyInput(GameStub *stub, int keyCode, bool pressed) {
	int key = 0;
	switch (keyCode) {
	case SDL_SCANCODE_LEFT:
		key = kKeyCodeLeft;
		break;
	case SDL_SCANCODE_RIGHT:
		key = kKeyCodeRight;
		break;
	case SDL_SCANCODE_UP:
		key = kKeyCodeUp;
		break;
	case SDL_SCANCODE_DOWN:
		key = kKeyCodeDown;
		break;
	case SDL_SCANCODE_SPACE:
		key = kKeyCodeSpace;
		break;
	case SDL_SCANCODE_RSHIFT:
	case SDL_SCANCODE_LSHIFT:
		key = kKeyCodeShift;
		break;
	case SDL_SCANCODE_RETURN:
		key = kKeyCodeReturn;
		break;
	case SDL_SCANCODE_BACKSPACE:
		key = kKeyCodeBackspace;
		break;
	default:
		return;
	}
	stub->queueKeyInput(key, pressed);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s datafile level\n", argv[0]);
		return 0;
	}
	GameStub *stub = 0;
	DynLib_impl dl;
	if (gUseDynLib) {
		stub = getGameStub(&dl);
	} else {
		stub = GameStub_create();
	}
	if (!stub) {
		fprintf(stderr, "Unable to create GameStub\n");
		return -1;
	}
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
//	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_Window *window = SDL_CreateWindow(kCaption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, gWindowW, gWindowH, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_GetWindowSize(window, &gWindowW, &gWindowH);
	const char *saveDirectory = ".";
	const int level = (argc >= 3) ? atoi(argv[2]) : -1;
	if (stub->init(argv[1], saveDirectory, level)) {
		Mixer mix;
		stub->setMixerImpl(&mix);
		mix.init();
		stub->initGL(gWindowW, gWindowH);
		bool quitGame = false;
		while (!quitGame) {
			int w = gWindowW;
			int h = gWindowH;
			SDL_Event ev;
			while (SDL_PollEvent(&ev)) {
				switch (ev.type) {
				case SDL_QUIT:
					quitGame = true;
					break;
				case SDL_KEYDOWN:
					switch (ev.key.keysym.scancode) {
					case SDL_SCANCODE_PAGEUP:
						rescaleWindowDim(gWindowW, gWindowH, kScaleUp);
						break;
					case SDL_SCANCODE_PAGEDOWN:
						rescaleWindowDim(gWindowW, gWindowH, kScaleDown);
						break;
					default:
						queueKeyInput(stub, ev.key.keysym.scancode, 1);
						break;
					}
					break;
				case SDL_KEYUP:
					switch (ev.key.keysym.scancode) {
					case SDL_SCANCODE_PAGEUP:
					case SDL_SCANCODE_PAGEDOWN:
						break;
					default:
						queueKeyInput(stub, ev.key.keysym.scancode, 0);
						break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					stub->queueTouchInput(0, ev.button.x, ev.button.y, 0);
					break;
				case SDL_MOUSEBUTTONDOWN:
					stub->queueTouchInput(0, ev.button.x, ev.button.y, 1);
					break;
				case SDL_MOUSEMOTION:
					if (ev.motion.state & SDL_BUTTON(1)) {
						stub->queueTouchInput(0, ev.motion.x, ev.motion.y, 1);
					}
					break;
				case SDL_WINDOWEVENT:
					if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
						gWindowW = ev.window.data1;
						gWindowH = ev.window.data2;
					}
					break;
				}
			}
			if (w != gWindowW || h != gWindowH) {
				stub->initGL(gWindowW, gWindowH);
			}
			stub->doTick();
			stub->drawGL(gWindowW, gWindowH);
			SDL_RenderPresent(renderer);
			mix.update();
			SDL_Delay(gTickDuration);
		}
		stub->save();
		stub->quit();
		mix.quit();
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

