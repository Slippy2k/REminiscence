
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2018 Gregory Montoir (cyx@users.sourceforge.net)
 */

#include <SDL.h>
#include "scaler.h"
#include "screenshot.h"
#include "systemstub.h"
#include "util.h"

static const int kAudioHz = 22050;

static const char *kIconBmp = "icon.bmp";

static const int kJoystickIndex = 0;
static const int kJoystickCommitValue = 3200;

static const uint32_t kPixelFormat = SDL_PIXELFORMAT_RGB888;

ScalerParameters ScalerParameters::defaults() {
	ScalerParameters params;
	params.type = kScalerTypeInternal;
	params.scaler = &_internalScaler;
	params.factor = _internalScaler.factorMin + (_internalScaler.factorMax - _internalScaler.factorMin) / 2;
	return params;
}

struct SystemStub_SDL : SystemStub {
	SDL_Window *_window;
	SDL_Renderer *_renderer;
	SDL_Texture *_texture;
	int _texW, _texH;
	SDL_GameController *_controller;
	SDL_PixelFormat *_fmt;
	const char *_caption;
	uint32_t *_screenBuffer;
	bool _fullscreen;
	uint8_t _overscanColor;
	uint32_t _rgbPalette[256];
	uint32_t _darkPalette[256];
	int _screenW, _screenH;
	SDL_Joystick *_joystick;
	bool _fadeOnUpdateScreen;
	void (*_audioCbProc)(void *, int16_t *, int);
	void *_audioCbData;
	int _screenshot;
	ScalerType _scalerType;
	const Scaler *_scaler;
	int _scaleFactor;
	bool _widescreen;
	SDL_Texture *_wideTexture;
	int _wideMargin;

	virtual ~SystemStub_SDL() {}
	virtual void init(const char *title, int w, int h, bool fullscreen, bool widescreen, ScalerParameters *scalerParameters);
	virtual void destroy();
	virtual bool hasWidescreen() const;
	virtual void setScreenSize(int w, int h);
	virtual void setPalette(const uint8_t *pal, int n);
	virtual void setPaletteEntry(int i, const Color *c);
	virtual void getPaletteEntry(int i, Color *c);
	virtual void setOverscanColor(int i);
	virtual void copyRect(int x, int y, int w, int h, const uint8_t *buf, int pitch);
	virtual void copyRectRgb24(int x, int y, int w, int h, const uint8_t *rgb);
	virtual void copyRectLeftBorder(int w, int h, const uint8_t *buf);
	virtual void copyRectRightBorder(int w, int h, const uint8_t *buf);
	virtual void fadeScreen();
	virtual void updateScreen(int shakeOffset);
	virtual void processEvents();
	virtual void sleep(int duration);
	virtual uint32_t getTimeStamp();
	virtual void startAudio(AudioCallback callback, void *param);
	virtual void stopAudio();
	virtual uint32_t getOutputSampleRate();
	virtual void lockAudio();
	virtual void unlockAudio();

	void setPaletteColor(int color, int r, int g, int b);
	void processEvent(const SDL_Event &ev, bool &paused);
	void prepareGraphics();
	void cleanupGraphics();
	void changeGraphics(bool fullscreen, int scaleFactor);
	void changeScaler(int scaler);
	void drawRect(int x, int y, int w, int h, uint8_t color);
};

SystemStub *SystemStub_SDL_create() {
	return new SystemStub_SDL();
}

