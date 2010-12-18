
#include <SDL.h>
#include <SDL_opengl.h>
#include "game.h"
#include "resource_data.h"
#include "resource_mac.h"
#include "util.h"

static const char *gWindowWindowTitle = "Flashback: The Quest For Identity";
static const int gWindowW = 512;
static const int gWindowH = 448;
static const int gTickDuration = 16;

struct Test {
	ResourceData _resData;
	GLuint _textureId;

	Test(const char *filePath)
		: _resData(filePath), _textureId(-1) {
	}

	void init() {
		_resData.loadClutData();
		_resData.loadIconData();
		_resData.loadFontData();
		_resData.loadPersoData();
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

	void uploadTexture(const uint8_t *imageData, const Color *clut, int w, int h) {
		uint8_t *texData = (uint8_t *)malloc(w * h * 3);
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				const Color &c = clut[imageData[y * w + x]];
				texData[(y * w + x) * 3] = c.r;
				texData[(y * w + x) * 3 + 1] = c.g;
				texData[(y * w + x) * 3 + 2] = c.b;
			}
		}
		if (_textureId == -1) {
			glGenTextures(1, &_textureId);
			glBindTexture(GL_TEXTURE_2D, _textureId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
		} else {
			glBindTexture(GL_TEXTURE_2D, _textureId);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, texData);
		}
		free(texData);
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
	SDL_WM_SetCaption(gWindowWindowTitle, 0);
	glEnable(GL_TEXTURE_2D);

	Test t(argv[1]);
	t.init();
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
		t.uploadTexture(game._frontLayer, game._palette, Game::kScreenWidth, Game::kScreenHeight);
		t.doFrame(gWindowW, gWindowH);
		SDL_GL_SwapBuffers();
		SDL_Delay(gTickDuration);
	}
	return 0;
}

