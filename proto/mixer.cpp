
#include "file.h"
#include "mixer.h"
#include "util.h"

#ifdef USE_SDL_MIXER
#include <SDL.h>
#include <SDL_mixer.h>

struct Mixer_impl {

	static const int kMixFreq = 22050;
	static const int kMixBufSize = 4096;
	static const int kMixChannels = 8;

	struct {
		int id;
		Mix_Chunk *sound;
		uint8_t *sample;
	} _channels[kMixChannels];
	Mix_Music *_music;
	int _channelId;

	void init() {
		memset(_channels, 0, sizeof(_channels));
		_music = 0;
		_channelId = 1;

		Mix_Init(MIX_INIT_OGG);
		if (Mix_OpenAudio(kMixFreq, AUDIO_S16SYS, 1, kMixBufSize) < 0) {
			warning("Mix_OpenAudio failed: %s", Mix_GetError());
		}
		Mix_AllocateChannels(kMixChannels);
	}
	void quit() {
		stopAll();
		Mix_CloseAudio();
		Mix_Quit();
	}
	void update() {
		for (int i = 0; i < kMixChannels; ++i) {
			if (!Mix_Playing(i)) {
				if (_channels[i].sound) {
					freeChannel(i);
				}
			}
		}
	}

	bool isPlayingSound(int id) const {
		for (int i = 0; i < kMixChannels; ++i) {
			if (_channels[i].id == id) {
				return Mix_Playing(i);
			}
		}
		return false;
	}

	void stopSound(int id) {
		for (int i = 0; i < kMixChannels; ++i) {
			if (_channels[i].id == id) {
				Mix_HaltChannel(i);
				freeChannel(i);
			}
		}
	}

	static uint8_t *convertSampleRaw(const uint8_t *data, uint32_t len, int freq, uint32_t *size) {
		SDL_AudioCVT cvt;
		if (SDL_BuildAudioCVT(&cvt, AUDIO_U8, 1, freq, AUDIO_S16SYS, 1, kMixFreq) < 0) {
			warning("Failed to resample from %d Hz", freq);
			return 0;
		}
		if (cvt.needed) {
			cvt.len = len;
			cvt.buf = (uint8_t *)malloc(len * cvt.len_mult);
			if (cvt.buf) {
				memcpy(cvt.buf, data, len);
				SDL_ConvertAudio(&cvt);
			}
			*size = cvt.len_cvt;
			return cvt.buf;
		} else {
			uint8_t *buf = (uint8_t *)malloc(len);
			if (buf) {
				memcpy(buf, data, len);
			}
			*size = len;
			return buf;
		}
	}

	int playSoundRaw(const uint8_t *data, uint32_t len, int freq, uint8_t volume) {
		int id = -1;
		uint32_t size;
		uint8_t *sample = convertSampleRaw(data, len, freq, &size);
		if (sample) {
			Mix_Chunk *chunk = Mix_QuickLoad_RAW(sample, size);
			id = playSound(volume, chunk, sample);
		}
		return id;
	}
	int playSoundWav(const uint8_t *data, uint8_t volume) {
		int id = -1;
		uint32_t size = READ_LE_UINT32(data + 4) + 8;
		// check format for QuickLoad
		bool canQuickLoad = (AUDIO_S16SYS == AUDIO_S16LSB);
		if (canQuickLoad) {
			if (memcmp(data + 8, "WAVEfmt ", 8) == 0 && READ_LE_UINT32(data + 16) == 16) {
				const uint8_t *fmt = data + 20;
				const int format = READ_LE_UINT16(fmt);
				const int channels = READ_LE_UINT16(fmt + 2);
				const int rate = READ_LE_UINT32(fmt + 4);
				const int bits = READ_LE_UINT16(fmt + 14);
				debug(DBG_SND, "wave format %d channels %d rate %d bits %d", format, channels, rate, bits);
				canQuickLoad = (format == 1 && channels == 1 && rate == kMixFreq && bits == 16);
			}
                }
		uint8_t *sample = (uint8_t *)malloc(size);
		if (sample) {
			memcpy(sample, data, size);
			Mix_Chunk *chunk;
			if (canQuickLoad) {
				chunk = Mix_QuickLoad_WAV(sample);
			} else {
				SDL_RWops *rw = SDL_RWFromConstMem(sample, size);
				chunk = Mix_LoadWAV_RW(rw, 1);
			}
			id = playSound(volume, chunk, sample);
		}
		return id;
	}
	int playSound(int volume, Mix_Chunk *chunk, uint8_t *sample) {
		int id = -1;
		if (chunk) {
			int channel = Mix_PlayChannel(-1, chunk, 0);
			if (_channels[channel].sound) {
				// free previous sample
				if (Mix_GetChunk(channel) == _channels[channel].sound) {
					freeChannel(channel);
				}
			}
			Mix_Volume(channel, getSoundVolume(volume));
			if (channel < kMixChannels) {
				_channels[channel].id = id = generateNextChannelId();
				_channels[channel].sound = chunk;
				_channels[channel].sample = sample;
			} else {
				warning("%d channels playing", channel + 1);
			}
		}
		return id;
	}