void SystemStub_SDL::init(const char *title, int w, int h, bool fullscreen, bool widescreen, ScalerParameters *scalerParameters) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
	SDL_ShowCursor(SDL_DISABLE);
	_caption = title;
	memset(&_pi, 0, sizeof(_pi));
	_window = 0;
	_renderer = 0;
	_texture = 0;
	_fmt = SDL_AllocFormat(kPixelFormat);
	_screenBuffer = 0;
	_fadeOnUpdateScreen = false;
	_fullscreen = fullscreen;
	_scalerType = scalerParameters->type;
	_scaler = scalerParameters->scaler;
	_scaleFactor = CLIP(scalerParameters->factor, _scaler->factorMin, _scaler->factorMax);
	memset(_rgbPalette, 0, sizeof(_rgbPalette));
	memset(_darkPalette, 0, sizeof(_darkPalette));
	_screenW = _screenH = 0;
	_widescreen = widescreen;
	_wideTexture = 0;
	_wideMargin = 0;
	setScreenSize(w, h);
	_joystick = 0;
	_controller = 0;
	if (SDL_NumJoysticks() > 0) {
		SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
		if (SDL_IsGameController(kJoystickIndex)) {
			_controller = SDL_GameControllerOpen(kJoystickIndex);
		}
		if (!_controller) {
			_joystick = SDL_JoystickOpen(kJoystickIndex);
		}
	}
	_screenshot = 1;
}

void SystemStub_SDL::destroy() {
	cleanupGraphics();
	if (_screenBuffer) {
		free(_screenBuffer);
		_screenBuffer = 0;
	}
	if (_fmt) {
		SDL_FreeFormat(_fmt);
		_fmt = 0;
	}
	if (_controller) {
		SDL_GameControllerClose(_controller);
		_controller = 0;
	}
	if (_joystick) {
		SDL_JoystickClose(_joystick);
		_joystick = 0;
	}
	SDL_Quit();
}

bool SystemStub_SDL::hasWidescreen() const {
	return _widescreen;
}

void SystemStub_SDL::setScreenSize(int w, int h) {
	if (_screenW == w && _screenH == h) {
		return;
	}
	cleanupGraphics();
	if (_screenBuffer) {
		free(_screenBuffer);
		_screenBuffer = 0;
	}
	const int screenBufferSize = w * h * sizeof(uint32_t);
	_screenBuffer = (uint32_t *)calloc(1, screenBufferSize);
	if (!_screenBuffer) {
		error("SystemStub_SDL::setScreenSize() Unable to allocate offscreen buffer, w=%d, h=%d", w, h);
	}
	_screenW = w;
	_screenH = h;
	prepareGraphics();
}

void SystemStub_SDL::setPaletteColor(int color, int r, int g, int b) {
	_rgbPalette[color] = SDL_MapRGB(_fmt, r, g, b);
	_darkPalette[color] = SDL_MapRGB(_fmt, r / 4, g / 4, b / 4);
}

void SystemStub_SDL::setPalette(const uint8_t *pal, int n) {
	assert(n <= 256);
	for (int i = 0; i < n; ++i) {
		setPaletteColor(i, pal[0], pal[1], pal[2]);
		pal += 3;
	}
}

void SystemStub_SDL::setPaletteEntry(int i, const Color *c) {
	setPaletteColor(i, c->r, c->g, c->b);
}

void SystemStub_SDL::getPaletteEntry(int i, Color *c) {
	SDL_GetRGB(_rgbPalette[i], _fmt, &c->r, &c->g, &c->b);
}

void SystemStub_SDL::setOverscanColor(int i) {
	_overscanColor = i;
}

void SystemStub_SDL::copyRect(int x, int y, int w, int h, const uint8_t *buf, int pitch) {
	if (x < 0) {
		x = 0;
	} else if (x >= _screenW) {
		return;
	}
	if (y < 0) {
		y = 0;
	} else if (y >= _screenH) {
		return;
	}
	if (x + w > _screenW) {
		w = _screenW - x;
	}
	if (y + h > _screenH) {
		h = _screenH - y;
	}

	uint32_t *p = _screenBuffer + y * _screenW + x;
	buf += y * pitch + x;

	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			p[i] = _rgbPalette[buf[i]];
		}
		p += _screenW;
		buf += pitch;
	}

	if (_pi.dbgMask & PlayerInput::DF_DBLOCKS) {
		drawRect(x, y, w, h, 0xE7);
	}
}

