
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

void sinc(double pos, const int8_t *in, int len, int fsr, int8_t *out) {
	static const int fmax = kCutOffFreq; // should be < fsr / 2
	const int windowWidth = 16;
	const double r_g = 2 * fmax / (double)fsr; // calc gain correction factor
	double r_y = 0;
	for (int i = -windowWidth / 2; i < windowWidth / 2; ++i) {
		const int j = int(pos + i);
		const double r_w = .5 - .5 * cos(2 * M_PI * (.5 + (j - pos) / (double)windowWidth));
		const double r_a = 2 * M_PI * (j - pos) * fmax / (double)fsr;
		double r_snc = 1;
		if (r_a != 0.) {
			r_snc = sin(r_a) / r_a;
		}
		if (j >= 0 && j < len) {
			r_y += r_g * r_w * r_snc * in[j];
		}
	}
	*out = (int8_t)r_y;
}
