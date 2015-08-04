
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "unpack.h"

static uint16_t READ_BE_UINT16(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[0] << 8) | b[1];
}

static uint32_t READ_BE_UINT32(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

static uint16_t freadUint16BE(FILE *fp) {
	uint8_t buf[2];
	fread(buf, 1, sizeof(buf), fp);
	return READ_BE_UINT16(buf);
}

static uint32_t freadUint32BE(FILE *fp) {
	uint8_t buf[4];
	fread(buf, 1, sizeof(buf), fp);
	return READ_BE_UINT32(buf);
}

#define MAX_ENTRIES 256
#define TAG 0x442E4D2E

static struct {
	char name[14];
	uint32_t offset;
	uint32_t compressedSize;
	uint32_t size;
	uint32_t tag;
} _entries[MAX_ENTRIES];
static int _entriesCount;

static void readEntries(FILE *fp) {
	int i;
	uint32_t nextOffset = 0;

	for (i = 0; i < _entriesCount; ++i) {
		fread(_entries[i].name, 1, sizeof(_entries[i].name), fp);
		_entries[i].offset = freadUint32BE(fp);
		_entries[i].compressedSize = freadUint32BE(fp);
		_entries[i].size = freadUint32BE(fp);
		_entries[i].tag = freadUint32BE(fp);
		assert(_entries[i].tag == TAG);
		fprintf(stdout, "%d/%d '%s' offset 0x%X size %d/%d\n", i, _entriesCount, _entries[i].name,  _entries[i].offset, _entries[i].compressedSize, _entries[i].size);
		if (i != 0) {
			assert(nextOffset == _entries[i].offset);
		}
		nextOffset = _entries[i].offset + _entries[i].compressedSize;
	}
}

static void dumpFile(const char *filename, const uint8_t *buf, int size) {
	char path[MAXPATHLEN];
	snprintf(path, sizeof(path), "out/%s", filename);
	FILE *fp = fopen(path, "wb");
	if (fp) {
		fwrite(buf, 1, size, fp);
		fclose(fp);
	}
}

static uint8_t _buffer[0x10000];
static uint8_t _out[0x10000];

static void dumpEntries(FILE *fp) {
	int i;

	for (i = 0; i < _entriesCount; ++i) {
		assert(_entries[i].compressedSize <= sizeof(_buffer));
		fseek(fp, _entries[i].offset, SEEK_SET);
		fread(_buffer, 1, _entries[i].compressedSize, fp);
		if (_entries[i].compressedSize == _entries[i].size) {
			dumpFile(_entries[i].name, _buffer, _entries[i].size);
		} else {
			assert(_entries[i].size <= sizeof(_out));
			const int ret = delphine_unpack(_buffer,  _entries[i].compressedSize, _out);
			assert(ret == 1);
			dumpFile(_entries[i].name,  _out, _entries[i].size);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			_entriesCount = freadUint16BE(fp);
			assert(_entriesCount <= MAX_ENTRIES);
			const int entrySize = freadUint16BE(fp);
			assert(entrySize == 30);
			readEntries(fp);
			dumpEntries(fp);
			fclose(fp);
		}
	}
	return 0;
}
