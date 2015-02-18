
#ifndef STUB_H__
#define STUB_H__

struct Mixer;

enum {
	kKeyCodeLeft = 1,
	kKeyCodeRight,
	kKeyCodeUp,
	kKeyCodeDown,
	kKeyCodeShift,
	kKeyCodeSpace,
	kKeyCodeReturn,
	kKeyCodeBackspace,
	kKeyCodeQuit,
};

struct GameStub {
	virtual bool init(const char *, const char *, int) = 0;
	virtual void quit() = 0;
	virtual void save() = 0;
	virtual void queueKeyInput(int keycode, int pressed) = 0;
	virtual void queueTouchInput(int num, int x, int y, int pressed) = 0;
	virtual void doTick() = 0;
	virtual void initGL(int, int) = 0;
	virtual void drawGL(int, int) = 0;
	virtual void setMixerImpl(Mixer *) = 0;
};

extern "C" {
	GameStub *GameStub_create();
}

#endif

