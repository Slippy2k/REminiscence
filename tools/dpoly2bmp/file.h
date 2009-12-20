
#ifndef __FILE_H__
#define __FILE_H__

#include "endian.h"

extern uint8 freadByte(FILE *fp);
extern uint16 freadUint16BE(FILE *fp);
extern uint32 freadUint32BE(FILE *fp);

extern void fwriteByte(FILE *fp, uint8 n);
extern void fwriteUint16LE(FILE *fp, uint16 n);
extern void fwriteUint32LE(FILE *fp, uint32 n);

#endif // __FILE_H__
