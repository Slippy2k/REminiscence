
#include <SDL.h>
#include <SDL_opengl.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "stub.h"

static const int kDefaultW = 512;
static const int kDefaultH = 448;
static const char *gWindowTitle = "Flashback: The Quest For Identity";
static const float gScale = 1.5;
static int gWindowW = (int)(kDefaultW * gScale);
static int gWindowH = (int)(kDefaultH * gScale);
static const int gTickDuration = 16;

static const char *gFbSoSym = "g_stub";
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

static void setupAudio(Stub *stub) {
	if (stub->getMixProc) {
		SDL_AudioSpec desired;
		memset(&desired, 0, sizeof(desired));
		desired.freq = 11025;
		desired.format = AUDIO_S8;
		desired.channels = 1;
		desired.samples = 2048;
		StubMixProc mix = stub->getMixProc(desired.freq, desired.format);
		desired.callback = mix.proc;
		desired.userdata = mix.data;
		if (SDL_OpenAudio(&desired, 0) == 0) {
			SDL_PauseAudio(0);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s datafile level\n", argv[0]);
		return 0;
	}
	DynLib_impl dl;
	dl.open(gFbSoName);
	if (!dl._dlso) {
		fprintf(stderr, "unable to open '%s'\n", gFbSoName);
		return 0;
	}
	Stub *stub = (struct Stub *)dl.getSymbol(gFbSoSym);
	if (!stub) {
		fprintf(stderr, "unable to lookup symbol '%s'\n", gFbSoSym);
		return 0;
	}
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
	SDL_WM_SetCaption(gWindowTitle, 0);
	const char *saveDirectory = ".";
	const int level = (argc >= 3) ? atoi(argv[2]) : -1;
	stub->init(argv[1], saveDirectory, level);
	setupAudio(stub);
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
				switch (ev.key.keysym.sym) {
				case SDLK_PAGEUP:
					rescaleWindowDim(gWindowW, gWindowH, kScaleUp);
					break;
				case SDLK_PAGEDOWN:
					rescaleWindowDim(gWindowW, gWindowH, kScaleDown);
					break;
				default:
					stub->queueKeyInput(ev.key.keysym.sym, 1);
					break;
				}
				break;
			case SDL_KEYUP:
				switch (ev.key.keysym.sym) {
				case SDLK_PAGEUP:
				case SDLK_PAGEDOWN:
					break;
				default:
					stub->queueKeyInput(ev.key.keysym.sym, 0);
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
			case SDL_VIDEORESIZE:
				gWindowW = ev.resize.w;
				gWindowH = ev.resize.h;
				break;
			}
		}
		if (w != gWindowW || h != gWindowH) {
			SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
			stub->initGL(gWindowW, gWindowH);
		}
		SDL_LockAudio();
		stub->doTick();
		SDL_UnlockAudio();
		stub->drawGL(gWindowW, gWindowH);
		SDL_GL_SwapBuffers();
		SDL_Delay(gTickDuration);
	}
	stub->save();
	stub->quit();
	return 0;
}


