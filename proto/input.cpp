
#include <math.h>
#include "input.h"


static int squareDist(int dy, int dx) {
	return dy * dy + dx * dx;
}

bool PadInput::feed(int x, int y, bool pressed, PlayerInput &pi) {
	switch (_state) {
	case kIdle:
		if (pressed) {
			if (x > 256 && x < 512 && y > 96 && y < 448) {
				_state = kButtonPressed;
				_refX0 = x;
				_refY0 = y;
				pi.shift = true;
				return true;
			}
			if (x > 0 && x < 256 && y > 0 && y < 448) {
				_state = kDirectionPressed;
				_refX0 = _dirX = x;
				_refY0 = _dirY = y;
				return true;
			}
		}
		return false;
	case kDirectionPressed:
		pi.dirMask = 0;
		if (!pressed) {
			_state = kIdle;
		} else {
			_dirX = x;
			_dirY = y;
			const int dist = squareDist(_dirY - _refY0, _dirX - _refX0);
			if (dist >= 1024) {
				int deg = (int)(atan2(_dirY - _refY0, _dirX - _refX0) * 180 / M_PI);
				if (deg > 0 && deg <= 180) {
					deg = 360 - deg;
				} else {
					deg = -deg;
				}
				if (deg >= 45 && deg < 135) {
					pi.dirMask = PlayerInput::kDirectionUp;
				} else if (deg >= 135 && deg < 225) {
					pi.dirMask = PlayerInput::kDirectionLeft;
				} else if (deg >= 225 && deg < 315) {
					pi.dirMask = PlayerInput::kDirectionDown;
				} else {
					pi.dirMask = PlayerInput::kDirectionRight;
				}
			}
		}
		return true;
	case kButtonPressed:
		if (!pressed) {
			_state = kIdle;
		} else {
			_refX0 = x;
			_refY0 = y;
		}
		pi.shift = pressed;
		return true;
	}
	return false;
}

