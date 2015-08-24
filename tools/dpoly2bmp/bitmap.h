
#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "endian.h"

extern void WriteBitmapFile(const char *filename, int w, int h, const uint8_t *bits, const uint8_t *pal);

#endif // __BITMAP_H__
