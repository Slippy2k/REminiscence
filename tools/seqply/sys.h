
#ifndef __SYS_H__
#define __SYS_H__

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;

#if defined SYS_LITTLE_ENDIAN

inline uint16 READ_LE_UINT16(const void *ptr) {
	return *(const uint16 *)ptr;
}

inline uint32 READ_LE_UINT32(const void *ptr) {
	return *(const uint32 *)ptr;
}

inline uint16 READ_BE_UINT16(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[0] << 8) | b[1];
}

inline uint32 READ_BE_UINT32(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

#elif defined SYS_BIG_ENDIAN

inline uint16 READ_LE_UINT16(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[1] << 8) | b[0];
}

inline uint32 READ_LE_UINT32(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}

inline uint16 READ_BE_UINT16(const void *ptr) {
	return *(const uint16 *)ptr;
}

inline uint32 READ_BE_UINT32(const void *ptr) {
	return *(const uint32 *)ptr;
}

#else

#error No endianness defined

#endif

#endif
