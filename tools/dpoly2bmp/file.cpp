
#include "file.h"

uint8 freadByte(FILE *fp) {
	return fgetc(fp);
}

uint16 freadUint16BE(FILE *fp) {
	uint16 n = fgetc(fp) << 8;
	n |= fgetc(fp);
	return n;
}

uint32 freadUint32BE(FILE *fp) {
	uint32 n = freadUint16BE(fp) << 16;
	n |= freadUint16BE(fp);
	return n;
}

void fwriteByte(FILE *fp, uint8 n) {
	fputc(n, fp);
}

void fwriteUint16LE(FILE *fp, uint16 n) {
	fwriteByte(fp, n & 0xFF);
	fwriteByte(fp, n >> 8);
}

void fwriteUint32LE(FILE *fp, uint32 n) {
	fwriteUint16LE(fp, n & 0xFFFF);
	fwriteUint16LE(fp, n >> 16);
}
