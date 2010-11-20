
#ifndef MAC_DATA_H__
#define MAC_DATA_H__

#include "resource_mac.h"

struct Color {
	int r, g, b;
};

Color *getClut();
void decodeClutData(ResourceMac &res);
uint8_t *decodeResourceData(ResourceMac &res, const char *name, bool decompressLzss);
int decodeImageData(const char *name, const uint8_t *ptr);

#endif

