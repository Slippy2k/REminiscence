
#include "seq.h"
#include "systemstub.h"


const char *Seq::_seqPlayerTitle = "Tiertex Video Sequence Player";


Seq::Seq(SystemStub *stub, const char *fileName) {
	memset(this, 0, sizeof(Seq));
	_stub = stub;
	_ply_fileName = fileName;
	_ply_processBuffer = (uint8 *)malloc(0x8306);
	_ply_tempBuffer = (uint8 *)malloc(0x30000);
}

Seq::~Seq() {
	free(_ply_processBuffer);
	free(_ply_tempBuffer);
}

void Seq::start() {
	debug(DBG_SEQ, "Seq::start()");
	ply_openSeqFile();
	char title[512];
	sprintf(title, "%s - %s", _seqPlayerTitle, _ply_fileName);
	_stub->init(title, VIDEO_W, VIDEO_H);
	_ply_lastTimeStamp = _stub->getTimeStamp();
	vid_clearPal();
	ply_mainLoop();
	ply_closeSeqFile();
	_stub->destroy();
}

static void audioCallback(void *param, uint8 *stream, int len) {
	Seq *seq = (Seq *)param;
	seq->snd_audioCallback(stream, len);
}

void Seq::ply_mainLoop() {
	debug(DBG_SEQ, "Seq::ply_mainLoop()");
	ply_readFromSeqFile(_ply_readBuffer, 0x1800);
	ply_initOffsets(_ply_readBuffer + 0x100);
	vid_clearTempFrame();
	_ply_sndFrameNum = 0;
	_ply_quitFlag = false;
	_stub->startAudio(audioCallback, this);
	while (!_ply_quitFlag) {
		_ply_quitFlag = _stub->processEvents();
		ply_readProcessData();
		if (_ply_blockNum != 0xFF) {
			ply_initProcessData();
			ply_copyProcessData();
			ply_decodeBlit();
		}
	}
	_stub->stopAudio();
}

void Seq::ply_openSeqFile() {
	debug(DBG_SEQ, "Seq::ply_openSeqFile()");
	_ply_fileHandle = fopen(_ply_fileName, "rb");
	_ply_fileHandleSnd = fopen(_ply_fileName, "rb");
	if (!_ply_fileHandle || !_ply_fileHandleSnd)
		error("Can't open video sequence file '%s'", _ply_fileName);
}

void Seq::ply_closeSeqFile() {
	debug(DBG_SEQ, "Seq::ply_closeSeqFile()");
	if (_ply_fileHandle) {
		fclose(_ply_fileHandle);
		_ply_fileHandle = 0;
	}
	if (_ply_fileHandleSnd) {
		fclose(_ply_fileHandleSnd);
		_ply_fileHandleSnd = 0;
	}
}

void Seq::ply_readFromSeqFile(uint8 *buf, size_t size) {
	debug(DBG_SEQ, "Seq::ply_readFromSeqFile() curPos = %d", ftell(_ply_fileHandle));
	size_t sz = fread(buf, 1, size, _ply_fileHandle);
	if (feof(_ply_fileHandle)) {
		_ply_quitFlag = true;
	} else if (sz != size) {
		error("i/o error when reading file");
	}
}

void Seq::ply_initOffsets(const uint8 *dataPtr) {
	debug(DBG_SEQ, "Seq::ply_initOffsets()");
	uint8 *p = _ply_tempBuffer;
	int i;
	for (i = 0; i < 100; ++i) {
		_ply_ptrsTable[i] = p;
		_ply_offTable[i] = 0;
	}
	p += 0x480;
	for (i = 0; i < 30; ++i) {
		_ply_ptrsTable[i] = p;
		uint16 offset = READ_LE_UINT16(dataPtr); dataPtr += 2;
		if (offset == 0) {
			break;
		} else {
			p += (offset + 16) & ~15;
		}
	}
	_ply_ptrsTable[i] = p;
}

void Seq::ply_copyVideoBlock(const uint8 *bufData, uint8 bufNum, uint16 bufSize) {
	debug(DBG_SEQ, "Seq::ply_copyVideoBlock() bufNum = %d bufSize = 0x%X", bufNum, bufSize);
	uint16 offStart = _ply_offTable[bufNum];
	uint16 offEnd = _ply_offTable[bufNum] + bufSize;

	assert(bufNum < 100);
	assert(bufSize < 0x1800);
	assert(bufData - _ply_vidStartOffset < 0x1800);
	assert(_ply_ptrsTable[bufNum] + offEnd < _ply_ptrsTable[bufNum + 1]);
	assert(_ply_ptrsTable[bufNum] + offEnd < _ply_tempBuffer + 0x30000);

	memcpy(_ply_ptrsTable[bufNum] + offStart, bufData, bufSize);
	_ply_offTable[bufNum] = offEnd;
}

