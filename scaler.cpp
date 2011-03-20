/* REminiscence - Flashback interpreter
 * Copyright (C) 2005-2011 Gregory Montoir
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "scaler.h"


static void point1x(uint16 *dst, int dstPitch, const uint16 *src, int srcPitch, int w, int h) {
	dstPitch >>= 1;
	while (h--) {
		memcpy(dst, src, w * 2);
		dst += dstPitch;
		src += srcPitch;
	}
}

static void point2x(uint16 *dst, int dstPitch, const uint16 *src, int srcPitch, int w, int h) {
	dstPitch >>= 1;
	const int dstPitch2 = dstPitch * 2;
	while (h--) {
		uint16 *p = dst;
		for (int i = 0; i < w; ++i, p += 2) {
			const uint16 c = *(src + i);
			for (int j = 0; j < 2; ++j) {
				*(p + j) = *(p + dstPitch + j) = c;
			}
		}
		dst += dstPitch2;
		src += srcPitch;
	}
}

static void point3x(uint16 *dst, int dstPitch, const uint16 *src, int srcPitch, int w, int h) {
	dstPitch >>= 1;
	const int dstPitch2 = dstPitch * 2;
	const int dstPitch3 = dstPitch * 3;
	while (h--) {
		uint16 *p = dst;
		for (int i = 0; i < w; ++i, p += 3) {
			const uint16 c = *(src + i);
			for (int j = 0; j < 3; ++j) {
				*(p + j) = *(p + dstPitch + j) = *(p + dstPitch2 + j) = c;
			}
		}
		dst += dstPitch3;
		src += srcPitch;
	}
}

static void point4x(uint16 *dst, int dstPitch, const uint16 *src, int srcPitch, int w, int h) {
	dstPitch >>= 1;
	const int dstPitch2 = dstPitch * 2;
	const int dstPitch3 = dstPitch * 3;
	const int dstPitch4 = dstPitch * 4;
	while (h--) {
		uint16 *p = dst;
		for (int i = 0; i < w; ++i, p += 4) {
			const uint16 c = *(src + i);
			for (int j = 0; j < 4; ++j) {
				*(p + j) = *(p + dstPitch + j) = *(p + dstPitch2 + j) = *(p + dstPitch3 + j) = c;
			}
		}
		dst += dstPitch4;
		src += srcPitch;
	}
}

static void scale2x(uint16 *dst, int dstPitch, const uint16 *src, int srcPitch, int w, int h) {
	dstPitch >>= 1;
	const int dstPitch2 = dstPitch * 2;
	while (h--) {
		uint16 *p = dst;
		uint16 D = *(src - 1);
		uint16 E = *(src);
		for (int i = 0; i < w; ++i, p += 2) {
			uint16 B = *(src + i - srcPitch);
			uint16 F = *(src + i + 1);
			uint16 H = *(src + i + srcPitch);
			if (B != H && D != F) {
				*(p) = D == B ? D : E;
				*(p + 1) = B == F ? F : E;
				*(p + dstPitch) = D == H ? D : E;
				*(p + dstPitch + 1) = H == F ? F : E;
			} else {
				*(p) = E;
				*(p + 1) = E;
				*(p + dstPitch) = E;
				*(p + dstPitch + 1) = E;
			}
			D = E;
			E = F;
		}
		dst += dstPitch2;
		src += srcPitch;
	}
}

static void scale3x(uint16 *dst, int dstPitch, const uint16 *src, int srcPitch, int w, int h) {
	dstPitch >>= 1;
	const int dstPitch2 = dstPitch * 2;
	const int dstPitch3 = dstPitch * 3;
	while (h--) {
		uint16 *p = dst;
		uint16 A = *(src - srcPitch - 1);
		uint16 B = *(src - srcPitch);
		uint16 D = *(src - 1);
		uint16 E = *(src);
		uint16 G = *(src + srcPitch - 1);
		uint16 H = *(src + srcPitch);
		for (int i = 0; i < w; ++i, p += 3) {
			uint16 C = *(src + i - srcPitch + 1);
			uint16 F = *(src + i + 1);
			uint16 I = *(src + i + srcPitch + 1);
			if (B != H && D != F) {
				*(p) = D == B ? D : E;
				*(p + 1) = (D == B && E != C) || (B == F && E != A) ? B : E;
				*(p + 2) = B == F ? F : E;
				*(p + dstPitch) = (D == B && E != G) || (D == B && E != A) ? D : E;
				*(p + dstPitch + 1) = E;
				*(p + dstPitch + 2) = (B == F && E != I) || (H == F && E != C) ? F : E;
				*(p + dstPitch2) = D == H ? D : E;
				*(p + dstPitch2 + 1) = (D == H && E != I) || (H == F && E != G) ? H : E;
				*(p + dstPitch2 + 2) = H == F ? F : E;
			} else {
				*(p) = E;
				*(p + 1) = E;
				*(p + 2) = E;
				*(p + dstPitch) = E;
				*(p + dstPitch + 1) = E;
				*(p + dstPitch + 2) = E;
				*(p + dstPitch2) = E;
				*(p + dstPitch2 + 1) = E;
				*(p + dstPitch2 + 2) = E;
			}
			A = B;
			B = C;
			D = E;
			E = F;
			G = H;
			H = I;
		}
		dst += dstPitch3;
		src += srcPitch;
	}
}

const Scaler _scalers[] = {
	{ "point1x", &point1x, 1 },
	{ "point2x", &point2x, 2 },
	{ "scale2x", &scale2x, 2 },
	{ "point3x", &point3x, 3 },
	{ "scale3x", &scale3x, 3 },
	{ "point4x", &point4x, 4 },
	{ 0, 0, 0 }
};

