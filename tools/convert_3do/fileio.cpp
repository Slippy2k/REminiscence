
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "fileio.h"

uint8_t *readFile(FILE *fp, int *size) {
	fseek(fp, 0, SEEK_END);
	*size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	uint8_t *buf = (uint8_t *)malloc(*size);
	if (buf) {
		const int count = fread(buf, 1, *size, fp);
		if (count != *size) {
			fprintf(stderr, "Failed to read %d bytes\n", count);
		}
	}
	return buf;
}

uint16_t freadUint16LE(FILE *fp) {
	uint8_t buf[2];
	fread(buf, 1, sizeof(buf), fp);
	return buf[0] | (buf[1] << 8);
}

uint16_t freadUint16BE(FILE *fp) {
	uint8_t buf[2];
	fread(buf, 1, sizeof(buf), fp);
	return (buf[0] << 8) | buf[1];
}

uint32_t freadUint32BE(FILE *fp) {
	uint8_t buf[4];
	fread(buf, 1, sizeof(buf), fp);
	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

uint32_t freadTag(FILE *fp, char *type) {
	fread(type, 4, 1, fp);
	uint32_t size = freadUint32BE(fp);
	return size;
}

void fwriteUint16LE(FILE *fp, uint16_t value) {
	fputc(value & 255, fp);
	fputc(value >> 8, fp);
}

void fwriteUint32LE(FILE *fp, uint32_t value) {
	fwriteUint16LE(fp, value & 0xFFFF);
	fwriteUint16LE(fp, value >> 16);
}