	int generateNextChannelId() {
		const int id = _channelId++;
		if (_channelId > 999) {
			_channelId = 1;
		}
		return id;
	}

	void freeChannel(int channel) {
		Mix_Chunk *chunk = _channels[channel].sound;
		if (chunk) {
			if (!chunk->allocated) {
				assert(chunk->abuf == _channels[channel].sample);
				free(_channels[channel].sample);
			}
			Mix_FreeChunk(chunk);
		}
		memset(&_channels[channel], 0, sizeof(_channels[channel]));
	}

	void playMusic(const char *path) {
		stopMusic();
		_music = Mix_LoadMUS(path);
		if (_music) {
			Mix_VolumeMusic(getMusicVolume());
			Mix_PlayMusic(_music, 0);
		}
	}
	void stopMusic() {
		Mix_HaltMusic();
		Mix_FreeMusic(_music);
		_music = 0;
	}

	void stopAll() {
		for (int i = 0; i < kMixChannels; ++i) {
			Mix_HaltChannel(i);
			freeChannel(i);
		}
		stopMusic();
	}

	static int getMusicVolume() {
		return MIX_MAX_VOLUME / 2;
	}
	static int getSoundVolume(int volume) {
		return MIX_MAX_VOLUME * 3 / 4 * volume / 255;
	}
};
#endif

#ifdef USE_OPENSL_ES
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

struct Mixer_impl {

	static const int kMixFreq = 22050;
	static const int kMixBufSize = 4096;
	static const int kMixChannels = 8;

	SLObjectItf _engineObject;
	SLEngineItf _engineEngine;
	SLObjectItf _outputMixObject;
	SLObjectItf _bqPlayerObject;
	SLPlayItf _bqPlayerPlay;
	SLAndroidSimpleBufferQueueItf _bqPlayerBufferQueue;
	int16_t _mixBuf[kMixBufSize];
	struct {
		const uint8_t *data;
		int len;
		float offset, step;
	} _channels[kMixChannels];