void SystemStub_SDL::copyRectRgb24(int x, int y, int w, int h, const uint8_t *rgb) {
	assert(x >= 0 && x + w <= _screenW && y >= 0 && y + h <= _screenH);
	uint32_t *p = _screenBuffer + y * _screenW + x;

	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			p[i] = SDL_MapRGB(_fmt, rgb[0], rgb[1], rgb[2]); rgb += 3;
		}
		p += _screenW;
	}

	if (_pi.dbgMask & PlayerInput::DF_DBLOCKS) {
		drawRect(x, y, w, h, 0xE7);
	}
}

static void clearTexture(SDL_Texture *texture, int h, SDL_PixelFormat *fmt) {
	void *dst = 0;
	int pitch = 0;
	if (SDL_LockTexture(texture, 0, &dst, &pitch) == 0) {
		assert((pitch & 3) == 0);
		const uint32_t color = SDL_MapRGB(fmt, 0, 0, 0);
		for (uint32_t i = 0; i < h * pitch / sizeof(uint32_t); ++i) {
			((uint32_t *)dst)[i] = color;
		}
		SDL_UnlockTexture(texture);
	}
}

void SystemStub_SDL::copyRectLeftBorder(int w, int h, const uint8_t *buf) {
	assert(w >= _wideMargin);
	uint32_t *rgb = (uint32_t *)malloc(w * h * sizeof(uint32_t));
	if (rgb) {
		if (buf) {
			for (int i = 0; i < w * h; ++i) {
				rgb[i] = _darkPalette[buf[i]];
			}
		} else {
			const uint32_t color = SDL_MapRGB(_fmt, 0, 0, 0);
			for (int i = 0; i < w * h; ++i) {
				rgb[i] = color;
			}
		}
		const int xOffset = w - _wideMargin;
		SDL_Rect r;
		r.x = 0;
		r.y = 0;
		r.w = _wideMargin;
		r.h = h;
		SDL_UpdateTexture(_wideTexture, &r, rgb + xOffset, w * sizeof(uint32_t));
		free(rgb);
	}
}

void SystemStub_SDL::copyRectRightBorder(int w, int h, const uint8_t *buf) {
	assert(w >= _wideMargin);
	uint32_t *rgb = (uint32_t *)malloc(w * h * sizeof(uint32_t));
	if (rgb) {
		if (buf) {
			for (int i = 0; i < w * h; ++i) {
				rgb[i] = _darkPalette[buf[i]];
			}
		} else {
			const uint32_t color = SDL_MapRGB(_fmt, 0, 0, 0);
			for (int i = 0; i < w * h; ++i) {
				rgb[i] = color;
			}
		}
		const int xOffset = 0;
		SDL_Rect r;
		r.x = _wideMargin + _screenW;
		r.y = 0;
		r.w = _wideMargin;
		r.h = h;
		SDL_UpdateTexture(_wideTexture, &r, rgb + xOffset, w * sizeof(uint32_t));
		free(rgb);
	}
}

void SystemStub_SDL::fadeScreen() {
	_fadeOnUpdateScreen = true;
}

