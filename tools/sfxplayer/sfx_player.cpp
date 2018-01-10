
#include <cmath>
#include "intern.h"
#include "SDL.h"


#define SAMPLE_RATE 22050
//#define PAULA_FREQ 3579545 // NTSC
#define PAULA_FREQ 3546897 // PAL

void low_pass(const int8_t *in, int len, int8_t *out);
void nr(const int8_t *in, int len, int8_t *out);
void sinc(double pos, const int8_t *in, int len, int fsr, int8_t *out);

static const bool kOutputToDisk = true;

struct SfxPlayer {
	enum {
		NUM_SAMPLES = 15,
		NUM_CHANNELS = 3,
		FRAC_BITS = 12
	};
	
	struct Module {
		const uint8_t *sampleData[NUM_SAMPLES];
		const uint8_t *moduleData;
	};
	
	struct SampleInfo {
		uint16_t len;
		uint16_t vol;
		uint16_t loopPos;
		uint16_t loopLen;
		int freq;
		int pos;
		const uint8_t *data;

		uint8_t getData(int offset) const {
			if (offset >= len) {
				offset = len - 1;
			}
			return data[offset];
		}
	};
	
	static const uint16_t _periodTable[];
	static const uint8_t _musicData68_69[];
	static const uint8_t _musicDataUnk[];
	static const uint8_t _musicData70_71[];
	static const uint8_t _musicData72[];
	static const uint8_t _musicData73[];
	static const uint8_t _musicData74[];
	static const uint8_t _musicData75[];
	static const uint8_t _musicDataSample1[];
	static const uint8_t _musicDataSample2[]; // tick
	static const uint8_t _musicDataSample3[]; // cloche
	static const uint8_t _musicDataSample4[];
	static const uint8_t _musicDataSample5[];
	static const uint8_t _musicDataSample6[];
	static const uint8_t _musicDataSample7[];
	static const uint8_t _musicDataSample8[];
	static const Module _module68_69;
	static const Module _moduleUnk;
	static const Module _module70_71;
	static const Module _module72;
	static const Module _module73;
	static const Module _module74;
	static const Module _module75;
	
	const Module *_mod;
	bool _playing;
	int _samplesLeft;
	uint16_t _curOrder;
	uint16_t _numOrders;
	uint16_t _orderDelay;
	const uint8_t *_modData;
	SampleInfo _samples[NUM_CHANNELS];
	
	void loadModule(const int num);
	void stop();
	void start();
	void playSample(int channel, const uint8_t *sampleData, uint16_t period);
	void handleTick();
	void mix(int8_t *buf, int len);
	void mixSamples(int8_t *buf, int samplesLen);
	
	static void mixCallback(void *param, uint8_t *buf, int len);
};

void SfxPlayer::loadModule(const int num) {
	switch (num) {
	case 68:
	case 69:
		_mod = &_module68_69;
		break;
	case 70:
	case 71:
		_mod = &_module70_71;
		break;
	case 72:
		_mod = &_module72;
		break;
	case 73:
		_mod = &_module73;
		break;
	case 74:
		_mod = &_module74;
		break;
	case 75:
		_mod = &_module75;
		break;
	case 99:
		_mod = &_moduleUnk;
		break;
	default:
		fprintf(stderr, "Unknown module number %d\n", num);
		break;	
	}
	_curOrder = 0;
	_numOrders = READ_BE_UINT16(_mod->moduleData);
	_orderDelay = 0;
	_modData = _mod->moduleData + 0x22; // 2 (_numOrders) + 2 (_orderDelay) + 15 * sizeof(uint16_t) (sample_period)
	for (int i = 0; i < 5; ++i) {
		const int16_t period = (int16_t)READ_BE_UINT16(_mod->moduleData + 4 + i * 2);
		printf("sample %d period %d\n", i, period);
		if (period >= 0 && period < 40) {
			printf("periodLut %d\n", _periodTable[period]);
		}
	}
}

void SfxPlayer::stop() {
	_playing = false;
}

void SfxPlayer::start() {
	memset(_samples, 0, sizeof(_samples));
	_samplesLeft = 0;
	if (!kOutputToDisk) {
		SDL_AudioSpec desired;
		memset(&desired, 0, sizeof(desired));
		desired.freq = SAMPLE_RATE;
		desired.format = AUDIO_S8;
		desired.channels = 1;
		desired.samples = 2048;
		desired.callback = mixCallback;
		desired.userdata = this;
		if (SDL_OpenAudio(&desired, NULL) == 0) {
			SDL_PauseAudio(0);
		} else {
			fprintf(stderr, "SfxPlayer::init() unable to open sound device\n");
		}
	}
	_playing = true;
}

