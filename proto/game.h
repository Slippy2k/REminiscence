
#ifndef GAME_H__
#define GAME_H__

#include <stdint.h>
#include "resource_mac.h"

struct Game {
	ResourceMac _res;

	Game(const char *filePath)
		: _res(filePath) {
	}

	void init();

	void doTick();
	void doFrame();
};

#endif

