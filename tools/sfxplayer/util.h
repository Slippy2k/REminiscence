
#ifndef __UTIL_H__
#define __UTIL_H__

#include "intern.h"

enum {
	DBG_INFO  = 1 << 0,
	DBG_SND   = 1 << 1,
	DBG_MOD   = 1 << 2
};

extern uint16 g_debugMask;

extern void debug(uint16 cm, const char *msg, ...);
extern void error(const char *msg, ...);
extern void warning(const char *msg, ...);

extern void string_lower(char *p);
extern void string_upper(char *p);

#endif
