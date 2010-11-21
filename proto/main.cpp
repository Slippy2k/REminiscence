
#include <SDL.h>
#include <SDL_opengl.h>
#include "game.h"
#include "resource_data.h"
#include "resource_mac.h"
#include "util.h"

static const char *gWindowWindowTitle = "Flashback: The Quest For Identity";
static const int gWindowW = 512;
static const int gWindowH = 448;
static const int gTickDuration = 10;

struct Test {
	ResourceData _resData;
	GLuint _textureId;

	Test(const char *filePath)
		: _resData(filePath) {
	}

	void init() {
		_resData.loadClutData();
		_resData.loadIconData();
		_resData.loadPersoData();

		uint8_t *ptr = _resData.decodeResourceData("Title 3", false);
		_textureId = decodeImageData(_resData, "Title 3", ptr);
		printf("_textureId %d\n", _textureId);
		free(ptr);
	}

	void doFrame() {
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
};

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

static void uploadTexture(GLuint textureId, const uint8_t *imageData, const Color *clut, int w, int h) {
	uint8_t *texData = (uint8_t *)malloc(w * h * 3);
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			const Color &c = clut[imageData[y * w + x]];
			texData[(y * w + x) * 3] = c.r;
			texData[(y * w + x) * 3 + 1] = c.g;
			texData[(y * w + x) * 3 + 2] = c.b;
		}
	}
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
	free(texData);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("%s datafile", argv[0]);
		return 0;
	}
	SDL_Init(SDL_INIT_VIDEO);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetVideoMode(gWindowW, gWindowH, 0, SDL_OPENGL | SDL_RESIZABLE);
	SDL_WM_SetCaption(gWindowWindowTitle, 0);
	glEnable(GL_TEXTURE_2D);

	Test t(argv[1]);
	t.init();
	Game game(t._resData);
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
			}
		}
		game.doTick();
		doFrame(gWindowW, gWindowH);
		uploadTexture(t._textureId, game._frontLayer, game._palette, Game::kScreenWidth, Game::kScreenHeight);
		t.doFrame();
		SDL_GL_SwapBuffers();
		SDL_Delay(gTickDuration);
	}
	return 0;
}

