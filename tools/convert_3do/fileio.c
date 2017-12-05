
#include <stdio.h>
#include <stdlib.h>

int fileReadUint16LE(FILE *fp) {
	unsigned char buf[2];
	fread(buf, 1, sizeof(buf), fp);
	return buf[0] | (buf[1] << 8);
}

int fileReadUint32LE(FILE *fp) {
	unsigned char buf[4];
	fread(buf, 1, sizeof(buf), fp);
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

int fileWriteUint16LE(FILE *fp, int n) {
	fputc(n & 255, fp);
	fputc((n >> 8) & 255, fp);
}

