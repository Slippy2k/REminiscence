
#ifndef AUDIO_MODS_PAULA_H
#define AUDIO_MODS_PAULA_H

#include <stdint.h>

enum {
        FRAC_BITS = 16,
        FRAC_LO_MASK = ((1L << FRAC_BITS) - 1),
        FRAC_HI_MASK = ((1L << FRAC_BITS) - 1) << FRAC_BITS,

        FRAC_ONE = (1L << FRAC_BITS),           // 1.0
        FRAC_HALF = (1L << (FRAC_BITS-1))       // 0.5
};

typedef int32_t frac_t;

inline frac_t doubleToFrac(double value) { return (frac_t)(value * FRAC_ONE); }
inline double fracToDouble(frac_t value) { return ((double)value) / FRAC_ONE; }

inline frac_t intToFrac(int16_t value) { return value * (1 << FRAC_BITS); }
inline int16_t fracToInt(frac_t value) { return value / (1 << FRAC_BITS); }

typedef uint32_t uint;
typedef uint8_t byte;

#undef MIN
template<typename T>
inline T MIN(T v1, T v2) {
	return (v1 < v2) ? v1 : v2;
}

/**
 * Emulation of the "Paula" Amiga music chip
 * The interrupt frequency specifies the number of mixed wavesamples between
 * calls of the interrupt method
 */
class Paula {
public:
	static const int NUM_VOICES = 4;
	enum {
		kPalSystemClock  = 7093790,
		kNtscSystemClock = 7159090,
		kPalCiaClock     = kPalSystemClock / 10,
		kNtscCiaClock    = kNtscSystemClock / 10,
		kPalPaulaClock   = kPalSystemClock / 2,
		kNtscPauleClock  = kNtscSystemClock / 2
	};

	struct Offset {
		uint	int_off;	// integral part of the offset
		frac_t	rem_off;	// fractional part of the offset, at least 0 and less than 1

		explicit Offset(int off = 0) : int_off(off), rem_off(0) {}
	};

	Paula(bool stereo = false, int rate = 44100, uint interruptFreq = 0);
	~Paula();

	bool playing() const { return _playing; }

	void clearVoice(byte voice);
	void clearVoices() { for (int i = 0; i < NUM_VOICES; ++i) clearVoice(i); }
	void startPlay() { _playing = true; }
	void stopPlay() { _playing = false; }
	void pausePlay(bool pause) { _playing = !pause; }

	int readBuffer(int16_t *buffer, const int numSamples);

protected:
	struct Channel {
		const int8_t *data;
		const int8_t *dataRepeat;
		uint32_t length;
		uint32_t lengthRepeat;
		int16_t period;
		byte volume;
		Offset offset;
		byte panning; // For stereo mixing: 0 = far left, 255 = far right
		int dmaCount;
	};

	bool _end;

	virtual void interrupt() = 0;

	void startPaula() {
		_playing = true;
		_end = false;
	}

	void stopPaula() {
		_playing = false;
		_end = true;
	}

	void setChannelPanning(byte channel, byte panning) {
		assert(channel < NUM_VOICES);
		_voice[channel].panning = panning;
	}

	void disableChannel(byte channel) {
		assert(channel < NUM_VOICES);
		_voice[channel].data = 0;
	}

	void enableChannel(byte channel) {
		assert(channel < NUM_VOICES);
		Channel &ch = _voice[channel];
		ch.data = ch.dataRepeat;
		ch.length = ch.lengthRepeat;
		// actually first 2 bytes are dropped?
		ch.offset = Offset(0);
		// ch.period = ch.periodRepeat;
	}

	void setChannelPeriod(byte channel, int16_t period) {
		assert(channel < NUM_VOICES);
		_voice[channel].period = period;
	}

	void setChannelVolume(byte channel, byte volume) {
		assert(channel < NUM_VOICES);
		_voice[channel].volume = volume;
	}

	void setChannelData(uint8_t channel, const int8_t *data, const int8_t *dataRepeat, uint32_t length, uint32_t lengthRepeat, int32_t offset = 0) {
		assert(channel < NUM_VOICES);

		Channel &ch = _voice[channel];

		ch.dataRepeat = data;
		ch.lengthRepeat = length;
		enableChannel(channel);
		ch.offset = Offset(offset);

		ch.dataRepeat = dataRepeat;
		ch.lengthRepeat = lengthRepeat;
	}

	void setChannelOffset(byte channel, Offset offset) {
		assert(channel < NUM_VOICES);
		_voice[channel].offset = offset;
	}

	Offset getChannelOffset(byte channel) {
		assert(channel < NUM_VOICES);
		return _voice[channel].offset;
	}

	int getRate() const {
		return _rate;
	}

private:
	Channel _voice[NUM_VOICES];

	const bool _stereo;
	const int _rate;
	const double _periodScale;
	uint _intFreq;
	uint _curInt;
	uint32_t _timerBase;
	bool _playing;

	template<bool stereo>
	int readBufferIntern(int16_t *buffer, const int numSamples);
};

#endif
