
#ifndef __SYSTEMSTUB_H__
#define __SYSTEMSTUB_H__

#include "intern.h"

struct SystemStub {	
	typedef void (*AudioCallback)(void *param, uint8 *stream, int len);

	virtual ~SystemStub() {}

	virtual void init(const char *title, uint16 w, uint16 h) = 0;
	virtual void destroy() = 0;

	virtual void setPalette(const uint8 *pal, uint16 n) = 0;
	virtual void copyRect(uint16 x, uint16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch) = 0;

	virtual void startAudio(AudioCallback callback, void *param) = 0;
	virtual void stopAudio() = 0;

	virtual bool processEvents() = 0;
	virtual void sleep(uint32 duration) = 0;
	virtual uint32 getTimeStamp() = 0;
};

extern SystemStub *SystemStub_SDL_create();

#endif
