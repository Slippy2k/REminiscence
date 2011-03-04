
#include <stdarg.h>
#include "util.h"


uint16 g_debugMask;

void debug(uint16 cm, const char *msg, ...) {
	char buf[1024];
	if (cm & g_debugMask) {
		va_list va;
		va_start(va, msg);
		vsnprintf(buf, sizeof(buf), msg, va);
		va_end(va);
		printf("%s\n", buf);
		fflush(stdout);
	}
}

void warning(const char *msg, ...) {
	char buf[1024];
	va_list va;
	va_start(va, msg);
	vsnprintf(buf, sizeof(buf), msg, va);
	va_end(va);
	fprintf(stderr, "WARNING: %s!\n", buf);
}

