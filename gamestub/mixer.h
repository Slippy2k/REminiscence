
#ifndef MIXER_H__
#define MIXER_H__

#include "intern.h"

struct MixerChunk {
	uint8_t *data;
	uint32_t len;

	MixerChunk()
		: data(0), len(0) {
	}

	int8_t getPCM(int offset) const {
		if (offset < 0) {
			offset = 0;
		} else if (offset >= (int)len) {
			offset = len - 1;
		}
		return (int8_t)data[offset];
	}
};

struct MixerChannel {
	uint8_t active;
	uint8_t volume;
	MixerChunk chunk;
	uint32_t chunkPos;
	uint32_t chunkInc;
};

struct SystemStub;

struct Mixer {
	typedef bool (*PremixHook)(void *userData, int8_t *buf, int len);

	enum {
		NUM_CHANNELS = 4,
		FRAC_BITS = 12,
		MAX_VOLUME = 64
	};

	MixerChannel _channels[NUM_CHANNELS];
	PremixHook _premixHook;
	void *_premixHookData;
        void (*_lock)(int);
	int _rate;

        Mixer();

	void setFormat(int rate, int fmt);
	void free();
	void setPremixHook(PremixHook premixHook, void *userData);
	void play(const MixerChunk *mc, uint16_t freq, uint8_t volume);
	void stopAll();
	uint32_t getSampleRate() const;
	void mix(int8_t *buf, int len);

	static void addclamp(int8_t &a, int b);
	static void mixCallback(void *param, uint8_t *buf, int len);
};

template <class T>
int resampleLinear(T *sample, int pos, int step, int fracBits) {
	const int inputPos = pos >> fracBits;
	const int inputFrac = pos & ((1 << fracBits) - 1);
	int out = sample->getPCM(inputPos);
	out += (sample->getPCM(inputPos + 1) - out) * inputFrac >> fracBits;
	return out;
}

#endif // MIXER_H__
