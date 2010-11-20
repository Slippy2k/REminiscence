
#include <stdlib.h>
#include <SDL_opengl.h>
#include "game.h"
#include "mac_data.h"

static GLuint _textureId;

void Game::init() {
	decodeClutData(_res);
	uint8_t *ptr = decodeResourceData(_res, "Title 3", false);
	_textureId = decodeImageData("Title 3", ptr);
	printf("_textureId %d\n", _textureId);
	free(ptr);
}

void Game::doTick() {
}

void Game::doFrame() {
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