	void init() {
		SLresult ret;

		// engine
		ret = slCreateEngine(&_engineObject, 0, 0, 0, 0, 0);
		if (ret != SL_RESULT_SUCCESS) {
			warning("slCreateEngine() failed, ret=0x%x", ret);
			return;
		}
		ret = (*_engineObject)->Realize(_engineObject, SL_BOOLEAN_FALSE);
		if (ret != SL_RESULT_SUCCESS) {
			warning("SLObjectItf::Realize() failed, ret=0x%x", ret);
			return;
		}
		ret = (*_engineObject)->GetInterface(_engineObject, SL_IID_ENGINE, &_engineEngine);
		if (ret != SL_RESULT_SUCCESS) {
			warning("SLObjectItf::GetInterface(SL_IID_ENGINE) failed, ret=0x%x", ret);
			return;
		}

		// buffer queue player
		SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
		SLDataFormat_PCM format_pcm;
		format_pcm.formatType = SL_DATAFORMAT_PCM;
		format_pcm.numChannels = 1;
		assert(kMixFreq == 22050);
		format_pcm.samplesPerSec = SL_SAMPLINGRATE_22_05;
		format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
		format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
		format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER,
		format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

		SLDataSource src = { &loc_bufq, &format_pcm };
		SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, _outputMixObject };
		SLDataSink snk = {&loc_outmix, NULL};
		const SLInterfaceID ids[2] = { SL_IID_BUFFERQUEUE, SL_IID_VOLUME };
		const SLboolean req[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
		ret = (*_engineEngine)->CreateAudioPlayer(_engineEngine, &_bqPlayerObject, &src, &snk, 2, ids, req);
		if (ret != SL_RESULT_SUCCESS) {
			warning("SLEngineItf::CreateAudioPlayer() failed, ret=0x%x", ret);
			return;
		}
		ret = (*_bqPlayerObject)->Realize(_bqPlayerObject, SL_BOOLEAN_FALSE);
		if (ret != SL_RESULT_SUCCESS) {
			warning("SLObjectItf::Realize() failed, ret=0x%x", ret);
			return;
		}
		ret = (*_bqPlayerObject)->GetInterface(_bqPlayerObject, SL_IID_PLAY, &_bqPlayerPlay);
		if (ret != SL_RESULT_SUCCESS) {
			warning("SLObjectItf::GetInterface(SL_IID_PLAY) failed, ret=0x%x", ret);
			return;
		}
		ret = (*_bqPlayerObject)->GetInterface(_bqPlayerObject, SL_IID_BUFFERQUEUE, &_bqPlayerBufferQueue);
		if (ret != SL_RESULT_SUCCESS) {
			warning("SLObjectItf::GetInterface(SL_IID_BUFFERQUEUE) failed, ret=0x%x", ret);
			return;
		}
		ret = (*_bqPlayerBufferQueue)->RegisterCallback(_bqPlayerBufferQueue, bqPlayerCallback, this);
		if (ret != SL_RESULT_SUCCESS) {
			warning("SLAndroidSimpleBufferQueueItf::RegisterCallback() failed, ret=0x%x", ret);
			return;
		}

		memset(_mixBuf, 0, sizeof(_mixBuf));
		memset(_channels, 0, sizeof(_channels));
	}
	void quit() {
		if (_bqPlayerObject) {
			(*_bqPlayerObject)->Destroy(_bqPlayerObject);
			_bqPlayerObject = 0;
			_bqPlayerPlay = 0;
			_bqPlayerBufferQueue = 0;
		}
		if (_outputMixObject) {
			(*_outputMixObject)->Destroy(_outputMixObject);
			_outputMixObject = 0;
		}
		if (_engineObject) {
			(*_engineObject)->Destroy(_engineObject);
			_engineObject = 0;
			_engineEngine = 0;
		}
	}
	void update() {
	}
	int findChannel() const {
		for (int i = 0; i < kMixChannels; ++i) {
			if (!_channels[i].data) {
				return i;
			}
		}
		return -1;
	}
	int playSoundRaw(const uint8_t *data, uint32_t len, int freq, uint8_t volume) {
		const int i = findChannel();
		if (i >= 0) {
			_channels[i].data = data;
			_channels[i].len = len;
			_channels[i].offset = 0.f;
			_channels[i].step = freq / (float)kMixFreq;
		}
		return i;
	}
	int playSoundWav(const uint8_t *data, uint8_t volume) {
		const int i = findChannel();
		if (i >= 0) {
			const uint32_t size = READ_LE_UINT32(data + 4) + 8;
			if (memcmp(data + 8, "WAVEfmt ", 8) == 0 && READ_LE_UINT32(data + 16) == 16) {
				const uint8_t *fmt = data + 20;
//				const int format = READ_LE_UINT16(fmt);
//				const int channels = READ_LE_UINT16(fmt + 2);
				const int rate = READ_LE_UINT32(fmt + 4);
//				const int bits = READ_LE_UINT16(fmt + 14);
				_channels[i].data = data + 44;
				_channels[i].len = READ_LE_UINT32(data + 40);
				_channels[i].offset = 0.f;
				_channels[i].step = rate / (float)kMixFreq;
			}
		}
		return i;

	}
	void stopSound(int id) {
		_channels[id].data = 0;
	}
	bool isPlayingSound(int id) {
		return _channels[id].data != 0;
	}
	void playMusic(const char *path) {
	}
	void stopMusic() {
	}
	void stopAll() {
	}
	static int16_t clipS16(int sample) {
		if (sample > 32767) {
			return 32767;
		} else if (sample < -32768) {
			return -32768;
		}
		return sample;
	}
	void mix() {
		memset(_mixBuf, 0, sizeof(_mixBuf));
		for (int i = 0; i < kMixBufSize; ++i) {
			for (int ch = 0; ch < kMixChannels; ++ch) {
				if (_channels[ch].data) {
					const int pos = (int)_channels[ch].offset;
					if (pos >= _channels[ch].len) {
						_channels[ch].data = 0;
						break;
					}
					_channels[ch].offset += _channels[ch].step;
					const int16_t sample = _mixBuf[i] + ((_channels[ch].data[pos] ^ 0x80) << 8); // U8 to S16
					_mixBuf[i] = clipS16(sample);
				}
			}
		}
		SLresult ret = (*_bqPlayerBufferQueue)->Enqueue(_bqPlayerBufferQueue, _mixBuf, sizeof(_mixBuf));
		if (ret != SL_RESULT_SUCCESS) {
			return;
		}
	}
	static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
		((Mixer_impl *)context)->mix();
	}
};
#endif

