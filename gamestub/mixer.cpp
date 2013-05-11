
#include "mixer.h"

struct MixerLock {
	void (*_lock)(int);
	MixerLock(void (*lock)(int))
		: _lock(lock) {
		_lock(1);
	}
	~MixerLock() {
		_lock(0);
	}
	static void noLock(int) {
	}
};

Mixer::Mixer() {
	memset(_channels, 0, sizeof(_channels));
	_premixHook = 0;
	_lock = MixerLock::noLock;
}

void Mixer::setFormat(int rate, int fmt) {
	_rate = rate;
}

void Mixer::free() {
	stopAll();
}

void Mixer::setPremixHook(PremixHook premixHook, void *userData) {
	debug(DBG_SND, "Mixer::setPremixHook()");
	MixerLock l(_lock);
	_premixHook = premixHook;
	_premixHookData = userData;
}

void Mixer::play(const MixerChunk *mc, uint16_t freq, uint8_t volume) {
	debug(DBG_SND, "Mixer::play(%d, %d)", freq, volume);
	MixerLock l(_lock);
	MixerChannel *ch = 0;
	for (int i = 0; i < NUM_CHANNELS; ++i) {
		MixerChannel *cur = &_channels[i];
		if (cur->active) {
			if (cur->chunk.data == mc->data) {
				cur->chunkPos = 0;
				return;
			}
		} else {
			ch = cur;
			break;
		}
	}
	if (ch) {
		ch->active = true;
		ch->volume = volume;
		ch->chunk = *mc;
		ch->chunkPos = 0;
		ch->chunkInc = (freq << FRAC_BITS) / getSampleRate();
	}
}

uint32_t Mixer::getSampleRate() const {
	return _rate;
}

void Mixer::stopAll() {
	debug(DBG_SND, "Mixer::stopAll()");
	MixerLock l(_lock);
	for (uint8_t i = 0; i < NUM_CHANNELS; ++i) {
		_channels[i].active = false;
	}
}

void Mixer::mix(int8_t *buf, int len) {
	memset(buf, 0, len);
	if (_premixHook) {
		if (!_premixHook(_premixHookData, buf, len)) {
			_premixHook = 0;
			_premixHookData = 0;
		}
	}
	for (uint8_t i = 0; i < NUM_CHANNELS; ++i) {
		MixerChannel *ch = &_channels[i];
		if (ch->active) {
			for (int pos = 0; pos < len; ++pos) {
				if ((ch->chunkPos >> FRAC_BITS) >= (ch->chunk.len - 1)) {
					ch->active = false;
					break;
				}
				int out = resampleLinear(&ch->chunk, ch->chunkPos, ch->chunkInc, FRAC_BITS);
				addclamp(buf[pos], out * ch->volume / Mixer::MAX_VOLUME);
				ch->chunkPos += ch->chunkInc;
			}
		}
	}
}

void Mixer::addclamp(int8_t& a, int b) {
	int add = a + b;
	if (add < -128) {
		add = -128;
	} else if (add > 127) {
		add = 127;
	}
	a = add;
}

void Mixer::mixCallback(void *param, uint8_t *buf, int len) {
	((Mixer *)param)->mix((int8_t *)buf, len);
	// S8 to U8
	for (int i = 0; i < len; ++i) {
		buf[i] ^= 0x80;
	}
}
