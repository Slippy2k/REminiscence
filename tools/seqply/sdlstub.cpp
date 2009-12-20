
#include <SDL.h>
#include "systemstub.h"


struct SDLStub : SystemStub {
	typedef void (SDLStub::*ScaleProc)(uint16 *dst, uint16 dstPitch, const uint16 *src, uint16 srcPitch, uint16 w, uint16 h);

	struct Scaler {
		const char *name;
		ScaleProc proc;
		uint8 factor;
	};

	static const Scaler _scalers[];

	uint8 *_offscreen;
	SDL_Surface *_screen;
	SDL_Surface *_sclscreen;
	uint8 _scaler;
	uint16 _pal[256];
	uint16 _screenW, _screenH;

	virtual ~SDLStub() {}
	virtual void init(const char *title, uint16 w, uint16 h);
	virtual void destroy();
	virtual void setPalette(const uint8 *pal, uint16 n);
	virtual void copyRect(uint16 x, uint16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch);
	virtual void startAudio(AudioCallback callback, void *param);
	virtual void stopAudio();
	virtual bool processEvents();
	virtual void sleep(uint32 duration);
	virtual uint32 getTimeStamp();

	void prepareGfxMode();
	void cleanupGfxMode();
	void switchGfxMode(uint8 scaler);

	void point1x(uint16 *dst, uint16 dstPitch, const uint16 *src, uint16 srcPitch, uint16 w, uint16 h);
	void point2x(uint16 *dst, uint16 dstPitch, const uint16 *src, uint16 srcPitch, uint16 w, uint16 h);
	void point3x(uint16 *dst, uint16 dstPitch, const uint16 *src, uint16 srcPitch, uint16 w, uint16 h);
};

const SDLStub::Scaler SDLStub::_scalers[] = {
	{ "Point1x", &SDLStub::point1x, 1 },
	{ "Point2x", &SDLStub::point2x, 2 },
	{ "Point3x", &SDLStub::point3x, 3 }
};

SystemStub *SystemStub_SDL_create() {
	return new SDLStub();
}

void SDLStub::init(const char *title, uint16 w, uint16 h) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_SetCaption(title, NULL);
	_screenW = w;
	_screenH = h;
	_offscreen = (uint8 *)malloc(_screenW * _screenH * 2);
	if (!_offscreen) {
		error("Unable to allocate offscreen buffer");
	}
	_scaler = 1;
	memset(_pal, 0, sizeof(_pal));
	prepareGfxMode();
}

void SDLStub::destroy() {
	cleanupGfxMode();
	SDL_Quit();
}

void SDLStub::setPalette(const uint8 *pal, uint16 n) {
	assert(n <= 256);
	for (int i = 0; i < n; ++i) {
		uint8 r = (pal[i * 3 + 0] << 2) | (pal[i * 3 + 0] & 3);
		uint8 g = (pal[i * 3 + 1] << 2) | (pal[i * 3 + 1] & 3);
		uint8 b = (pal[i * 3 + 2] << 2) | (pal[i * 3 + 2] & 3);
		_pal[i] = SDL_MapRGB(_screen->format, r, g, b);
	}	
}

void SDLStub::copyRect(uint16 x, uint16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch) {
	buf += y * pitch + x;
	uint16 *p = (uint16 *)_offscreen;
	while (h--) {
		for (int i = 0; i < w; ++i) {
			p[i] = _pal[buf[i]];
		}
		p += _screenW;
		buf += pitch;
	}
	SDL_LockSurface(_sclscreen);
	(this->*_scalers[_scaler].proc)((uint16 *)_sclscreen->pixels, _sclscreen->pitch, (uint16 *)_offscreen, _screenW, _screenW, _screenH);
	SDL_UnlockSurface(_sclscreen);
	SDL_BlitSurface(_sclscreen, NULL, _screen, NULL);
	SDL_UpdateRect(_screen, 0, 0, 0, 0);
}

