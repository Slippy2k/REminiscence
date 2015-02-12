
#ifndef MIXER_H__
#define MIXER_H__

#include "intern.h"

struct Mixer_impl;

struct Mixer {
	Mixer_impl *_impl;

	Mixer();

	void init();
	void quit();
	void update();

	void playSoundRaw(const uint8_t *data, uint32_t len, int freq, uint8_t volume);
	void playSoundWav(const uint8_t *data, uint8_t volume);
	void playMusic(const char *path);
	void stopMusic();
	void stopAll();
};

#endif
