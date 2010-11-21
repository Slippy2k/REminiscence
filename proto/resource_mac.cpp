
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "resource_mac.h"

ResourceMac::ResourceMac(const char *filePath)
	: _types(0), _entries(0) {
	memset(&_map, 0, sizeof(_map));
	if (_f.open(filePath, "rb")) {
		loadMap();
	}
}

ResourceMac::~ResourceMac() {
	for (int i = 0; i < _map.typesCount; ++i) {
		free(_entries[i]);
	}
	free(_entries);
	free(_types);
}

void ResourceMac::loadMap() {
	_f.seek(83);
	uint32_t dataSize = _f.readUint32BE();
	uint32_t resourceOffset = 128 + ((dataSize + 127) & ~127);
	printf("resourceOffset = 0x%X\n", resourceOffset);

	_f.seek(resourceOffset);
	_dataOffset = resourceOffset + _f.readUint32BE();
	uint32_t mapOffset = resourceOffset + _f.readUint32BE();

	_f.seek(mapOffset + 22);
	_f.readUint16BE();
	_map.typesOffset = _f.readUint16BE();
	_map.namesOffset = _f.readUint16BE();
	_map.typesCount = _f.readUint16BE() + 1;

	_f.seek(mapOffset + _map.typesOffset + 2);
	_types = (ResourceType *)calloc(_map.typesCount, sizeof(ResourceType));
	for (int i = 0; i < _map.typesCount; ++i) {
		_f.read(_types[i].id, 4);
		_types[i].count = _f.readUint16BE() + 1;
		_types[i].startOffset = _f.readUint16BE();
	}
	_entries = (ResourceEntry **)calloc(_map.typesCount, sizeof(ResourceEntry *));
	for (int i = 0; i < _map.typesCount; ++i) {
		printf("entry type %d\n", i);
		_f.seek(mapOffset + _map.typesOffset + _types[i].startOffset, SEEK_SET);
		_entries[i] = (ResourceEntry *)calloc(_types[i].count, sizeof(ResourceEntry));
		for (int j = 0; j < _types[i].count; ++j) {
			_entries[i][j].id = _f.readUint16BE();
			_entries[i][j].nameOffset = _f.readUint16BE();
			_entries[i][j].dataOffset = _f.readUint32BE() & 0x00FFFFFF;
			_f.readUint32BE();
		}
		for (int j = 0; j < _types[i].count; ++j) {
			_entries[i][j].name[0] = '\0';
			if (_entries[i][j].nameOffset != 0xFFFF) {
				_f.seek(mapOffset + _map.namesOffset + _entries[i][j].nameOffset, SEEK_SET);
				const int len = _f.readByte();
				assert(len < sizeof(_entries[i][j].name) - 1);
				_f.read(_entries[i][j].name, len);
				_entries[i][j].name[len] = '\0';
				printf("Entry %d name '%s'\n", j, _entries[i][j].name);
			}
		}
	}
}

const ResourceEntry *ResourceMac::findEntry(const char *name) const {
	for (int type = 0; type < _map.typesCount; ++type) {
		for (int i = 0; i < _types[type].count; ++i) {
			if (strcmp(name, _entries[type][i].name) == 0) {
				return &_entries[type][i];
			}
		}
	}
	return 0;
}

