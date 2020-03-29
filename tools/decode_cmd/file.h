
#ifndef FILE_H__
#define FILE_H__

#include <stdio.h>
#include <stdint.h>

extern uint8_t freadByte(FILE *fp);
extern uint16_t freadUint16BE(FILE *fp);
extern uint32_t freadUint32BE(FILE *fp);

extern void fwriteByte(FILE *fp, uint8_t n);
extern void fwriteUint16LE(FILE *fp, uint16_t n);
extern void fwriteUint32LE(FILE *fp, uint32_t n);

#endif // FILE_H__
