
#ifndef BITMAP_H__
#define BITMAP_H__

#include <stdint.h>

void saveTGA(const char *filename, const uint8_t *rgb555, int w, int h);
void saveBMP(const char *filename, const uint8_t *bits, const uint8_t *pal, int w, int h);

#endif