void SfxPlayer::playSample(int channel, const uint8_t *sampleData, uint16_t period) {
	assert(channel < NUM_CHANNELS);
	SampleInfo *si = &_samples[channel];
	si->len = READ_BE_UINT16(sampleData); sampleData += 2;
	si->vol = READ_BE_UINT16(sampleData); sampleData += 2;
	si->loopPos = READ_BE_UINT16(sampleData); sampleData += 2;
	si->loopLen = READ_BE_UINT16(sampleData); sampleData += 2;
	si->freq = PAULA_FREQ / period;
	si->pos = 0;
	si->data = sampleData;
	assert(si->loopPos + si->loopLen <= si->len);
	fprintf(stdout, "playSample(channel=%d, freq=%d)\n", channel, si->freq);
}

void SfxPlayer::handleTick() {
	if (!_playing) {
		return;
	}
	if (_orderDelay != 0) {
		--_orderDelay;
		// check for end of song
		if (_orderDelay == 0 && _modData == 0) {
			_playing = false;
		}
	} else {
		_orderDelay = READ_BE_UINT16(_mod->moduleData + 2);
		fprintf(stdout, "handleTick() order=%d/%d\n", _curOrder, _numOrders);
		int16_t period = 0;
		for (int ch = 0; ch < 3; ++ch) {
			const uint8_t *sampleData = 0;
			uint8_t b = *_modData++;
			if (b != 0) {
				--b;
				assert(b < 5);
				period = READ_BE_UINT16(_mod->moduleData + 4 + b * 2);
				sampleData = _mod->sampleData[b];
			}
			b = *_modData++;
			if (b != 0) { // arpeggio
				int16_t per = period + (b - 1);
				if (per >= 0 && per < 40) {
					per = _periodTable[per];
				} else {
					fprintf(stdout, "Out of bounds per=%d period=%d b=%d\n", per, period, b);
					if (per == -3) {
						per = 0xA0;
					} else if (per == 40 || per == 74) {
						per = 0x71;
					} else {
						assert(0);
					}
				}
				playSample(ch, sampleData, per);
			}
		}
		++_curOrder;
		if (_curOrder >= _numOrders) {
			printf("end of song\n");
			_orderDelay += 20;
			_modData = 0;
		}
	}
}

static void addclamp(int8_t& a, int b) {
	int add = a + b;
	if (add < -128) {
		add = -128;
	} else if (add > 127) {
		add = 127;
	}
	a = add;
}

int resampleLinear(SfxPlayer::SampleInfo *si, int pos, int frac) {
	int8_t b0 = si->getData((pos >> frac));
	int8_t b1 = si->getData((pos >> frac) + 1);
	int a1 = pos & ((1 << frac) - 1);
	int a0 = (1 << frac) - a1;
	int b = (b0 * a0 + b1 * a1) >> frac;
	return b;
}

int resamplePoint(SfxPlayer::SampleInfo *si, int pos, int frac) {
	const int8_t b = si->getData(pos >> frac);
	return b;
}

int resampleSinc(SfxPlayer::SampleInfo *si, int pos, int frac) {
	const double x = (double)pos / (1 << frac);
	int8_t sample;
	sinc(x, (const int8_t *)si->data, si->len, si->freq, &sample);
	return sample;
}

static int (*resample)(SfxPlayer::SampleInfo *, int, int) = resamplePoint;

void SfxPlayer::mixSamples(int8_t *buf, int samplesLen) {
	memset(buf, 0, samplesLen);
	for (int i = 0; i < NUM_CHANNELS; ++i) {
		SampleInfo *si = &_samples[i];
		if (si->data) {
			int8_t *mixbuf = buf;
			int len = si->len << FRAC_BITS;
			int loopLen = si->loopLen << FRAC_BITS;
			int loopPos = si->loopPos << FRAC_BITS;
			int deltaPos = (si->freq << FRAC_BITS) / SAMPLE_RATE;
			int curLen = samplesLen;
			int pos = si->pos;
			while (curLen != 0) {
				int count;
				if (loopLen > (2 << FRAC_BITS)) {
					assert(si->loopPos + si->loopLen <= si->len);					
					if (pos >= loopPos + loopLen) {
						pos -= loopLen;
					}
					count = MIN(curLen, (loopPos + loopLen - pos - 1) / deltaPos + 1);
					curLen -= count;
				} else {
					if (pos >= len) {
						count = 0;
					} else {
						count = MIN(curLen, (len - pos - 1) / deltaPos + 1);
					}
					curLen = 0;
				}
				while (count--) {
					const int sample = resample(si, pos, FRAC_BITS);
					addclamp(*mixbuf++, sample * si->vol / 64);
					pos += deltaPos;
				}
			}
			si->pos = pos;
     		}
	}
}

