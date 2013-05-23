
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include "gl_texture.h"
#include "stub.h"

static const char *g_caption = "Flashback: The Quest For Identity";
static char g_errBuf[512];
static int g_w, g_h;

struct GetStub_impl {
	GameStub *getGameStub() {
		return GameStub_create();
	}
};

static void onExit() {
	if (g_errBuf[0]) {
		fprintf(stderr, "%s\n", g_errBuf);
	}
}

static void lockAudio(int lock) {
	if (lock) {
		SDL_LockAudio();
	} else {
		SDL_UnlockAudio();
	}
}

static void setupAudio(GameStub *stub) {
	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));
	desired.freq = 22050;
	desired.format = AUDIO_U8;
	desired.channels = 1;
	desired.samples = 2048;
	StubMixProc mix = stub->getMixProc(desired.freq, desired.format, lockAudio);
	if (mix.proc) {
		desired.callback = mix.proc;
		desired.userdata = mix.data;
		if (SDL_OpenAudio(&desired, 0) == 0) {
			SDL_PauseAudio(0);
		}
	}
}

static void eventKey(GameStub *stub, int key, int pressed) {
	switch (key) {
	case SDLK_LEFT:
		stub->eventKey(kKeyLeft, pressed);
		break;
	case SDLK_RIGHT:
		stub->eventKey(kKeyRight, pressed);
		break;
	case SDLK_UP:
		stub->eventKey(kKeyUp, pressed);
		break;
	case SDLK_DOWN:
		stub->eventKey(kKeyDown, pressed);
		break;
	case SDLK_BACKSPACE:
	case SDLK_TAB:
		stub->eventKey(kKeyBackspace, pressed);
		break;
	case SDLK_RSHIFT:
	case SDLK_LSHIFT:
		stub->eventKey(kKeyShift, pressed);
		break;
	case SDLK_ESCAPE:
		stub->eventKey(kKeyEscape, pressed);
		break;
	case SDLK_SPACE:
		stub->eventKey(kKeySpace, pressed);
		break;
	case SDLK_RETURN:
		stub->eventKey(kKeyEnter, pressed);
		break;
	}
}

static void emitQuadTex(int x1, int y1, int x2, int y2, GLfloat u1, GLfloat v1, GLfloat u2, GLfloat v2) {
	glBegin(GL_QUADS);
		glTexCoord2f(u1, v1);
		glVertex2i(x1, y1);
		glTexCoord2f(u2, v1);
		glVertex2i(x2, y1);
		glTexCoord2f(u2, v2);
		glVertex2i(x2, y2);
		glTexCoord2f(u1, v2);
		glVertex2i(x1, y2);
	glEnd();
}

static void drawGL(Texture &tex) {
	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, g_w, g_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, g_w, g_h, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT);
	if (tex._id != (GLuint)-1) {
		glBindTexture(GL_TEXTURE_2D, tex._id);
		emitQuadTex(0, 0, g_w, g_h, 0, 0, tex._u, tex._v);
	}
}

int main(int argc, char *argv[]) {
	GetStub_impl gs;
	GameStub *stub = gs.getGameStub();
	if (!stub) {
		return 0;
	}
	atexit(onExit);
	if (stub->init(argc - 1, argv + 1, g_errBuf) != 0) {
		return 1;
	}
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	setupAudio(stub);
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_SetCaption(g_caption, 0);
	StubBackBuffer buf = stub->getBackBuffer();
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	g_w = buf.w * 2;
	g_h = buf.h * 2;
	SDL_SetVideoMode(g_w, g_h, 0, SDL_OPENGL | SDL_RESIZABLE);
	Texture tex;
	Texture::init();
	bool quit = false;
	while (stub->doTick() == 0) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_VIDEORESIZE:
				g_w = ev.resize.w;
				g_h = ev.resize.h;
				SDL_SetVideoMode(g_w, g_h, 0, SDL_OPENGL | SDL_RESIZABLE);
				break;
			case SDL_KEYDOWN:
				eventKey(stub, ev.key.keysym.sym, 1);
				break;
			case SDL_KEYUP:
				eventKey(stub, ev.key.keysym.sym, 0);
				break;
			default:
				break;
			}
		}
		if (quit) {
			break;
		}
		tex.setPalette(buf.pal);
		tex.uploadData(buf.ptr, buf.w, buf.h);
		drawGL(tex);
		SDL_GL_SwapBuffers();
		SDL_Delay(30);
	}
	SDL_PauseAudio(1);
	stub->quit();
	SDL_Quit();
	return 0;
}
