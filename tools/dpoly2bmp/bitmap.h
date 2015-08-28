
#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "endian.h"

void WriteFile_BMP_PAL(const char *filename, int w, int h, const uint8_t *bits, const uint8_t *pal);
void WriteFile_RAW_RGB(const char *filename, int w, int h, const uint32_t *bits);

#endif // __BITMAP_H__
