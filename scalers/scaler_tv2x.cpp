
#include <assert.h>
#include "../scaler.h"

static void scale_tv2x(int factor, uint32_t *dst, int dstPitch, const uint32_t *src, int srcPitch, int w, int h) {
	assert(factor == 2);
	for (int y = 0; y < h; ++y) {
		uint32_t *p = dst;
		for (int x = 0; x < w; ++x, p += 2) {
			const uint32_t color = src[x];
			p[0] = p[1] = color;
			uint32_t color2 = (((color & 0xFF00FF) * 7) >> 3) & 0xFF00FF;
			color2 |= (((color & 0xFF00) * 7) >> 3) & 0xFF00;
			p[dstPitch] = p[dstPitch + 1] = color2;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}

static const Scaler scaler = {
	SCALER_TAG,
	"tv2x",
	2, 2,
	scale_tv2x
};

extern "C" {
	const Scaler *getScaler() {
		return &scaler;
	}
};
