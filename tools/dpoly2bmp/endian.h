/* REminiscence - Flashback interpreter
 * Copyright (C) 2005-2007 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;

inline uint16 READ_BE_UINT16(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[0] << 8) | b[1];
}

inline uint32 READ_BE_UINT32(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

inline uint16 READ_LE_UINT16(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[1] << 8) | b[0];
}

inline uint32 READ_LE_UINT32(const void *ptr) {
	const uint8 *b = (const uint8 *)ptr;
	return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}

#endif // __ENDIAN_H__
