
#include "../scaler.h"
#include "hqx/HQ2x.hh"
#include "hqx/HQ3x.hh"

static void scale_hqx(int factor, uint32_t *dst, int dstPitch, const uint32_t *src, int srcPitch, int w, int h) {
	switch (factor) {
	case 2:
		HQ2x().resize(src, w, h, dst);
		break;
	case 3:
		HQ3x().resize(src, w, h, dst);
		break;
	}
}

static const Scaler scaler = {
	SCALER_TAG,
	"hqx",
	2, 3,
	scale_hqx
};

extern "C" {
	const Scaler *getScaler() {
		return &scaler;
	}
};
