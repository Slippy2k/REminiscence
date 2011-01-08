
#ifndef FILE_H__
#define FILE_H__

#include <stdio.h>
#include <stdint.h>

struct File_impl;

struct File {
	File_impl *_impl;

	File();
	~File();

	bool open(const char *filepath, const char *mode, const char *extmode = 0);
	void close();
	bool ioErr() const;
	uint32_t size();
	void seek(int off, int whence = SEEK_SET);
	int tell();
	void read(void *ptr, uint32_t len);
	uint8_t readByte();
	uint16_t readUint16BE();
	uint32_t readUint32BE();
	void write(void *ptr, size_t len);
	void writeByte(uint8_t val);
	void writeUint16BE(uint16_t val);
	void writeUint32BE(uint32_t val);
};

#endif
