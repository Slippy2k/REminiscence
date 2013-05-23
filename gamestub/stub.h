
#ifndef STUB_H__
#define STUB_H__

struct StubMixProc {
	void (*proc)(void *data, uint8_t *buf, int size);
	void *data;
};

struct StubBackBuffer {
	int w, h;
	uint8_t *ptr; // w*h
	uint8_t *pal; // 256*3
};

enum {
	kKeyLeft,
	kKeyRight,
	kKeyUp,
	kKeyDown,
	kKeyBackspace,
	kKeyShift,
	kKeyEscape,
	kKeySpace,
	kKeyEnter,
};

struct GameStub {
	virtual int init(int argc, char *argv[], char *errBuf) = 0;
	virtual void quit() = 0;
	virtual int getFrameTimeMs() = 0;
	virtual StubBackBuffer getBackBuffer() = 0;
	virtual StubMixProc getMixProc(int rate, int fmt, void (*lock)(int)) = 0;
	virtual void eventKey(int keycode, int pressed) = 0;
	virtual int doTick() = 0;
};

extern "C" {
	GameStub *GameStub_create();
}

#endif // STUB_H__
