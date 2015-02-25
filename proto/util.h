
#ifndef __UTIL_H__
#define __UTIL_H__

#include "intern.h"

enum {
	DBG_INFO   = 1 << 0,
	DBG_PGE    = 1 << 1,
	DBG_GAME   = 1 << 2,
	DBG_COL    = 1 << 3,
	DBG_SND    = 1 << 4,
};

extern uint16_t g_debugMask;

extern void debug(uint16_t cm, const char *msg, ...);
extern void warning(const char *msg, ...);

#endif // __UTIL_H__
