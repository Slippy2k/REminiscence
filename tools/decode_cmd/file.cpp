
#include "file.h"

uint8_t freadByte(FILE *fp) {
	return fgetc(fp);
}

uint16_t freadUint16BE(FILE *fp) {
	uint16_t n = fgetc(fp) << 8;
	n |= fgetc(fp);
	return n;
}

uint32_t freadUint32BE(FILE *fp) {
	uint32_t n = freadUint16BE(fp) << 16;
	n |= freadUint16BE(fp);
	return n;
}

void fwriteByte(FILE *fp, uint8_t n) {
	fputc(n, fp);
}

void fwriteUint16LE(FILE *fp, uint16_t n) {
	fwriteByte(fp, n & 0xFF);
	fwriteByte(fp, n >> 8);
}

void fwriteUint32LE(FILE *fp, uint32_t n) {
	fwriteUint16LE(fp, n & 0xFFFF);
	fwriteUint16LE(fp, n >> 16);
}
