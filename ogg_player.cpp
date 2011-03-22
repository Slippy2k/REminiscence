/* REminiscence - Flashback interpreter
 * Copyright (C) 2005-2011 Gregory Montoir
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef USE_VORBIS
#include <vorbis/vorbisfile.h>
#endif
#include "file.h"
#include "mixer.h"
#include "ogg_player.h"

#ifdef USE_VORBIS
struct VorbisFile: File {
	uint32 offset;

	static size_t readHelper(void *ptr, size_t size, size_t nmemb, void *datasource) {
		VorbisFile *vf = (VorbisFile *)datasource;
		if (size != 0 && nmemb != 0) {
			const int n = vf->read(ptr, size * nmemb);
			if (n > 0) {
				vf->offset += n;
				return n / size;
			}
		}
		return 0;
	}
	static int seekHelper(void *datasource, ogg_int64_t offset, int whence) {
		VorbisFile *vf = (VorbisFile *)datasource;
		switch (whence) {
		case SEEK_SET:
			vf->offset = offset;
			break;
		case SEEK_CUR:
			vf->offset += offset;
			break;
		case SEEK_END:
			vf->offset = vf->size() + offset;
			break;
		}
		vf->seek(vf->offset);
		return 0;
	}
	static int closeHelper(void *datasource) {
		VorbisFile *vf = (VorbisFile *)datasource;
		vf->close();
		delete vf;
		return 0;
	}
	static long tellHelper(void *datasource) {
		VorbisFile *vf = (VorbisFile *)datasource;
		return vf->offset;
	}
};

struct OggDecoder_impl {
	OggDecoder_impl()
		: _loop(true), _readBuf(0), _readBufSize(0) {
	}
	~OggDecoder_impl() {
		free(_readBuf);
		_readBuf = 0;
	}

	bool load(VorbisFile *f, int mixerSampleRate) {
		ov_callbacks ovcb;
		ovcb.read_func  = VorbisFile::readHelper;
		ovcb.seek_func  = VorbisFile::seekHelper;
		ovcb.close_func = VorbisFile::closeHelper;
		ovcb.tell_func  = VorbisFile::tellHelper;
		if (ov_open_callbacks(f, &_ovf, 0, 0, ovcb) < 0) {
			warning("Invalid .ogg file");
			return false;
		}
		_vi = ov_info(&_ovf, -1);
		if ((_vi->channels != 1 && _vi->channels != 2) || _vi->rate != mixerSampleRate) {
			warning("Unhandled ogg/pcm format ch %d rate %d", _vi->channels, _vi->rate);
			return false;
		}
		return true;
	}
	int read(int8 *dst, int samples) {
		int size = samples * _vi->channels;
		if (size > _readBufSize) {
			_readBufSize = size;
			free(_readBuf);
			_readBuf = (int8 *)malloc(_readBufSize);
			if (!_readBuf) {
				return 0;
			}
		}
		int count = 0;
		while (size > 0) {
			const int len = ov_read(&_ovf, (char *)_readBuf, size, 0, 1, 1, 0);
			if (len < 0) {
				// error in decoder
				return 0;
			} else if (len == 0) {
				if (_loop) {
					ov_raw_seek(&_ovf, 0);
					continue;
				}
				break;
			}
			switch (_vi->channels) {
			case 2:
				assert((len & 1) == 0);
				for (int i = 0; i < len; i += 2) {
					Mixer::addclamp(*dst++, (_readBuf[i] + _readBuf[i + 1]) / 2);
				}
				break;
			case 1:
				for (int i = 0; i < len; ++i) {
					Mixer::addclamp(*dst++, _readBuf[i]);
				}
				break;
			}
			size -= len;
			count += len;
		}
		assert(size == 0);
		return count;
	}

	OggVorbis_File _ovf;
	bool _loop;
	int8 *_readBuf;
	int _readBufSize;
	vorbis_info *_vi;
};
#endif

OggPlayer::OggPlayer(Mixer *mixer, FileSystem *fs)
	: _mix(mixer), _fs(fs), _impl(0) {
}

OggPlayer::~OggPlayer() {
	delete _impl;
	_impl = 0;
}

bool OggPlayer::playTrack(int num) {
#ifdef USE_VORBIS
	stopTrack();
	char buf[16];
	snprintf(buf, sizeof(buf), "track%02d.ogg", num);
	VorbisFile *vf = new VorbisFile;
	if (vf->open(buf, "rb", _fs)) {
		vf->offset = 0;
		_impl = new OggDecoder_impl();
		if (_impl->load(vf, _mix->getSampleRate())) {
			debug(DBG_INFO, "Playing '%s'", buf);
			_mix->setPremixHook(mixCallback, this);
			return true;
		}
	}
	delete vf;
#endif
	return false;
}

void OggPlayer::stopTrack() {
#ifdef USE_VORBIS
	_mix->setPremixHook(0, 0);
	delete _impl;
	_impl = 0;
#endif
}

void OggPlayer::pauseTrack() {
#ifdef USE_VORBIS
	if (_impl) {
		_mix->setPremixHook(0, 0);
	}
#endif
}

void OggPlayer::resumeTrack() {
#ifdef USE_VORBIS
	if (_impl) {
		_mix->setPremixHook(mixCallback, this);
	}
#endif
}

bool OggPlayer::mix(int8 *buf, int len) {
#ifdef USE_VORBIS
	if (_impl) {
		return _impl->read(buf, len) != 0;
	}
#endif
	return false;
}

bool OggPlayer::mixCallback(void *param, int8 *buf, int len) {
	return ((OggPlayer *)param)->mix(buf, len);
}

