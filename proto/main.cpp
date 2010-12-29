
#include <SDL.h>
#include <SDL_opengl.h>
#include <dlfcn.h>
#include "stub.h"

static const char *gWindowTitle = "Flashback: The Quest For Identity";
static const int gWindowW = 512;
static const int gWindowH = 448;
static const int gTickDuration = 16;

static const char *gFbSoName = "libfb.so";
static const char *gFbSoSym = "g_stub";

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s datafile level\n", argv[0]);
		return 0;
	}
	void *dlFbSo = dlopen(gFbSoName, RTLD_LAZY);
	if (!dlFbSo) {
		fprintf(stderr, "unable to open '%s'\n", gFbSoName);
		return 0;
	}
	Stub *stub = (struct Stub *)dlsym(dlFbSo, gFbSoSym);
	if (!stub) {
		fprintf(stderr, "unable to lookup symbol '%s'\n", gFbSoSym);
		dlclose(dlFbSo);
		return 0;
	}
	SDL_Init(SDL_INIT_VIDEO);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
	SDL_WM_SetCaption(gWindowTitle, 0);
	const int level = (argc >= 3) ? atoi(argv[2]) : -1;
	stub->init(argv[1], level);
	stub->initGL(gWindowW, gWindowH);
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
				stub->queueKeyInput(ev.key.keysym.sym, 1);
				break;
			case SDL_KEYUP:
				stub->queueKeyInput(ev.key.keysym.sym, 0);
				break;
			case SDL_MOUSEBUTTONUP:
				stub->queueTouchInput(ev.button.x, ev.button.y, 1);
				break;
			case SDL_MOUSEBUTTONDOWN:
				stub->queueTouchInput(ev.button.x, ev.button.y, 0);
				break;
			case SDL_MOUSEMOTION:
				if (ev.motion.state & SDL_BUTTON(1)) {
					stub->queueTouchInput(ev.motion.x, ev.motion.y, 0);
				}
				break;
			}
		}
		stub->doTick();
		stub->drawGL(gWindowW, gWindowH);
		SDL_GL_SwapBuffers();
		SDL_Delay(gTickDuration);
	}
	dlclose(dlFbSo);
	return 0;
}