Mixer::Mixer() {
}

void Mixer::init() {
	_impl = new Mixer_impl();
	_impl->init();
}

void Mixer::quit() {
	stopAll();
	if (_impl) {
		_impl->quit();
		delete _impl;
	}
}

void Mixer::update() {
	if (_impl) {
		_impl->update();
	}
}

int Mixer::playSoundRaw(const uint8_t *data, uint32_t len, int freq, uint8_t volume) {
	debug(DBG_SND, "Mixer::playSoundRaw(%p, %d, %d)", data, freq, volume);
	if (_impl) {
		return _impl->playSoundRaw(data, len, freq, volume);
	}
	return -1;
}

int Mixer::playSoundWav(const uint8_t *data, uint8_t volume) {
	debug(DBG_SND, "Mixer::playSoundWav(%p, %d)", data, volume);
	if (_impl) {
		return _impl->playSoundWav(data, volume);
	}
	return -1;
}

void Mixer::stopSound(int id) {
	debug(DBG_SND, "Mixer::stopSound(%d)", id);
	if (_impl) {
		return _impl->stopSound(id);
	}
}

bool Mixer::isPlayingSound(int id) {
	debug(DBG_SND, "Mixer::isPlayingSound(%d)", id);
	if (_impl) {
		return _impl->isPlayingSound(id);
	}
	return false;
}

void Mixer::playMusic(const char *path) {
	debug(DBG_SND, "Mixer::playMusic(%s)", path);
	if (_impl) {
		return _impl->playMusic(path);
	}
}

void Mixer::stopMusic() {
	debug(DBG_SND, "Mixer::stopMusic()");
	if (_impl) {
		return _impl->stopMusic();
	}
}

void Mixer::stopAll() {
	debug(DBG_SND, "Mixer::stopAll()");
	if (_impl) {
		return _impl->stopAll();
	}
}
