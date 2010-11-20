
#include <SDL.h>
#include <SDL_opengl.h>
#include "game.h"

static const char *gWindowWindowTitle = "Flashback: The Quest For Identity";
static const int gWindowW = 512;
static const int gWindowH = 448;
static const int gTickDuration = 10;

static void doFrame(int w, int h) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
//	glScalef(1., -1., 1.);
}

static void doTick() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1., 0., 0.);
	glBegin(GL_TRIANGLE_FAN);
		glVertex2i(0, 0);
		glVertex2i(gWindowW / 2, 0);
		glVertex2i(0, gWindowH / 2);
	glEnd();
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("%s datafile", argv[0]);
		return 0;
	}
	Game game(argv[1]);
	SDL_Init(SDL_INIT_VIDEO);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
	SDL_WM_SetCaption(gWindowWindowTitle, 0);
	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, gWindowW, gWindowH);
	bool quitGame = false;
	game.init();
	while (!quitGame) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				quitGame = true;
				break;
			}
		}
//		doTick();
		game.doTick();
		doFrame(gWindowW, gWindowH);
		game.doFrame();
		SDL_GL_SwapBuffers();
		SDL_Delay(gTickDuration);
	}
	return 0;
}

