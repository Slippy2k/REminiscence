
#include <math.h>
#include <stdint.h>

static const int kCutOffFreq = 4000;
static const int kFreq = 11025;

void low_pass(const int8_t *in, int len, int8_t *out) {
	const double rc = 1. / (kCutOffFreq * 2 * M_PI);
	const double dt = 1. / kFreq;
	const double alpha = dt / (rc + dt);
	out[0] = in[0];
	for (int i = 1; i < len; ++i) {
		const double sample = out[i - 1] + alpha * (in[i] - out[i - 1]);
		out[i] = int8_t(sample);
	}
}

void nr(const int8_t *in, int len, int8_t *out) {
	static int prev = 0;
	for (int i = 0; i < len; ++i) {
		const int vnr = in[i] >> 1;
		out[i] = vnr + prev;
		prev = vnr;
	}
}
