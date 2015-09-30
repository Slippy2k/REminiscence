
#ifndef RESOURCE_ABA_H__
#define RESOURCE_ABA_H__

#include "file.h"

struct FileSystem;

struct ResourceAbaEntry {
	char name[14];
	uint32_t offset;
	uint32_t compressedSize;
	uint32_t size;
	uint32_t tag;
};

struct ResourceAba {

	static const char *FILENAME;
	static const int MAX_ENTRIES = 256;
	static const int TAG = 0x442E4D2E;

	FileSystem *_fs;
	File _f;
	struct ResourceAbaEntry _entries[MAX_ENTRIES];
	int _entriesCount;

	ResourceAba(FileSystem *fs);

	void readEntries();
	const ResourceAbaEntry *findEntry(const char *name) const;
	uint8_t *loadEntry(const char *name, uint32_t *size = 0);
};

#endif // RESOURCE_ABA_H__
