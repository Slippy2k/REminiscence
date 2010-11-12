
#include <stdio.h>
#include "file.h"

struct File_impl {
	bool _ioErr;
	File_impl() : _ioErr(false) {}
	virtual bool open(const char *path, const char *mode) = 0;
	virtual void close() = 0;
	virtual uint32_t size() = 0;
	virtual void seek(int off, int whence) = 0;
	virtual int tell() = 0;
	virtual void read(void *ptr, uint32_t len) = 0;
	virtual void write(void *ptr, uint32_t len) = 0;
};

struct stdFile : File_impl {
	FILE *_fp;
	stdFile() : _fp(0) {}
	bool open(const char *path, const char *mode) {
		_ioErr = false;
		_fp = fopen(path, mode);
		return (_fp != NULL);
	}
	void close() {
		if (_fp) {
			fclose(_fp);
			_fp = 0;
		}
	}
	uint32_t size() {
		uint32_t sz = 0;
		if (_fp) {
			int pos = ftell(_fp);
			fseek(_fp, 0, SEEK_END);
			sz = ftell(_fp);
			fseek(_fp, pos, SEEK_SET);
		}
		return sz;
	}
	void seek(int off, int whence) {
		if (_fp) {
			fseek(_fp, off, whence);
		}
	}
	int tell() {
		int pos = 0;
		if (_fp) {
			pos = ftell(_fp);
		}
		return pos;
	}
	void read(void *ptr, uint32_t len) {
		if (_fp) {
			uint32_t r = fread(ptr, 1, len, _fp);
			if (r != len) {
				_ioErr = true;
			}
		}
	}
	void write(void *ptr, uint32_t len) {
		if (_fp) {
			uint32_t r = fwrite(ptr, 1, len, _fp);
			if (r != len) {
				_ioErr = true;
			}
		}
	}
};


File::File() {
	_impl = new stdFile;
}

File::~File() {
	_impl->close();
	delete _impl;
}

bool File::open(const char *filepath, const char *mode) {	
	_impl->close();
	return _impl->open(filepath, mode);
}

void File::close() {
	_impl->close();
}

bool File::ioErr() const {
	return _impl->_ioErr;
}

uint32_t File::size() {
	return _impl->size();
}

int File::tell() {
	return _impl->tell();
}

void File::seek(int off, int whence) {
	_impl->seek(off, whence);
}

void File::read(void *ptr, uint32_t len) {
	_impl->read(ptr, len);
}

uint8_t File::readByte() {
	uint8_t b;
	read(&b, 1);
	return b;
}

uint16_t File::readUint16LE() {
	uint8_t lo = readByte();
	uint8_t hi = readByte();
	return (hi << 8) | lo;
}

uint32_t File::readUint32LE() {
	uint16_t lo = readUint16LE();
	uint16_t hi = readUint16LE();
	return (hi << 16) | lo;
}

uint16_t File::readUint16BE() {
	uint8_t hi = readByte();
	uint8_t lo = readByte();
	return (hi << 8) | lo;
}

uint32_t File::readUint32BE() {
	uint16_t hi = readUint16BE();
	uint16_t lo = readUint16BE();
	return (hi << 16) | lo;
}

void File::write(void *ptr, uint32_t len) {
	_impl->write(ptr, len);
}

