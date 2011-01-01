
#ifndef STUB_H__
#define STUB_H__

struct Stub {
	void (*init)(const char *, int);
	void (*quit)();
	void (*queueKeyInput)(int keycode, int pressed);
	void (*queueTouchInput)(int num, int x, int y, int released);
	void (*doTick)();
	void (*initGL)(int, int);
	void (*drawGL)(int, int);
};

#endif

