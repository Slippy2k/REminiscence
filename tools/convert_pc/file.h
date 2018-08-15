
#ifndef FILE_H__
#define FILE_H__

#include <stdio.h>
#include <stdint.h>

struct File_impl;

struct File {
	File_impl *_impl;

	File();
	virtual ~File();

	bool open(const char *filepath, const char *mode);
	void close();
	bool ioErr() const;
	uint32_t size();
	void seek(int off, int whence = SEEK_SET);
	int tell();
	void read(void *ptr, uint32_t len);
	uint8_t readByte();
	uint16_t readUint16LE();
	uint32_t readUint32LE();
	uint16_t readUint16BE();
	uint32_t readUint32BE();
	void write(void *ptr, uint32_t len);
};

#endif