void SDLStub::startAudio(AudioCallback callback, void *param) {
	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));
	desired.freq = 22050;
	desired.format = AUDIO_S16;
	desired.channels = 1;
	desired.samples = 2048;
	desired.callback = callback;
	desired.userdata = param;
	if (SDL_OpenAudio(&desired, NULL) == 0) {
		SDL_PauseAudio(0);
	} else {
		error("SDLStub::startAudio() unable to open sound device");
	}
}

void SDLStub::stopAudio() {
	SDL_CloseAudio();
}

bool SDLStub::processEvents() {
	bool quit = false;
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
			quit = true;
			break;
		case SDL_KEYDOWN:
			if (ev.key.keysym.mod & KMOD_ALT) {
				if (ev.key.keysym.sym == SDLK_KP_PLUS) {
					uint8 s = _scaler + 1;
					if (s < ARRAYSIZE(_scalers)) {
						switchGfxMode(s);
					}
				} else if (ev.key.keysym.sym == SDLK_KP_MINUS) {
					int8 s = _scaler - 1;
					if (_scaler > 0) {
						switchGfxMode(s);
					}
				}
				break;
			}
			break;
		default:
			break;
		}
	}
	return quit;
}

void SDLStub::sleep(uint32 duration) {
	SDL_Delay(duration);
}

uint32 SDLStub::getTimeStamp() {
	return SDL_GetTicks();
}

void SDLStub::prepareGfxMode() {
	int w = _screenW * _scalers[_scaler].factor;
	int h = _screenH * _scalers[_scaler].factor;
	_screen = SDL_SetVideoMode(w, h, 16, SDL_SWSURFACE);
	if (!_screen) {
		error("SDLStub::prepareGfxMode() unable to allocate _screen buffer");
	}
	_sclscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 16,
						_screen->format->Rmask,
						_screen->format->Gmask,
						_screen->format->Bmask,
						_screen->format->Amask);
	if (!_sclscreen) {
		error("SDLStub::prepareGfxMode() unable to allocate _sclscreen buffer");
	}
}

void SDLStub::cleanupGfxMode() {
	if (_offscreen) {
		free(_offscreen);
		_offscreen = 0;
	}
	if (_sclscreen) {
		SDL_FreeSurface(_sclscreen);
		_sclscreen = 0;
	}
	if (_screen) {
		SDL_FreeSurface(_screen);
		_screen = 0;
	}
}

void SDLStub::switchGfxMode(uint8 scaler) {
	SDL_Surface *prev_sclscreen = _sclscreen;
	SDL_FreeSurface(_screen); 	
	_scaler = scaler;
	prepareGfxMode();
	SDL_BlitSurface(prev_sclscreen, NULL, _sclscreen, NULL);
	SDL_FreeSurface(prev_sclscreen);
}

void SDLStub::point1x(uint16 *dst, uint16 dstPitch, const uint16 *src, uint16 srcPitch, uint16 w, uint16 h) {
	dstPitch >>= 1;
	while (h--) {
		memcpy(dst, src, w * 2);
		dst += dstPitch;
		src += srcPitch;
	}
}

void SDLStub::point2x(uint16 *dst, uint16 dstPitch, const uint16 *src, uint16 srcPitch, uint16 w, uint16 h) {
	dstPitch >>= 1;
	while (h--) {
		uint16 *p = dst;
		for (int i = 0; i < w; ++i, p += 2) {
			uint16 c = *(src + i);
			*(p + 0) = c;
			*(p + 1) = c;
			*(p + 0 + dstPitch) = c;
			*(p + 1 + dstPitch) = c;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}

void SDLStub::point3x(uint16 *dst, uint16 dstPitch, const uint16 *src, uint16 srcPitch, uint16 w, uint16 h) {
	dstPitch >>= 1;
	while (h--) {
		uint16 *p = dst;
		for (int i = 0; i < w; ++i, p += 3) {
			uint16 c = *(src + i);
			*(p + 0) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + 0 + dstPitch) = c;
			*(p + 1 + dstPitch) = c;
			*(p + 2 + dstPitch) = c;
			*(p + 0 + dstPitch * 2) = c;
			*(p + 1 + dstPitch * 2) = c;
			*(p + 2 + dstPitch * 2) = c;
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}
