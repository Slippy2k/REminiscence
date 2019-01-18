
#include <math.h>
#include <stdint.h>

static const int kCutOffFreq = 4000;
static const int kFreq = 22050;

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

#define NZEROS 2
#define NPOLES 2
#define GAIN   7.655158005e+00

static float xv[NZEROS+1], yv[NPOLES+1];

void butterworth(const int8_t *in, int len, int8_t *out) {
	for (int i = 0; i < len; ++i) {
		xv[0] = xv[1]; xv[1] = xv[2];
		xv[2] = in[i] / GAIN;
		yv[0] = yv[1]; yv[1] = yv[2];
		yv[2] = (xv[0] + xv[2]) + 2 * xv[1] + (-0.2729352339 * yv[0]) + (0.7504117278 * yv[1]);
		if (yv[2] > 127) {
			out[i] = 127;
		} else if (yv[2] < -128) {
			out[i] = -128;
		} else {
			out[i] = (int8_t)yv[2];
		}
	}
}
