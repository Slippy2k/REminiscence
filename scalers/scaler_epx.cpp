
#include "../scaler.h"
extern "C" {
#include "scalebit.h"
}

static void scale_epx(int factor, uint32_t *dst, int dstPitch, const uint32_t *src, int srcPitch, int w, int h) {
	scale(factor, dst, w * factor * sizeof(uint32_t), src, w * sizeof(uint32_t), 4, w, h);
}

const Scaler scaler_epx = {
	SCALER_TAG,
	"epx",
	2, 4,
	scale_epx
};

#ifndef USE_STATIC_SCALER
extern "C" {
	const Scaler *getScaler() {
		return &scaler_epx;
	}
};
#endif