void SystemStub_SDL::updateScreen(int shakeOffset) {
	if (_texW != _screenW || _texH != _screenH) {
		void *dst = 0;
		int pitch = 0;
		if (SDL_LockTexture(_texture, 0, &dst, &pitch) == 0) {
			assert((pitch & 3) == 0);
			_scaler->scale(_scaleFactor, (uint32_t *)dst, pitch / sizeof(uint32_t), _screenBuffer, _screenW, _screenW, _screenH);
			SDL_UnlockTexture(_texture);
		}
	} else {
		SDL_UpdateTexture(_texture, 0, _screenBuffer, _screenW * sizeof(uint32_t));
	}
	SDL_RenderClear(_renderer);
	if (_widescreen) {
		// borders / background screen
		SDL_RenderCopy(_renderer, _wideTexture, 0, 0);
		// game screen
		SDL_Rect r;
		r.y = shakeOffset * _scaleFactor;
		SDL_RenderGetLogicalSize(_renderer, &r.w, &r.h);
		r.x = (r.w - _texW) / 2;
		r.w = _texW;
		SDL_RenderCopy(_renderer, _texture, 0, &r);
	} else {
		if (_fadeOnUpdateScreen) {
			SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
			SDL_Rect r;
			r.x = r.y = 0;
			SDL_RenderGetLogicalSize(_renderer, &r.w, &r.h);
			for (int i = 1; i <= 16; ++i) {
				SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 256 - i * 16);
				SDL_RenderCopy(_renderer, _texture, 0, 0);
				SDL_RenderFillRect(_renderer, &r);
				SDL_RenderPresent(_renderer);
				SDL_Delay(30);
			}
			_fadeOnUpdateScreen = false;
			SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_NONE);
			return;
		}
		SDL_Rect r;
		r.x = 0;
		r.y = shakeOffset * _scaleFactor;
		SDL_RenderGetLogicalSize(_renderer, &r.w, &r.h);
		SDL_RenderCopy(_renderer, _texture, 0, &r);
	}
	SDL_RenderPresent(_renderer);
}

void SystemStub_SDL::processEvents() {
	bool paused = false;
	while (true) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			processEvent(ev, paused);
			if (_pi.quit) {
				return;
			}
		}
		if (!paused) {
			break;
		}
		SDL_Delay(100);
	}
}

