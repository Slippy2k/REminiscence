
#include <SDL.h>
#include <SDL_mixer.h>
#include "file.h"
#include "mixer.h"
#include "util.h"

struct Mixer_impl {

	static const int kMixFreq = 22050;
	static const int kMixBufSize = 4096;
	static const int kMixChannels = 8;

	struct {
		Mix_Chunk *sound;
		const uint8_t *sample;
	} _channels[kMixChannels];
	Mix_Music *_music;

	void init() {
		memset(_channels, 0, sizeof(_channels));
		_music = 0;

		Mix_Init(MIX_INIT_OGG);
		if (Mix_OpenAudio(kMixFreq, AUDIO_S16SYS, 2, kMixBufSize) < 0) {
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
					freeSound(i);
					// TODO: notify sound finished playback
				}
			}
		}
	}

	int findPlayingChannel(const uint8_t *data) const {
		for (int i = 0; i < kMixChannels; ++i) {
			if (_channels[i].sample == data) {
				if (Mix_Playing(i)) {
					return i;
				}
				break;
			}
		}
		return -1;
	}

	static uint8_t *convertSampleRaw(const uint8_t *data, uint32_t len, int freq, uint32_t *size) {
		SDL_AudioCVT cvt;
		memset(&cvt, 0, sizeof(cvt));
		if (SDL_BuildAudioCVT(&cvt, AUDIO_U8, 1, freq, AUDIO_S16SYS, 2, kMixFreq) < 0) {
			warning("Failed to resample from %d Hz", freq);
			return 0;
		}
		if (cvt.needed) {
			cvt.len = len;
			cvt.buf = (uint8_t *)calloc(1, len * cvt.len_mult);
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

	void playSoundRaw(const uint8_t *data, uint32_t len, int freq, uint8_t volume) {
		const int channel = findPlayingChannel(data);
		if (channel != -1) {
//			Mix_FadeOutChannel(channel, 500);
//			_channels[channel].sample = 0;
			return;
		}
		uint32_t size;
		uint8_t *sample = convertSampleRaw(data, len, freq, &size);
		if (sample) {
			Mix_Chunk *chunk = Mix_QuickLoad_RAW(sample, size);
			playSound(volume, chunk, data);
		}
	}
	void playSoundWav(const uint8_t *data, uint8_t volume) {
		const int channel = findPlayingChannel(data);
		if (channel != -1) {
//			Mix_FadeOutChannel(channel, 500);
//			_channels[channel].sample = 0;
			return;
		}
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
				canQuickLoad = (format == 1 && channels == 2 && rate == kMixFreq && bits == 16);
			}
                }
		if (canQuickLoad) {
			Mix_Chunk *chunk = Mix_QuickLoad_WAV(const_cast<uint8_t *>(data));
			playSound(volume, chunk, data);
		} else {
			SDL_RWops *rw = SDL_RWFromConstMem(data, size);
			Mix_Chunk *chunk = Mix_LoadWAV_RW(rw, 1);
			playSound(volume, chunk, data);
		}
	}
	void playSound(int volume, Mix_Chunk *chunk, const uint8_t *data) {
		if (chunk) {
			int channel = Mix_PlayChannel(-1, chunk, 0);
			Mix_Volume(channel, volume * MIX_MAX_VOLUME / 255);
			if (channel < kMixChannels) {
				_channels[channel].sound = chunk;
				_channels[channel].sample = data;
			} else {
				warning("%d channels playing", channel + 1);
			}
		}
	}
	void freeSound(int channel) {
		Mix_Chunk *chunk = _channels[channel].sound;
		if (chunk) {
			if (!chunk->allocated) {
				free(chunk->abuf);
			}
			Mix_FreeChunk(chunk);
		}
		memset(&_channels[channel], 0, sizeof(_channels[channel]));
	}
	void stopSound(int channel) {
		Mix_HaltChannel(channel);
		freeSound(channel);
	}

	void playMusic(const char *path) {
		stopMusic();
		_music = Mix_LoadMUS(path);
		if (_music) {
			Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
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
			stopSound(i);
		}
		stopMusic();
	}
};

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

void Mixer::playSoundRaw(const uint8_t *data, uint32_t len, int freq, uint8_t volume) {
	debug(DBG_SND, "Mixer::playSoundRaw(%p, %d, %d)", data, freq, volume);
	if (_impl) {
		return _impl->playSoundRaw(data, len, freq, volume);
	}
}

void Mixer::playSoundWav(const uint8_t *data, uint8_t volume) {
	debug(DBG_SND, "Mixer::playSoundWav(%p, %d)", data, volume);
	if (_impl) {
		return _impl->playSoundWav(data, volume);
	}
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