void SfxPlayer::mix(int8_t *buf, int len) {
	if (!_playing) {
		return;
	}
	memset(buf, 0, len);
	const int samplesPerTick = SAMPLE_RATE / 50;
	while (len != 0) {
		if (_samplesLeft == 0) {
			handleTick();
			_samplesLeft = samplesPerTick;
		}
		int count = _samplesLeft;
		if (count > len) {
			count = len;
		}
		_samplesLeft -= count;
		len -= count;
		
		mixSamples(buf, count);
		buf += count;
	}
}

void SfxPlayer::mixCallback(void *param, uint8_t *buf, int len) {
	((SfxPlayer *)param)->mix((int8_t *)buf, len);
}

#undef main
int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_AUDIO);
	if (argc != 2) {
		printf("Syntax: %s mod\n",argv[0]);
	} else {
		SfxPlayer p;
		p.loadModule(atoi(argv[1]));
		p.start();
		if (kOutputToDisk) {
			FILE *fp = fopen("out.raw", "wb");
			if (fp) {
				while (p._playing) {
					static const int kBufSize = 2048;
					int8_t buf[3][kBufSize];
					p.mix(buf[0], kBufSize);
					nr(buf[0], kBufSize, buf[1]);
					low_pass(buf[1], kBufSize, buf[2]);
					fwrite(buf[2], 1, kBufSize, fp);
				}
				fclose(fp);
			}
		} else {
			bool quit = false;
			while (!quit && p._playing) {
				SDL_Event ev;
				while (SDL_PollEvent(&ev)) {
					switch (ev.type) {
					case SDL_KEYUP:
						quit = true;
						break;
					}
				}
				SDL_Delay(100);
			}
		}
		p.stop();
		SDL_Quit();
	}
	return 0;
}


const uint16_t SfxPlayer::_periodTable[] = {
	0x434, 0x3F8, 0x3C0, 0x38A, 0x358, 0x328, 0x2FA, 0x2D0, 0x2A6, 0x280,
	0x25C, 0x23A, 0x21A, 0x1FC, 0x1E0, 0x1C5, 0x1AC, 0x194, 0x17D, 0x168,
	0x153, 0x140, 0x12E, 0x11D, 0x10D, 0x0FE, 0x0F0, 0x0E2, 0x0D6, 0x0CA,
	0x0BE, 0x0B4, 0x0AA, 0x0A0, 0x097, 0x08F, 0x087, 0x07F, 0x078, 0x071
};

#include "musicdata.h"
const SfxPlayer::Module SfxPlayer::_module68_69 = {
	{
		_musicDataSample1, _musicDataSample8, _musicDataSample3, _musicDataSample4, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8
	},
	_musicData68_69
};
const SfxPlayer::Module SfxPlayer::_moduleUnk = {
	{
		_musicDataSample2, _musicDataSample3, _musicDataSample8, _musicDataSample8, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8,
	},
	_musicDataUnk
};
const SfxPlayer::Module SfxPlayer::_module70_71 = {
	{
		_musicDataSample1, _musicDataSample2, _musicDataSample3, _musicDataSample3, _musicDataSample8,
 		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8,
 		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8
 	},
	_musicData70_71
};
const SfxPlayer::Module SfxPlayer::_module72 = {
	{
		_musicDataSample1, _musicDataSample2, _musicDataSample5, _musicDataSample4, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8
 	},
	_musicData72
};
const SfxPlayer::Module SfxPlayer::_module73 = {
	{
		_musicDataSample1, _musicDataSample2, _musicDataSample4, _musicDataSample3, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8
 	},
	_musicData73
};
const SfxPlayer::Module SfxPlayer::_module74 = {
	{
		_musicDataSample1, _musicDataSample2, _musicDataSample5, _musicDataSample6, _musicDataSample7,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8
 	},
	_musicData74
};
const SfxPlayer::Module SfxPlayer::_module75 = {
	{
		_musicDataSample1, _musicDataSample2, _musicDataSample5, _musicDataSample6, _musicDataSample7,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8,
		_musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8, _musicDataSample8
 	},
	_musicData75
};
