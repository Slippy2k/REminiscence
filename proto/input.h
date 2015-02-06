
#ifndef INPUT_H__
#define INPUT_H__

#include "intern.h"

struct PadInput {

	enum {
		kIdle,
		kDirectionPressed,
		kButtonPressed
	};

	int _state;
	int _refX0, _refY0, _dirX, _dirY;

	PadInput()
		: _state(kIdle) {
	}

	bool feed(int x, int y, bool pressed, PlayerInput &pi);
};

#endif