void Seq::ply_readProcessData() {
	debug(DBG_SEQ, "Seq::ply_readProcessData()");
	ply_readFromSeqFile(_ply_readBuffer, 0x1800);
	if (_ply_quitFlag) {
		_ply_blockNum = 0xFF;
		return;
	}
	_ply_vidStartOffset = _ply_readBuffer;
	const uint8 *p = _ply_vidStartOffset;

	uint16 palOffset = READ_LE_UINT16(p + 2);
	if (palOffset != 0) {
		vid_setPalBuf1(p + palOffset);
	}
	uint16 off1, off2;

	off1 = READ_LE_UINT16(p + 0x8);
	if (off1 != 0) {
		off2 = READ_LE_UINT16(p + 0xA);
		if (off2 == 0) {
			off2 = READ_LE_UINT16(p + 0xC);
			if (off2 == 0) {
				off2 = READ_LE_UINT16(p + 0xE);
			}
		}
		ply_copyVideoBlock(p + off1, p[5], off2 - off1);
	}

	off1 = READ_LE_UINT16(p + 0xA);
	if (off1 != 0) {
		off2 = READ_LE_UINT16(p + 0xC);
		if (off2 == 0) {
			off2 = READ_LE_UINT16(p + 0xE);
		}
		ply_copyVideoBlock(p + off1, p[6], off2 - off1);
	}

	off1 = READ_LE_UINT16(p + 0xC);
	if (off1 != 0) {
		off2 = READ_LE_UINT16(p + 0xE);
		ply_copyVideoBlock(p + off1, p[7], off2 - off1);
	}

	_ply_blockNum = p[4];
}

void Seq::ply_initProcessData() {
	debug(DBG_SEQ, "Seq::ply_initProcessData()");
	uint16 len = _ply_offTable[_ply_blockNum];
	memcpy(_ply_processBuffer, _ply_ptrsTable[_ply_blockNum], len);
	_ply_offTable[_ply_blockNum] = 0;
}

void Seq::ply_copyProcessData() {
	const uint8 *src = _ply_processBuffer;
	uint8 *dst = _ply_processOpBuffer;
	uint8 n = 64;
	while (n--) {
		uint16 code = READ_LE_UINT16(src); src += 2;
		for (int i = 0; i < 8; ++i) {
			*dst++ = code & 3;
			code >>= 2;
		}
	}
	_ply_curProcessBuffer = src;
	ply_processFrame();
}

void Seq::ply_decodeBlit() {
	debug(DBG_SEQ, "Seq::ply_decodeBlit()");
	vid_setPalSync(_vid_pal1);
	_stub->copyRect(0, 0, VIDEO_W, VIDEO_H, _vid_tempFrame, 256);
	int32 delay = SYNC_DELAY - (_stub->getTimeStamp() - _ply_lastTimeStamp);
	if (delay > 0) {
		_stub->sleep(delay);
	}
	_ply_lastTimeStamp = _stub->getTimeStamp();
}

void Seq::ply_processFrame() {
	debug(DBG_SEQ, "Seq::ply_processFrame()");
	const uint8 *src = _ply_curProcessBuffer;
	const uint8 *opBuf = _ply_processOpBuffer;
	for (int j = 0; j < 128; j += 8) {
		for (int i = 0; i < 256; i += 8) {
			uint8 *dst = _vid_tempFrame + j * 256 + i;
			uint8 op = *opBuf++;
			assert(op < 4);
			switch (op) {
			case 0:
				break;
			case 1:
				// unpack/copy a 8x8 block
				ply_processFrameOp2(src, dst);
				break;
			case 2:
				// copy some dirty 8x8 blocks
				ply_processFrameOp3(src, dst);
				break;
			case 3:
				// copy some dirty pixels
				ply_processFrameOp4(src, dst);
				break;
			}
		}
	}
}

void Seq::ply_processFrameOp2(const uint8 *&src, uint8 *dst) {
	debug(DBG_SEQ, "Seq::ply_processFrameOp2()");
	uint8 len = *src++;
	if (len & 0x80) {
		int n = 8;
		uint8 *data;
		switch (len & 0xF) {
		case 1:
			ply_unpackFrame(src);
			data = _ply_unpackedFrameBuffer;
			while (n--) {
				memcpy(dst, data, 8);
				data += 8;
				dst += 0x100;
			}
			break;
		case 2:
			ply_unpackFrame(src);
			data = _ply_unpackedFrameBuffer;
			while (n--) {
				for (int i = 0; i < 8; ++i) {
					dst[i * 0x100] = *data++;
				}
				++dst;
			}
			break;
		}
	} else {
		const uint8 *data = src;
		src += len;
		if (len > 8) {
			int n = 8;
			while (n--) {
				for (int j = 0; j < 2; ++j) {
					uint16 code = READ_LE_UINT16(src); src += 2;
					for (int i = 0; i < 4; ++i) {
						*dst++ = data[code & 0xF];
						code >>= 4;
					}
				}
				dst += 248;
			}
		} else if (len > 4) {
			int n = 8;
			while (n--) {
				uint16 code = READ_LE_UINT16(src); src += 2;
				for (int i = 0; i < 5; ++i) {
					*dst++ = data[code & 7];
					code >>= 3;
				}
				code |= (*src++) << 1;
				for (int i = 0; i < 3; ++i) {
					*dst++ = data[code & 7];
					code >>= 3;
				}
				dst += 248;
			}
		} else if (len > 2) {
			int n = 8;
			while (n--) {
				uint16 code = READ_LE_UINT16(src); src += 2;
				for (int i = 0; i < 8; ++i) {
					*dst++ = data[code & 3];
					code >>= 2;
				}
				dst += 248;
			}
		} else {
			int n = 8;
			while (n--) {
				uint8 code = *src++;
				for (int i = 0; i < 8; ++i) {
					*dst++ = data[code & 1];
					code >>= 1;
				}
				dst += 248;
			}
		}
	}
}

