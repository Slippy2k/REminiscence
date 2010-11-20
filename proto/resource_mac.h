
#ifndef RESOURCE_MAC_H__
#define RESOURCE_MAC_H__

#include <stdint.h>
#include "file.h"

struct ResourceMap {
	uint16_t typesOffset;
	uint16_t namesOffset;
	uint16_t typesCount;
};

struct ResourceType {
	unsigned char id[4];
	uint16_t count;
	uint16_t startOffset;
};

enum {
	kResourceEntryNameLength = 64
};

struct ResourceEntry {
	uint16_t id;
	uint16_t nameOffset;
	uint32_t dataOffset;
	char name[kResourceEntryNameLength];
};

struct ResourceMac {

	File _f;

	uint32_t _dataOffset;
	ResourceMap _map;
	ResourceType *_types;
	ResourceEntry **_entries;

	ResourceMac(const char *filePath);
	~ResourceMac();

	void loadMap();
	const ResourceEntry *findEntry(const char *name) const;
	int findTypeById(const unsigned char id[4]) const;
};

#endif

