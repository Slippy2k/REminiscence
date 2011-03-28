
#ifndef SCALER_H__
#define SCALER_H__

#include "util.h"

extern void point1x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);
extern void point2x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);
extern void point3x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);
extern void scale2x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);
extern void scale3x(uint16_t *dst, int dstPitch, const uint16_t *src, int srcPitch, int w, int h);

#endif // SCALER_H__
