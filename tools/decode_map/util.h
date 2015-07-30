
#ifndef __UTIL_H__
#define __UTIL_H__

#define MIN(a,b) ((a)<(b):(a)?(b))
#define MAX(a,b) ((a)>(b):(a)?(b))
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;

extern uint16 read_uint16LE(const void *ptr);
extern uint32 read_uint32LE(const void *ptr);
extern uint16 read_uint16BE(const void *ptr);
extern uint32 read_uint32BE(const void *ptr);
extern void write_uint16BE(void *ptr, uint16 value);
extern void write_uint32BE(void *ptr, uint16 value);

#endif /* __UTIL_H__ */
