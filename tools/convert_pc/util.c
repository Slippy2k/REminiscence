
#include "util.h"


uint16 read_uint16LE(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[1] << 8) | b[0];
}

uint32 read_uint32LE(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}

uint16 read_uint16BE(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[0] << 8) | b[1];
}

uint32 read_uint32BE(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

void write_uint16BE(void *ptr, uint16 value) {
	uint8 *b = (uint8 *)ptr;
	b[0] = value >> 8;
	b[1] = value & 0xFF;
}

void write_uint32BE(void *ptr, uint16 value) {
	uint8 *b = (uint8 *)ptr;
	b[0] = value >> 24;
	b[1] = (value >> 16) & 0xFF;
	b[2] = (value >> 8) & 0xFF;
	b[3] = value & 0xFF;
}
