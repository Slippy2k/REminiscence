
#ifndef STUB_H__
#define STUB_H__

struct StubMixProc {
	void (*proc)(void *data, uint8_t *buf, int size);
	void *data;
};

struct Stub {
	void (*init)(const char *, const char *, int);
	void (*quit)();
	void (*save)();
	void (*queueKeyInput)(int keycode, int pressed);
	void (*queueTouchInput)(int num, int x, int y, int pressed);
	void (*doTick)();
	void (*initGL)(int, int);
	void (*drawGL)(int, int);
	StubMixProc (*getMixProc)(int rate, int fmt);
};

#endif