void SystemStub_SDL::processEvent(const SDL_Event &ev, bool &paused) {
	switch (ev.type) {
	case SDL_QUIT:
		_pi.quit = true;
		break;
	case SDL_WINDOWEVENT:
		switch (ev.window.event) {
		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			paused = (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST);
			SDL_PauseAudio(paused);
			break;
		}
		break;
	case SDL_JOYHATMOTION:
		if (_joystick) {
			_pi.dirMask = 0;
			if (ev.jhat.value & SDL_HAT_UP) {
				_pi.dirMask |= PlayerInput::DIR_UP;
			}
			if (ev.jhat.value & SDL_HAT_DOWN) {
				_pi.dirMask |= PlayerInput::DIR_DOWN;
			}
			if (ev.jhat.value & SDL_HAT_LEFT) {
				_pi.dirMask |= PlayerInput::DIR_LEFT;
			}
			if (ev.jhat.value & SDL_HAT_RIGHT) {
				_pi.dirMask |= PlayerInput::DIR_RIGHT;
			}
		}
		break;
	case SDL_JOYAXISMOTION:
		if (_joystick) {
			switch (ev.jaxis.axis) {
			case 0:
				_pi.dirMask &= ~(PlayerInput::DIR_RIGHT | PlayerInput::DIR_LEFT);
				if (ev.jaxis.value > kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_RIGHT;
				} else if (ev.jaxis.value < -kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_LEFT;
				}
				break;
			case 1:
				_pi.dirMask &= ~(PlayerInput::DIR_UP | PlayerInput::DIR_DOWN);
				if (ev.jaxis.value > kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_DOWN;
				} else if (ev.jaxis.value < -kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_UP;
				}
				break;
			}
		}
		break;
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		if (_joystick) {
			const bool pressed = (ev.jbutton.state == SDL_PRESSED);
			switch (ev.jbutton.button) {
			case 0:
				_pi.space = pressed;
				break;
			case 1:
				_pi.shift = pressed;
				break;
			case 2:
				_pi.enter = pressed;
				break;
			case 3:
				_pi.backspace = pressed;
				break;
			}
		}
		break;
	case SDL_CONTROLLERAXISMOTION:
		if (_controller) {
			switch (ev.caxis.axis) {
			case SDL_CONTROLLER_AXIS_LEFTX:
			case SDL_CONTROLLER_AXIS_RIGHTX:
				if (ev.caxis.value < -kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_LEFT;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_LEFT;
				}
				if (ev.caxis.value > kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_RIGHT;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
				}
				break;
			case SDL_CONTROLLER_AXIS_LEFTY:
			case SDL_CONTROLLER_AXIS_RIGHTY:
				if (ev.caxis.value < -kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_UP;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_UP;
				}
				if (ev.caxis.value > kJoystickCommitValue) {
					_pi.dirMask |= PlayerInput::DIR_DOWN;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_DOWN;
				}
				break;
			}
		}
		break;
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		if (_controller) {
			const bool pressed = (ev.cbutton.state == SDL_PRESSED);
			switch (ev.cbutton.button) {
			case SDL_CONTROLLER_BUTTON_A:
				_pi.enter = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_B:
				_pi.space = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_X:
				_pi.shift = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				_pi.backspace = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_BACK:
			case SDL_CONTROLLER_BUTTON_START:
				_pi.escape = pressed;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				if (pressed) {
					_pi.dirMask |= PlayerInput::DIR_UP;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_UP;
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				if (pressed) {
					_pi.dirMask |= PlayerInput::DIR_DOWN;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_DOWN;
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				if (pressed) {
					_pi.dirMask |= PlayerInput::DIR_LEFT;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_LEFT;
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				if (pressed) {
					_pi.dirMask |= PlayerInput::DIR_RIGHT;
				} else {
					_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
				}
				break;
			}
		}
		break;
	case SDL_KEYUP:
		if (ev.key.keysym.mod & KMOD_ALT) {
			switch (ev.key.keysym.sym) {
			case SDLK_RETURN:
				changeGraphics(!_fullscreen, _scaleFactor);
				break;
			case SDLK_KP_PLUS:
			case SDLK_PAGEUP:
				changeGraphics(_fullscreen, _scaleFactor + 1);
				break;
			case SDLK_KP_MINUS:
			case SDLK_PAGEDOWN:
				changeGraphics(_fullscreen, _scaleFactor - 1);
				break;
			case SDLK_s: {
					char name[32];
					snprintf(name, sizeof(name), "screenshot-%03d.tga", _screenshot);
					saveTGA(name, (const uint8_t *)_screenBuffer, _screenW, _screenH);
					++_screenshot;
					debug(DBG_INFO, "Written '%s'", name);
				}
				break;
			case SDLK_x:
				_pi.quit = true;
				break;
			}
			break;
		} else if (ev.key.keysym.mod & KMOD_CTRL) {
			switch (ev.key.keysym.sym) {
			case SDLK_f:
				_pi.dbgMask ^= PlayerInput::DF_FASTMODE;
				break;
			case SDLK_b:
				_pi.dbgMask ^= PlayerInput::DF_DBLOCKS;
				break;
			case SDLK_i:
				_pi.dbgMask ^= PlayerInput::DF_SETLIFE;
				break;
			case SDLK_s:
				_pi.save = true;
				break;
			case SDLK_l:
				_pi.load = true;
				break;
			case SDLK_KP_PLUS:
			case SDLK_PAGEUP:
				_pi.stateSlot = 1;
				break;
			case SDLK_KP_MINUS:
			case SDLK_PAGEDOWN:
				_pi.stateSlot = -1;
				break;
			}
			break;
		}
		_pi.lastChar = ev.key.keysym.sym;
		switch (ev.key.keysym.sym) {
		case SDLK_LEFT:
			_pi.dirMask &= ~PlayerInput::DIR_LEFT;
			break;
		case SDLK_RIGHT:
			_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
			break;
		case SDLK_UP:
			_pi.dirMask &= ~PlayerInput::DIR_UP;
			break;
		case SDLK_DOWN:
			_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			break;
		case SDLK_SPACE:
			_pi.space = false;
			break;
		case SDLK_RSHIFT:
		case SDLK_LSHIFT:
			_pi.shift = false;
			break;
		case SDLK_RETURN:
			_pi.enter = false;
			break;
		case SDLK_ESCAPE:
			_pi.escape = false;
			break;
		case SDLK_F1:
		case SDLK_F2:
		case SDLK_F3:
		case SDLK_F4:
		case SDLK_F5:
		case SDLK_F6:
		case SDLK_F7:
		case SDLK_F8:
			changeScaler(ev.key.keysym.sym - SDLK_F1);
			break;
		default:
			break;
		}
		break;
	case SDL_KEYDOWN:
		if (ev.key.keysym.mod & (KMOD_ALT | KMOD_CTRL)) {
			break;
		}
		switch (ev.key.keysym.sym) {
		case SDLK_LEFT:
			_pi.dirMask |= PlayerInput::DIR_LEFT;
			break;
		case SDLK_RIGHT:
			_pi.dirMask |= PlayerInput::DIR_RIGHT;
			break;
		case SDLK_UP:
			_pi.dirMask |= PlayerInput::DIR_UP;
			break;
		case SDLK_DOWN:
			_pi.dirMask |= PlayerInput::DIR_DOWN;
			break;
		case SDLK_BACKSPACE:
		case SDLK_TAB:
			_pi.backspace = true;
			break;
		case SDLK_SPACE:
			_pi.space = true;
			break;
		case SDLK_RSHIFT:
		case SDLK_LSHIFT:
			_pi.shift = true;
			break;
		case SDLK_RETURN:
			_pi.enter = true;
			break;
		case SDLK_ESCAPE:
			_pi.escape = true;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void SystemStub_SDL::sleep(int duration) {
	SDL_Delay(duration);
}

uint32_t SystemStub_SDL::getTimeStamp() {
	return SDL_GetTicks();
}

static void mixAudioS16(void *param, uint8_t *buf, int len) {
	SystemStub_SDL *stub = (SystemStub_SDL *)param;
	memset(buf, 0, len);
	stub->_audioCbProc(stub->_audioCbData, (int16_t *)buf, len / 2);
}

void SystemStub_SDL::startAudio(AudioCallback callback, void *param) {
	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));
	desired.freq = kAudioHz;
	desired.format = AUDIO_S16SYS;
	desired.channels = 1;
	desired.samples = 2048;
	desired.callback = mixAudioS16;
	desired.userdata = this;
	if (SDL_OpenAudio(&desired, 0) == 0) {
		_audioCbProc = callback;
		_audioCbData = param;
		SDL_PauseAudio(0);
	} else {
		error("SystemStub_SDL::startAudio() Unable to open sound device");
	}
}

void SystemStub_SDL::stopAudio() {
	SDL_CloseAudio();
}

uint32_t SystemStub_SDL::getOutputSampleRate() {
	return kAudioHz;
}

void SystemStub_SDL::lockAudio() {
	SDL_LockAudio();
}

void SystemStub_SDL::unlockAudio() {
	SDL_UnlockAudio();
}

void SystemStub_SDL::prepareGraphics() {
	_texW = _screenW;
	_texH = _screenH;
	switch (_scalerType) {
	case kScalerTypePoint:
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest pixel sampling
		break;
	case kScalerTypeLinear:
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // linear filtering
		break;
	case kScalerTypeInternal:
	case kScalerTypeExternal:
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		_texW *= _scaleFactor;
		_texH *= _scaleFactor;
		break;
	}
	int windowW = _screenW * _scaleFactor;
	int windowH = _screenH * _scaleFactor;
	int flags = 0;
	if (_fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	} else {
		flags |= SDL_WINDOW_RESIZABLE;
	}
	if (_widescreen) {
		windowW = windowH * 16 / 9;
	}
	_window = SDL_CreateWindow(_caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowW, windowH, flags);
	SDL_Surface *icon = SDL_LoadBMP(kIconBmp);
	if (icon) {
		SDL_SetWindowIcon(_window, icon);
		SDL_FreeSurface(icon);
	}
	_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
	SDL_RenderSetLogicalSize(_renderer, windowW, windowH);
	_texture = SDL_CreateTexture(_renderer, kPixelFormat, SDL_TEXTUREACCESS_STREAMING, _texW, _texH);
	if (_widescreen) {
		const int w = _screenH * 16 / 9;
		const int h = _screenH;
		_wideTexture = SDL_CreateTexture(_renderer, kPixelFormat, SDL_TEXTUREACCESS_STREAMING, w, h);
		clearTexture(_wideTexture, _screenH, _fmt);

		// left and right borders
		_wideMargin = (w - _screenW) / 2;
	}
}

void SystemStub_SDL::cleanupGraphics() {
	if (_texture) {
		SDL_DestroyTexture(_texture);
		_texture = 0;
	}
	if (_wideTexture) {
		SDL_DestroyTexture(_wideTexture);
		_wideTexture = 0;
	}
	if (_renderer) {
		SDL_DestroyRenderer(_renderer);
		_renderer = 0;
	}
	if (_window) {
		SDL_DestroyWindow(_window);
		_window = 0;
	}
}

void SystemStub_SDL::changeGraphics(bool fullscreen, int scaleFactor) {
	int factor = CLIP(scaleFactor, _scaler->factorMin, _scaler->factorMax);
	if (fullscreen == _fullscreen && factor == _scaleFactor) {
		// no change
		return;
	}
	_fullscreen = fullscreen;
	_scaleFactor = factor;
	cleanupGraphics();
	prepareGraphics();
}

void SystemStub_SDL::changeScaler(int scaler) {
	ScalerParameters scalerParameters = ScalerParameters::defaults();
	switch (scaler) {
	case 0:
		scalerParameters.type = kScalerTypePoint;
		break;
	case 1:
		scalerParameters.type = kScalerTypeLinear;
		break;
	case 2:
		scalerParameters.type = kScalerTypeInternal;
		scalerParameters.scaler = &_internalScaler;
		break;
#ifdef USE_STATIC_SCALER
	case 3:
		scalerParameters.type = kScalerTypeInternal;
		scalerParameters.scaler = &scaler_nearest;
		break;
	case 4:
		scalerParameters.type = kScalerTypeInternal;
		scalerParameters.scaler = &scaler_tv2x;
		break;
	case 5:
		scalerParameters.type = kScalerTypeInternal;
		scalerParameters.scaler = &scaler_xbrz;
		break;
#endif
	default:
		return;
	}
	if (_scalerType != scalerParameters.type || scalerParameters.scaler != _scaler) {
		_scalerType = scalerParameters.type;
		_scaler = scalerParameters.scaler;
		const int scaleFactor = CLIP(_scaleFactor, _scaler->factorMin, _scaler->factorMax);
		// only recreate the window if dimensions actually changed
		if (scaleFactor != _scaleFactor) {
			cleanupGraphics();
			_scaleFactor = scaleFactor;
			prepareGraphics();
		}
	}
}

void SystemStub_SDL::drawRect(int x, int y, int w, int h, uint8_t color) {
	const int x1 = x;
	const int y1 = y;
	const int x2 = x + w - 1;
	const int y2 = y + h - 1;
	assert(x1 >= 0 && x2 < _screenW && y1 >= 0 && y2 < _screenH);
	for (int i = x1; i <= x2; ++i) {
		*(_screenBuffer + y1 * _screenW + i) = *(_screenBuffer + y2 * _screenW + i) = _rgbPalette[color];
	}
	for (int j = y1; j <= y2; ++j) {
		*(_screenBuffer + j * _screenW + x1) = *(_screenBuffer + j * _screenW + x2) = _rgbPalette[color];
	}
}