void Seq::ply_processFrameOp3(const uint8 *&src, uint8 *dst) {
	debug(DBG_SEQ, "Seq::ply_processFrameOp3()");
	for (int i = 0; i < 8; ++i) {
		memcpy(dst, src, 8);
		src += 8;
		dst += 256;
	}
}

void Seq::ply_processFrameOp4(const uint8 *&src, uint8 *dst) {
	debug(DBG_SEQ, "Seq::ply_processFrameOp4()");
	uint8 c;
	do {
		c = *src++;
		uint16 off = ((c >> 3) & 7) * 256 + (c & 7);
		dst[off] = *src++;
	} while (!(c & 0x80));
}

void Seq::ply_unpackFrame(const uint8 *&src) {
	static const int8 frameCodeTable[] = {
		0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1
	};
	static const uint8 frameLenTable[] = {
		0, 1, 2, 3, 4, 5, 6, 7,	8, 7, 6, 5,	4, 3, 2, 1
	};
	int8 tempBuffer[0x40];

	int8 *data = tempBuffer;
	uint8 size = 0;
	do {
		uint16 code = *src++;
		uint8 c1 = (code >> 0) & 0xF;
		uint8 c2 = (code >> 4) & 0xF;
		*data++ = frameCodeTable[c1];
		*data++ = frameCodeTable[c2];
		size += frameLenTable[c1] + frameLenTable[c2];
	} while (size < 64);
	memset(data, 0, 2);

	data = tempBuffer;
	uint8 *dst = _ply_unpackedFrameBuffer;
	while (1) {
		int16 code = *data++;
		if (code == 0) {
			break;
		} else if (code < 0) {
			code = -code;
			memset(dst, *src++, code);
			dst += code;
		} else {
			memcpy(dst, src, code);
			dst += code;
			src += code;
		}
	}
}

void Seq::vid_clearTempFrame() {
	debug(DBG_SEQ, "Seq::vid_clearTempFrame()");
	memset(_vid_tempFrame, 0, sizeof(_vid_tempFrame));
}

void Seq::vid_setPalSync(const uint8 *palData) {
	debug(DBG_SEQ, "Seq::vid_setPalSync()");
	_stub->setPalette(palData, 0x100);
}

void Seq::vid_clearPal() {
	debug(DBG_SEQ, "Seq::vid_clearPal()");
	memset(_vid_pal1, 0, 0x300);
	vid_setPalSync(_vid_pal1);
}

void Seq::vid_setPalBuf1(const uint8 *palData) {
	memcpy(_vid_pal1, palData, 0x300);
}

void Seq::snd_readNextFrame() {
	if (feof(_ply_fileHandleSnd)) {
		memset(_snd_buf, 0, 0x372 * 2);
	} else {
		++_ply_sndFrameNum;
		if (_ply_sndFrameNum == 1) {
			fseek(_ply_fileHandleSnd, 0x1800 * 100, SEEK_SET);
		}
		int pos = ftell(_ply_fileHandleSnd);
		uint16 offs = fread_uint16LE(_ply_fileHandleSnd);
		if (offs) {
			fseek(_ply_fileHandleSnd, pos + offs, SEEK_SET);
			fread(_snd_buf, 0x372 * 2, 1, _ply_fileHandleSnd);
		}
		fseek(_ply_fileHandleSnd, pos + 0x1800, SEEK_SET);
	}
}

void Seq::snd_audioCallback(uint8 *stream, int len) {
	const uint8 *end = stream + len;
	uint16 *dst = (uint16 *)stream;
	while (len != 0) {
		if (_snd_len == 0) {
			snd_readNextFrame();
			_snd_len = 0x372 * 2;
			_snd_pos = 0;
		}
		int sz = MIN(len, _snd_len);
		for (int i = 0; i < sz / 2; ++i) {
			*dst++ = READ_BE_UINT16(_snd_buf + _snd_pos);
			_snd_pos += 2;
		}
		_snd_len -= sz;
		len -= sz;
	}
	assert(end == (uint8 *)dst);
}
